#!/usr/bin/env bash
#
# Pull guestbook recordings off the phone onto this computer, and optionally
# into Google Drive so they can be listened to anywhere.
#
# Safe to run as often as you like: it only copies files that are new or have
# changed, and it never deletes anything at either end.
#
#   ./pull_recordings.sh              pull new recordings
#   ./pull_recordings.sh --analyse    pull, then report level and clipping
#   ./pull_recordings.sh --all        re-copy everything, not just new files

set -uo pipefail

PHONE_HOST="${PHONE_HOST:-james@192.168.1.239}"
PHONE_DIR="${PHONE_DIR:-/home/james/messages}"
LOCAL_DIR="${LOCAL_DIR:-$HOME/rotary-phone-recordings}"

# Google Drive for Desktop syncs this folder, so anything landed here uploads
# on its own. Set DRIVE_DIR="" to skip that copy entirely.
DRIVE_DIR="${DRIVE_DIR:-$HOME/Library/CloudStorage/GoogleDrive-james.korzekwa@gmail.com/My Drive/AI}"

ANALYSE=0
IGNORE_EXISTING="--ignore-existing"
for arg in "$@"; do
    case "$arg" in
        --analyse|--analyze) ANALYSE=1 ;;
        --all) IGNORE_EXISTING="" ;;
        -h|--help) sed -n '3,12p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

mkdir -p "$LOCAL_DIR"

# accept-new adds an unknown host key without prompting, which BatchMode alone
# cannot do; without it a new hostname fails as "Host key verification failed"
# and looks like the phone is offline. Show ssh's actual error rather than
# guessing at the cause.
SSH_OPTS=(-o BatchMode=yes -o StrictHostKeyChecking=accept-new -o ConnectTimeout=10)
if ! ssh_error=$(ssh "${SSH_OPTS[@]}" "$PHONE_HOST" true 2>&1); then
    echo "error: cannot reach the phone at $PHONE_HOST" >&2
    [ -n "$ssh_error" ] && echo "       ssh said: $ssh_error" >&2
    echo "       check it is powered on and on the network" >&2
    exit 1
fi

before=$(find "$LOCAL_DIR" -name 'MSG*.WAV' 2>/dev/null | wc -l | tr -d ' ')

echo "pulling from $PHONE_HOST:$PHONE_DIR"
# Recordings only. prompt.wav and beep.wav belong to the device, not here.
rsync -av -e "ssh ${SSH_OPTS[*]}" $IGNORE_EXISTING \
    --include='MSG*.WAV' --exclude='*' \
    "$PHONE_HOST:$PHONE_DIR/" "$LOCAL_DIR/"
status=$?
if [ $status -ne 0 ]; then
    echo "error: rsync failed with status $status" >&2
    exit $status
fi

after=$(find "$LOCAL_DIR" -name 'MSG*.WAV' 2>/dev/null | wc -l | tr -d ' ')
echo
echo "$LOCAL_DIR: $after recordings ($((after - before)) new)"

if [ -n "$DRIVE_DIR" ]; then
    if [ -d "$DRIVE_DIR" ]; then
        rsync -a --ignore-existing --include='MSG*.WAV' --exclude='*' \
            "$LOCAL_DIR/" "$DRIVE_DIR/" && echo "copied to Google Drive"
    else
        echo "note: Drive folder not found, skipping ($DRIVE_DIR)" >&2
    fi
fi

if [ "$ANALYSE" -eq 1 ]; then
    echo
    python3 - "$LOCAL_DIR" <<'PY'
import math, os, struct, sys, wave

directory = sys.argv[1]
names = sorted(n for n in os.listdir(directory) if n.startswith("MSG") and n.endswith(".WAV"))
if not names:
    raise SystemExit("no recordings to analyse")

print("%-16s %7s %8s %8s  %s" % ("file", "secs", "peak", "rms", "notes"))
for name in names[-12:]:
    path = os.path.join(directory, name)
    try:
        with wave.open(path, "rb") as handle:
            frames = handle.getnframes()
            rate = handle.getframerate()
            channels = handle.getnchannels()
            raw = handle.readframes(frames)
    except (wave.Error, OSError) as error:
        print("%-16s  unreadable: %s" % (name, error))
        continue

    samples = struct.unpack("<%dh" % (len(raw) // 2), raw)
    if channels > 1:
        samples = samples[1::channels]
    if not samples:
        continue

    peak = max(abs(v) for v in samples)
    rms = math.sqrt(sum(v * v for v in samples) / len(samples))

    notes = []
    if peak >= 32700:
        notes.append("CLIPPING at full scale")
    elif peak > 16000:
        notes.append("hot")
    elif peak < 3000:
        notes.append("very quiet")
    else:
        notes.append("level ok")

    # A hard ceiling below full scale means something upstream is clipping and
    # the codec never saw it, which no amount of gain adjustment here fixes.
    ceiling = sum(1 for v in samples if peak * 0.75 < abs(v) <= peak * 0.85)
    above = sum(1 for v in samples if abs(v) > peak * 0.9)
    if peak < 30000 and above and ceiling > above * 8:
        notes.append("preamp clipping below full scale")

    print("%-16s %7.1f %8d %8.0f  %s"
          % (name, len(samples) / rate, peak, rms, ", ".join(notes)))
PY
fi
