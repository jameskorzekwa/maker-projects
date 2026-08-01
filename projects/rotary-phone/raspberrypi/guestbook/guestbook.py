#!/usr/bin/env python3
"""Rotary phone guestbook recorder.

Lift the handset, hear a greeting and a beep, leave a message, hang up. The
message is written once, after the handset is back on the cradle.

That last point is the reason this exists. The ESP32 implementation in
../../esphome/ streamed audio to a microSD card while recording, and the
card's write-current surge coupled a loud tick into the analog front end. It
was never solved, only masked by muting the audio across each write at a cost
of about 3.4% of every recording. See section 12 of ../../TECHNICAL.md.

Here the whole message is held in memory and written after the call ends, so
no write happens while the microphone is live. The problem cannot occur rather
than being mitigated.

Design notes:

- Audio comes from arecord over a pipe rather than a Python binding. The
  target is a Pi Zero W, and ARMv6 has no prebuilt wheels for sounddevice or
  pyalsaaudio, so either would be compiled from source on a single 1 GHz core.
  Reading 64 KB/s from a pipe costs nothing by comparison.
- The codec captures stereo. The handset microphone reaches it on the right
  channel through the L3 and C14 modification described in ../../TECHNICAL.md;
  the left channel carries the HAT's onboard microphone and is discarded.
- Nothing about playback is allowed to prevent recording. A missing greeting,
  a busy device or a dead speaker still results in a saved message.
"""

import errno
import math
import os
import re
import signal
import struct
import subprocess
import sys
import time
import wave

# --- configuration ----------------------------------------------------------

ALSA_DEVICE = os.environ.get("GUESTBOOK_DEVICE", "hw:0")
# The codec accepts only stereo at 16 kHz. Capture asks for exactly that, so
# it can use the hardware device directly and be certain nothing is quietly
# resampling. The greeting and beep are mono files, which hw: rejects outright
# with "Channels count non available", so playback goes through the plug layer
# to have channels converted.
PLAYBACK_DEVICE = os.environ.get("GUESTBOOK_PLAYBACK_DEVICE", "plughw:0")
SAMPLE_RATE = 16000
CAPTURE_CHANNELS = 2  # Codec is stereo; only the right channel is kept.
SAMPLE_BYTES = 2
FRAME_BYTES = CAPTURE_CHANNELS * SAMPLE_BYTES

HOOK_GPIO = 17
# Measured on the real cradle: several lift and replace cycles produced no
# bounce at all. This is insurance for a handset dropped rather than placed.
HOOK_DEBOUNCE_SECONDS = 0.1

MESSAGE_DIR = os.environ.get("GUESTBOOK_DIR", "/home/james/messages")
PROMPT_PATH = os.environ.get("GUESTBOOK_PROMPT", os.path.join(MESSAGE_DIR, "prompt.wav"))
MESSAGE_PATTERN = re.compile(r"^MSG(\d{5})\.WAV$")

# Time to raise the handset to the ear before the greeting starts.
LIFT_TO_PROMPT_SECONDS = 2.0
# Reaching this limit means the handset was left off the cradle rather than
# that somebody spoke for two minutes: knocked off, put down beside the phone,
# or a child playing. Audio past this point is discarded rather than saved,
# because a two-minute recording of a reception room is not a guestbook
# message and burying the real ones under them is worse than losing it.
# 120 s of stereo capture is about 7.7 MB against the ~300 MB free on a Pi
# Zero W, so memory is not the constraint here; usefulness is.
MAX_MESSAGE_SECONDS = 120
# Below this a "message" is a hang-up, a wrong number, or a child playing with
# the cradle. Saving them buries the real ones.
MIN_MESSAGE_SECONDS = 0.5

# Mixer state the codec needs, asserted at every start rather than trusted to
# survive in alsactl's saved state. Two of these default to off and break the
# signal path silently: without the input boost the PGA never reaches the ADC
# and recordings are silent, and without the output mixer PCM switch the DAC
# never reaches the headphone and the greeting is inaudible. Saved state also
# drifts: a capture gain set by hand and not stored reverted on the next
# reboot and put clipping straight back into the recordings.
MIXER_SETUP = [
    ("Right Input Mixer Boost", "on"),    # PGA -> ADC, defaults off
    ("Left Input Mixer Boost", "on"),
    ("Right Output Mixer PCM", "on"),     # DAC -> output mixer, defaults off
    ("Left Output Mixer PCM", "on"),
    ("Capture", "3"),                     # -15 dB; the MAX4466 runs hot
    ("ADC PCM", "195"),                   # 0 dB
    ("Playback", "255"),                  # 0 dB
    ("Headphone", "115"),                 # -6 dB into a 130 ohm earpiece
]

BEEP_HZ = 1000
BEEP_SECONDS = 0.25
# Beside the greeting on real storage rather than in /dev/shm. The beep was
# generated into tmpfs and went missing between startup and the first call,
# with write_beep() demonstrably working when run by hand and no systemd
# sandboxing to explain it. The cause was never identified, so the dependency
# was removed instead. It is written once at startup, never while recording,
# so this costs nothing that matters.
BEEP_PATH = os.environ.get("GUESTBOOK_BEEP", os.path.join(MESSAGE_DIR, "beep.wav"))

# 64 ms per read: fine enough to notice a hang-up promptly, coarse enough that
# the loop costs nothing.
READ_CHUNK_BYTES = 4096

_running = True


def log(message):
    """Timestamped line to stdout, which systemd captures into the journal."""
    print("%s  %s" % (time.strftime("%Y-%m-%d %H:%M:%S"), message), flush=True)


# --- audio ------------------------------------------------------------------


def write_beep(path):
    """Generate the post-greeting beep once, into tmpfs."""
    frames = int(SAMPLE_RATE * BEEP_SECONDS)
    fade = int(SAMPLE_RATE * 0.01)
    samples = []
    for i in range(frames):
        value = math.sin(2.0 * math.pi * BEEP_HZ * i / SAMPLE_RATE) * 12000
        # Fade both ends, or the discontinuity is itself an audible click.
        if i < fade:
            value *= i / fade
        elif i > frames - fade:
            value *= (frames - i) / fade
        samples.append(int(value))
    with wave.open(path, "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes(struct.pack("<%dh" % len(samples), *samples))


def apply_mixer():
    """Force the codec into a known state.

    Called at every start rather than relying on alsactl's saved state, which
    only holds what someone remembered to store. Failures are logged and
    ignored: a wrong mixer setting is recoverable, refusing to run is not.
    """
    for control, value in MIXER_SETUP:
        try:
            result = subprocess.run(
                ["amixer", "-c", "0", "sset", control, value],
                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, timeout=10,
            )
            if result.returncode != 0:
                log("mixer: could not set %s to %s: %s"
                    % (control, value, result.stderr.decode("utf-8", "replace").strip()))
        except (OSError, subprocess.SubprocessError) as error:
            log("mixer: %s failed: %s" % (control, error))
    log("mixer configured")


def play(path, hook):
    """Play a file, returning early if the handset goes back on the cradle.

    Returns False if playback failed, but the caller is expected to carry on
    regardless: losing the greeting is an inconvenience, losing the message is
    not acceptable.
    """
    if not os.path.exists(path):
        log("playback skipped, missing %s" % path)
        return False
    try:
        process = subprocess.Popen(
            ["aplay", "-D", PLAYBACK_DEVICE, "-q", path],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )
    except OSError as error:
        log("playback failed to start: %s" % error)
        return False

    while process.poll() is None:
        if not hook.lifted:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
            return False
        time.sleep(0.05)

    if process.returncode != 0:
        stderr = process.stderr.read().decode("utf-8", "replace").strip()
        log("playback of %s failed: %s" % (os.path.basename(path), stderr))
        return False
    return True


def record_until_hangup(hook):
    """Capture into memory until the handset is replaced or the limit is hit.

    Returns (raw interleaved stereo bytes, hit_limit). When hit_limit is true
    the caller must discard the audio: the handset was left off the cradle,
    not used to leave a message.
    """
    try:
        process = subprocess.Popen(
            [
                "arecord", "-D", ALSA_DEVICE,
                "-f", "S16_LE",
                "-r", str(SAMPLE_RATE),
                "-c", str(CAPTURE_CHANNELS),
                "-t", "raw",
                "-q",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except OSError as error:
        log("capture failed to start: %s" % error)
        return b"", False

    captured = bytearray()
    limit_bytes = MAX_MESSAGE_SECONDS * SAMPLE_RATE * FRAME_BYTES
    started = time.time()
    hit_limit = False

    try:
        while hook.lifted and len(captured) < limit_bytes:
            chunk = process.stdout.read(READ_CHUNK_BYTES)
            if not chunk:
                break
            captured.extend(chunk)
        hit_limit = len(captured) >= limit_bytes
    except (IOError, OSError) as error:
        if error.errno != errno.EINTR:
            log("capture read failed: %s" % error)
    finally:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=2)

    log("captured %.1f s in %.1f s wall time"
        % (len(captured) / (SAMPLE_RATE * FRAME_BYTES), time.time() - started))
    return bytes(captured), hit_limit


# --- storage ----------------------------------------------------------------


def next_message_path(directory):
    """Lowest unused MSGnnnnn.WAV.

    Derived by scanning rather than from a counter file, so an unclean
    shutdown cannot desynchronise a counter from what is on disk and start
    overwriting real messages.
    """
    highest = 0
    for name in os.listdir(directory):
        match = MESSAGE_PATTERN.match(name)
        if match:
            highest = max(highest, int(match.group(1)))
    return os.path.join(directory, "MSG%05d.WAV" % (highest + 1))


def save(raw_stereo, directory):
    """Write the right channel as a mono WAV. Returns the path, or None."""
    frames = len(raw_stereo) // FRAME_BYTES
    seconds = frames / SAMPLE_RATE
    if seconds < MIN_MESSAGE_SECONDS:
        log("discarded %.2f s, below the %.1f s minimum" % (seconds, MIN_MESSAGE_SECONDS))
        return None

    samples = struct.unpack("<%dh" % (frames * CAPTURE_CHANNELS), raw_stereo[:frames * FRAME_BYTES])
    right = samples[1::CAPTURE_CHANNELS]

    peak = max((abs(value) for value in right), default=0)
    total = sum(value * value for value in right)
    rms = math.sqrt(total / len(right)) if right else 0.0
    clipped = sum(1 for value in right if abs(value) >= 32700)

    final_path = next_message_path(directory)
    # Write to a temporary name and rename into place. A rename within one
    # filesystem is atomic, so a crash mid-write cannot leave a half-written
    # file sitting there under a name that looks like a real message.
    temp_path = os.path.join(directory, ".partial-%d.wav" % os.getpid())
    try:
        with wave.open(temp_path, "wb") as handle:
            handle.setnchannels(1)
            handle.setsampwidth(2)
            handle.setframerate(SAMPLE_RATE)
            handle.writeframes(struct.pack("<%dh" % len(right), *right))
        with open(temp_path, "rb+") as handle:
            handle.flush()
            os.fsync(handle.fileno())
        os.rename(temp_path, final_path)
        directory_fd = os.open(directory, os.O_RDONLY)
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    except OSError as error:
        log("could not save message: %s" % error)
        try:
            os.unlink(temp_path)
        except OSError:
            pass
        return None

    log("saved %s: %.1f s, peak %d, RMS %.0f" % (os.path.basename(final_path), seconds, peak, rms))
    if clipped:
        # The ESP32 build wasted hours judging "interference" on recordings
        # that were clipping. Say so plainly instead.
        log("WARNING: %d clipped samples (%.2f%%). Reduce capture gain."
            % (clipped, 100.0 * clipped / len(right)))
    return final_path


# --- hook switch ------------------------------------------------------------


class Hook:
    """Cradle switch on GPIO 17.

    Measured on the real phone: on cradle reads low because the switch closes
    to ground, lifted reads high through the internal pull-up.
    """

    def __init__(self):
        from gpiozero import DigitalInputDevice

        self._device = DigitalInputDevice(
            HOOK_GPIO, pull_up=True, bounce_time=HOOK_DEBOUNCE_SECONDS
        )

    @property
    def lifted(self):
        # gpiozero inverts this. With pull_up=True it treats the device as
        # active when the pin reads LOW, because that is what a switch to
        # ground does, so device.value == 1 means the pin is low. Measured on
        # this phone, the cradle switch closes when the handset is DOWN, so a
        # low pin means on the cradle and value == 1 means on the cradle.
        # Reading device.value as "lifted" inverts the whole state machine:
        # the greeting plays when the handset is replaced and stops when it is
        # picked up.
        return not bool(self._device.value)

    def wait_for(self, lifted, timeout=None):
        deadline = None if timeout is None else time.time() + timeout
        while self.lifted != lifted:
            if not _running:
                return False
            if deadline is not None and time.time() > deadline:
                return False
            time.sleep(0.02)
        return True


# --- main -------------------------------------------------------------------


def handle_signal(signum, frame):
    global _running
    _running = False
    log("signal %d, shutting down" % signum)


def main():
    signal.signal(signal.SIGTERM, handle_signal)
    signal.signal(signal.SIGINT, handle_signal)

    os.makedirs(MESSAGE_DIR, exist_ok=True)
    write_beep(BEEP_PATH)
    apply_mixer()

    hook = Hook()
    log("guestbook ready, device %s, messages in %s" % (ALSA_DEVICE, MESSAGE_DIR))
    log("handset is currently %s" % ("lifted" if hook.lifted else "on the cradle"))

    while _running:
        if not hook.wait_for(lifted=True, timeout=0.5):
            continue

        log("handset lifted")
        # Let the caller get the handset to their ear before speaking starts.
        if not hook.wait_for(lifted=False, timeout=LIFT_TO_PROMPT_SECONDS):
            play(PROMPT_PATH, hook)
            if hook.lifted:
                play(BEEP_PATH, hook)

        if not hook.lifted:
            log("hung up before recording started")
            continue

        raw, hit_limit = record_until_hangup(hook)
        if hit_limit:
            # Deliberately not saved. See MAX_MESSAGE_SECONDS.
            log("discarded %.0f s: handset left off the cradle, not a message"
                % (len(raw) / (SAMPLE_RATE * FRAME_BYTES)))
        elif raw:
            log("handset replaced")
            save(raw, MESSAGE_DIR)

        # Wait for the handset to actually go back down, with no timeout. It
        # may already be down, or it may have been left off for an hour. Either
        # way the next recording must not begin until it has been replaced and
        # lifted again, otherwise a handset left off the cradle records the
        # room on a loop, two minutes at a time, forever.
        hook.wait_for(lifted=False)

    log("stopped")
    return 0


if __name__ == "__main__":
    sys.exit(main())
