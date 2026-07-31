#!/usr/bin/env bash
#
# Records twice, once idle and once while the microSD card is being written
# hard, so the two can be compared for the write tick that limits the ESP32
# build. See section 12 of ../../TECHNICAL.md.
#
# Say nothing during either recording. The analyser looks for periodic
# structure and speech masks it without being periodic itself.

set -euo pipefail

CARD="${CARD:-wm8960soundcard}"
SECONDS_PER_TAKE="${SECONDS_PER_TAKE:-20}"
OUT_DIR="${OUT_DIR:-/tmp/gate}"

# Must land on the real card, not a tmpfs, or nothing is exercised. /tmp is
# frequently RAM-backed, so the load file goes next to the recordings only if
# those are on disk; default to the invoking user's home instead.
LOAD_FILE="${LOAD_FILE:-$HOME/.gate_test_load.bin}"
LOAD_MB="${LOAD_MB:-20}"

mkdir -p "$OUT_DIR"

if ! arecord -l 2>/dev/null | grep -q "$CARD"; then
    echo "error: capture device '$CARD' not found. 'arecord -l' shows:" >&2
    arecord -l >&2 || true
    exit 1
fi

record() {
    arecord -D "hw:CARD=$CARD" -f S16_LE -r 16000 -c 2 \
        -d "$SECONDS_PER_TAKE" "$1"
}

echo "1/2 idle take, ${SECONDS_PER_TAKE}s. Stay quiet."
record "$OUT_DIR/control.wav"

echo "2/2 loaded take, ${SECONDS_PER_TAKE}s. Stay quiet."

# conv=fsync forces the data out to the card rather than leaving it in the
# page cache, which is the whole point: the current surge happens on the
# physical write, not on the write() syscall.
(
    while true; do
        dd if=/dev/urandom of="$LOAD_FILE" bs=1M count="$LOAD_MB" \
            conv=fsync status=none 2>/dev/null || true
        sync
    done
) &
LOAD_PID=$!

# Kill the load generator however this exits, including on interrupt.
trap 'kill "$LOAD_PID" 2>/dev/null || true; wait "$LOAD_PID" 2>/dev/null || true; rm -f "$LOAD_FILE"' EXIT

record "$OUT_DIR/loaded.wav"

kill "$LOAD_PID" 2>/dev/null || true
wait "$LOAD_PID" 2>/dev/null || true
rm -f "$LOAD_FILE"
trap - EXIT

echo
echo "Wrote $OUT_DIR/control.wav and $OUT_DIR/loaded.wav"
echo "Compare them:"
echo "  python3 $(dirname "$0")/analyze_tick.py $OUT_DIR/control.wav $OUT_DIR/loaded.wav"
