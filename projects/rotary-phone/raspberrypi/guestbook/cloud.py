#!/usr/bin/env python3
"""Device API client: the phone's half of the contract in device-protocol.md.

Runs alongside the recorder as a separate service. The split is deliberate and
is the most important design decision here: recording must never be blocked,
delayed or crashed by anything to do with the network. A guest lifting the
handset at a reception has no idea the Wi-Fi is down, and must not be able to
tell.

So this process only ever reads finished recordings from disk. It never
touches the audio pipeline, never holds a lock the recorder needs, and can be
stopped or crash without the phone losing its ability to record.

Standard library only. The target is a Pi Zero W, where ARMv6 has no prebuilt
wheels for `requests` and installing it means compiling on a single 1 GHz core.
"""

import json
import hashlib
import os
import random
import signal
import ssl
import sys
import time
import urllib.error
import urllib.request

# --- configuration ----------------------------------------------------------

API_BASE = os.environ.get("HH_API_BASE", "https://api.heirloomhotline.com/api/device/v1")
IDENTITY_PATH = os.environ.get("HH_IDENTITY", "/boot/firmware/heirloom-identity.json")
MESSAGE_DIR = os.environ.get("GUESTBOOK_DIR", "/home/james/messages")
STATE_DIR = os.environ.get("HH_STATE_DIR", "/var/lib/guestbook")

CHECKIN_SECONDS = 60
# Backoff for a venue whose Wi-Fi is flaky or captive. Capped so a phone that
# comes back after an outage rejoins within a quarter hour rather than hours.
BACKOFF_START = 5
BACKOFF_CAP = 900
HTTP_TIMEOUT = 30

# A sidecar marks a file uploaded. A sidecar rather than deleting or renaming
# because the card copy stays authoritative until the depot confirms parity,
# and renaming would break the recorder's MSGnnnnn numbering.
UPLOADED_SUFFIX = ".uploaded"

_running = True


def log(message):
    print("%s  %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), message), flush=True)


# --- identity ---------------------------------------------------------------


def load_identity():
    """Read the unit token written to the boot partition at provisioning.

    Returns None when absent: an unprovisioned phone should still record, it
    just has nowhere to send anything.
    """
    try:
        with open(IDENTITY_PATH) as handle:
            identity = json.load(handle)
    except FileNotFoundError:
        log("no identity file at %s; running offline" % IDENTITY_PATH)
        return None
    except (OSError, ValueError) as error:
        log("identity file unreadable: %s" % error)
        return None

    if not identity.get("token"):
        log("identity file has no token; running offline")
        return None
    return identity


# --- http -------------------------------------------------------------------


class ApiError(Exception):
    def __init__(self, status, body=""):
        super().__init__("HTTP %s" % status)
        self.status = status
        self.body = body


def request(token, method, path, payload=None, timeout=HTTP_TIMEOUT):
    url = path if path.startswith("http") else API_BASE + path
    data = json.dumps(payload).encode() if payload is not None else None

    req = urllib.request.Request(url, data=data, method=method)
    req.add_header("authorization", "Bearer %s" % token)
    if data is not None:
        req.add_header("content-type", "application/json")

    try:
        with urllib.request.urlopen(req, timeout=timeout, context=ssl.create_default_context()) as response:
            body = response.read()
            return json.loads(body) if body else {}
    except urllib.error.HTTPError as error:
        raise ApiError(error.code, error.read().decode("utf-8", "replace")[:500])
    except (urllib.error.URLError, OSError, ValueError) as error:
        raise ApiError(0, str(error))


# --- local state ------------------------------------------------------------


def sha256_of(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        # Chunked so a long message does not have to fit in memory beside the
        # recorder's own buffer.
        for chunk in iter(lambda: handle.read(1024 * 256), b""):
            digest.update(chunk)
    return digest.hexdigest()


def pending_recordings():
    """Finished recordings with no sidecar, oldest first."""
    try:
        names = sorted(
            name
            for name in os.listdir(MESSAGE_DIR)
            if name.startswith("MSG") and name.endswith(".WAV")
        )
    except OSError as error:
        log("cannot list %s: %s" % (MESSAGE_DIR, error))
        return []

    out = []
    for name in names:
        path = os.path.join(MESSAGE_DIR, name)
        if os.path.exists(path + UPLOADED_SUFFIX):
            continue
        # Skip anything still being written. The recorder writes to a
        # .partial- name and renames, so a MSG file is complete by the time it
        # has that name, but guard anyway rather than upload a truncated WAV.
        if name.startswith(".partial"):
            continue
        out.append(path)
    return out


def mark_uploaded(path):
    try:
        with open(path + UPLOADED_SUFFIX, "w") as handle:
            handle.write(time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    except OSError as error:
        log("could not mark %s uploaded: %s" % (path, error))


# --- telemetry --------------------------------------------------------------


def card_free_mb():
    try:
        stat = os.statvfs(MESSAGE_DIR)
        return int(stat.f_bavail * stat.f_frsize / (1024 * 1024))
    except OSError:
        return None


def rootfs_read_only():
    """Whether the root filesystem is actually mounted read-only.

    Reported so the fleet dashboard can flag a unit that booted read-write and
    has silently lost its corruption protection. It will work perfectly until
    the day power is pulled mid-write.
    """
    try:
        with open("/proc/mounts") as handle:
            for line in handle:
                fields = line.split()
                if len(fields) >= 4 and fields[1] == "/":
                    return "ro" in fields[3].split(",")
    except OSError:
        pass
    return None


def wifi_rssi():
    try:
        with open("/proc/net/wireless") as handle:
            for line in handle:
                if ":" in line:
                    parts = line.split()
                    if len(parts) >= 4:
                        return int(float(parts[3]))
    except (OSError, ValueError):
        pass
    return None


def app_version():
    return os.environ.get("HH_APP_VERSION", "0.0.0-dev")


def os_version():
    try:
        with open("/etc/rpi-issue") as handle:
            for line in handle:
                if "Raspberry Pi reference" in line:
                    return line.strip().split()[-1]
    except OSError:
        pass
    return None


# --- operations -------------------------------------------------------------


def checkin(token, pending_count):
    payload = {
        "app": app_version(),
        "os": os_version(),
        "sd_free_mb": card_free_mb(),
        "pending_uploads": pending_count,
        "rootfs_ro": rootfs_read_only(),
        "rssi": wifi_rssi(),
        "recordings_on_card": pending_count,
    }
    # Drop unknown values rather than sending nulls the server would store.
    payload = {k: v for k, v in payload.items() if v is not None}
    return request(token, "POST", "/checkin", payload)


def upload_one(token, path):
    """Upload a single recording. Returns True when the server has it."""
    name = os.path.basename(path)
    size = os.path.getsize(path)
    digest = sha256_of(path)

    begin = request(
        token,
        "POST",
        "/uploads/begin",
        {
            "filename": name,
            "size_bytes": size,
            "sha256": digest,
            "recorded_at": time.strftime(
                "%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(os.path.getmtime(path))
            ),
        },
    )

    if begin.get("already_stored"):
        log("%s already stored server-side" % name)
        return True

    url = begin.get("url")
    upload_id = begin.get("upload_id")
    if not url or not upload_id:
        log("%s: begin returned no destination" % name)
        return False

    if url.startswith("stub://"):
        # The server is running without object storage configured. Treat the
        # upload as unsuccessful rather than marking the card copy done, or a
        # later wipe would delete a message that was never actually stored.
        log("%s: server storage is a stub; not uploading" % name)
        return False

    with open(path, "rb") as handle:
        put = urllib.request.Request(url, data=handle.read(), method="PUT")
        put.add_header("content-type", "audio/wav")
        try:
            with urllib.request.urlopen(put, timeout=HTTP_TIMEOUT * 4) as response:
                if response.status not in (200, 201):
                    log("%s: PUT returned %s" % (name, response.status))
                    return False
        except (urllib.error.URLError, OSError) as error:
            log("%s: PUT failed: %s" % (name, error))
            return False

    result = request(token, "POST", "/uploads/complete", {"upload_id": upload_id})
    if result.get("stored"):
        log("uploaded %s (%.1f KB)" % (name, size / 1024))
        return True

    log("%s: complete returned %s" % (name, result))
    return False


def run_actions(token, actions):
    for action in actions:
        ok, detail = True, None
        # Only actions that are safe without touching the recorder are handled
        # here. Anything needing the audio device belongs in the recorder.
        if action == "send_logs":
            detail = "not implemented"
            ok = False
        else:
            detail = "unsupported by this client"
            ok = False
        try:
            request(token, "POST", "/actions/ack", {"action": action, "ok": ok, "detail": detail})
        except ApiError as error:
            log("could not ack %s: %s" % (action, error))


# --- main -------------------------------------------------------------------


def handle_signal(signum, frame):
    global _running
    _running = False
    log("signal %d, shutting down" % signum)


def main():
    signal.signal(signal.SIGTERM, handle_signal)
    signal.signal(signal.SIGINT, handle_signal)
    os.makedirs(STATE_DIR, exist_ok=True)

    identity = load_identity()
    if not identity:
        # Nothing to do, but exiting would make systemd restart forever. Idle
        # instead so the recorder keeps working and a later provisioning is
        # picked up on the next restart.
        log("idle: no identity")
        while _running:
            time.sleep(30)
        return 0

    token = identity["token"]
    log("cloud client started for %s" % identity.get("serial", "unknown unit"))

    backoff = BACKOFF_START
    while _running:
        pending = pending_recordings()
        try:
            response = checkin(token, len(pending))
            backoff = BACKOFF_START

            if response.get("actions"):
                run_actions(token, response["actions"])

            # Upload one per cycle. Slow and deliberate: a Pi Zero W uploading
            # flat out competes with the recorder for CPU and Wi-Fi, and there
            # is no deadline here that matters.
            if pending:
                path = pending[0]
                if upload_one(token, path):
                    mark_uploaded(path)

        except ApiError as error:
            if error.status == 401:
                # Expected after a rental is wiped and the token rotated. Keep
                # recording, stop talking, and show up as token-mismatch on the
                # dashboard rather than hammering a 401 loop.
                log("token rejected; entering quarantine, recording continues")
                while _running:
                    time.sleep(300)
                return 0

            log("check-in failed (%s); retrying in %ds" % (error, backoff))
            slept = 0
            # Jitter so a room full of phones does not retry in lockstep.
            delay = backoff + random.uniform(0, backoff * 0.3)
            while _running and slept < delay:
                time.sleep(1)
                slept += 1
            backoff = min(backoff * 2, BACKOFF_CAP)
            continue

        slept = 0
        while _running and slept < CHECKIN_SECONDS:
            time.sleep(1)
            slept += 1

    log("stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
