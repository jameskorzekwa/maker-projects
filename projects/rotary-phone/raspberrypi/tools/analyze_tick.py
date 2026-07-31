#!/usr/bin/env python3
"""Look for periodic energy spikes in a recording.

Written for the gate test in SETUP.md: compare a recording made while the
microSD card is idle against one made while it is being written hard. A tick
caused by card writes shows up as spikes that are both louder than the noise
floor and evenly spaced.

Standard library only, deliberately. This runs on a Pi Zero W, where ARMv6 has
no prebuilt wheels for most of the scientific stack and numpy would be
compiled from source.
"""

import math
import statistics
import struct
import sys
import wave

WINDOW_MS = 10
# A spike must clear both tests: well above the local noise floor, and not so
# quiet that it is inaudible anyway. The floor multiple catches ticks in quiet
# recordings; the absolute value stops a near-silent file reporting hundreds of
# meaningless spikes.
FLOOR_MULTIPLE = 4.0
ABSOLUTE_FLOOR = 150.0


def read_channel(path, channel="right"):
    """Return (samples, sample_rate). Mono files ignore the channel choice."""
    with wave.open(path, "rb") as handle:
        if handle.getsampwidth() != 2:
            raise SystemExit(f"{path}: expected 16-bit audio")
        frames = handle.getnframes()
        rate = handle.getframerate()
        channels = handle.getnchannels()
        raw = handle.readframes(frames)

    samples = struct.unpack(f"<{len(raw) // 2}h", raw)
    if channels == 1:
        return list(samples), rate
    # The handset microphone reaches the codec on RINPUT1, so the right
    # channel is the one carrying real signal. See ../../TECHNICAL.md.
    offset = 1 if channel == "right" else 0
    return list(samples[offset::channels]), rate


def rms_envelope(samples, rate):
    """RMS per fixed window, as (start_sample, rms) pairs."""
    width = max(1, int(rate * WINDOW_MS / 1000))
    out = []
    for start in range(0, len(samples) - width + 1, width):
        block = samples[start:start + width]
        total = sum(value * value for value in block)
        out.append((start, math.sqrt(total / len(block))))
    return out


def find_spikes(envelope, floor):
    """Cluster adjacent loud windows and return one start index per cluster."""
    threshold = max(floor * FLOOR_MULTIPLE, ABSOLUTE_FLOOR)
    spikes = []
    in_spike = False
    for start, value in envelope:
        if value > threshold:
            if not in_spike:
                spikes.append(start)
                in_spike = True
        else:
            in_spike = False
    return spikes, threshold


def describe(path):
    samples, rate = read_channel(path)
    if not samples:
        raise SystemExit(f"{path}: no audio")

    envelope = rms_envelope(samples, rate)
    if not envelope:
        raise SystemExit(f"{path}: too short to analyse")

    values = sorted(value for _, value in envelope)
    # Lower quartile rather than median: any speech or handling noise that
    # slips into a take drags the median up far enough to hide the ticks the
    # threshold is meant to catch.
    floor = values[len(values) // 4]
    peak = max(abs(sample) for sample in samples)
    clipped = peak >= 32767
    spikes, threshold = find_spikes(envelope, floor)

    duration = len(samples) / rate
    print(f"{path}")
    print(f"  duration      {duration:.1f} s")
    print(f"  peak          {peak}")
    print(f"  noise floor   {floor:.0f} (RMS, {WINDOW_MS} ms windows)")
    print(f"  threshold     {threshold:.0f}")
    print(f"  spikes        {len(spikes)}")

    if clipped:
        print("  WARNING: clipping. Levels are wrong and any verdict here is")
        print("           unreliable. Fix gain before trusting this.")

    intervals = []
    if len(spikes) >= 3:
        intervals = [
            (spikes[i] - spikes[i - 1]) / rate for i in range(1, len(spikes))
        ]
        median = statistics.median(intervals)
        spread = statistics.pstdev(intervals) if len(intervals) > 1 else 0.0
        print(f"  interval      {median:.3f} s median")
        print(f"  spread        {spread:.3f} s")
        # Evenly spaced spikes indicate a machine causing them. Speech and
        # room noise are irregular.
        if median > 0 and spread / median < 0.25:
            print("  -> REGULAR spacing: consistent with a periodic source")
        else:
            print("  -> irregular spacing: more like ambient noise or speech")
    elif spikes:
        print("  interval      too few spikes to judge spacing")

    return {
        "path": path,
        "floor": floor,
        "spikes": len(spikes),
        "duration": duration,
        "clipped": clipped,
        "per_second": len(spikes) / duration if duration else 0.0,
    }


def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit(
            "usage: analyze_tick.py <control.wav> [loaded.wav]"
        )

    results = []
    for path in sys.argv[1:]:
        results.append(describe(path))
        print()

    if len(results) != 2:
        return

    control, loaded = results
    print("verdict")
    print(f"  idle    {control['per_second']:.2f} spikes/s, "
          f"floor {control['floor']:.0f}")
    print(f"  loaded  {loaded['per_second']:.2f} spikes/s, "
          f"floor {loaded['floor']:.0f}")

    if control["clipped"] or loaded["clipped"]:
        print()
        print("  NO VERDICT: a take is clipping. Clipping is harsh and")
        print("  crackly and reads as interference, so this comparison")
        print("  cannot separate the two. Fix the capture gain and retake.")
        return

    # Compare rates rather than counts so unequal take lengths still compare.
    extra = loaded["per_second"] - control["per_second"]
    if extra > 0.15:
        print()
        print("  Card writes are audible in the recording. The coupling is")
        print("  physical and followed the hardware across the platform")
        print("  change. Apply the hardware mitigations in section 12.5 of")
        print("  ../../TECHNICAL.md before building on this.")
    else:
        print()
        print("  No meaningful difference under card load. The platform")
        print("  change resolved it; the ESP32 mute machinery is not needed.")


if __name__ == "__main__":
    main()
