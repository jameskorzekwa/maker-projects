# Rotary Phone Guestbook on Raspberry Pi

A second implementation of the guestbook, alongside the ESP32 build in
`../esphome/`. Both are kept. The ESP32 version records and plays back
correctly and is a working reference; this one exists because of what it could
not fix.

## Why

Two reasons, and the second matters more than the first.

**The tick.** The ESP32 build has a loud tick in recordings caused by the
microSD card's write-current surge coupling into the analog front end. It was
diagnosed thoroughly and never solved, only masked by muting the audio across
each write, at a cost of about 3.4% of the recording. Section 12 of
`../TECHNICAL.md` records what was eliminated and what remains untried.

The root cause is that the firmware calls a blocking write from inside the
audio capture path. Under Linux that structure disappears: the kernel buffers
capture and performs writeback on a separate thread. More decisively, a Pi has
enough memory to hold an entire message and write it once after hang-up, so
no write occurs during recording at all. The problem stops being mitigated and
starts being impossible.

**The hardware mismatch.** The WM8960 Audio HAT is a Raspberry Pi HAT. The
ESP32 build hand-wires it with eight jumper wires, no I2C pull-ups, manual
codec register writes, and a hand-rolled I2S DMA and ring buffer. A meaningful
share of the difficulty in that build comes from running Pi hardware on
something that is not a Pi. On a Pi it seats on the header and the kernel
already has a driver for it.

## Target board and its cost

A Raspberry Pi Zero W v1.1: single-core ARM1176 on **ARMv6**, 512 MB RAM,
32-bit only.

512 MB is ample. Two minutes at 16 kHz 16-bit mono is 3.84 MB, so the
record-to-memory design has room to spare.

ARMv6 is the real constraint, and it is an ecosystem problem rather than a
speed one. Many Python packages ship no ARMv6 wheels and are compiled from
source on a single 1 GHz core, turning short installs into long ones. Much of
the container ecosystem, including images that fleet tools such as balena
depend on, has dropped ARMv6 entirely. Prefer standard-library Python and
system packages over anything that needs building. The tools here follow that
rule already.

A Pi Zero 2 W is a drop-in improvement for a production fleet: four ARMv8
cores, working wheels, and a live container ecosystem. Capture, a web server,
an MQTT client and file writing all contend for one core on the Zero W, and a
starved audio callback means a dropout in someone's message.

## Phases

| Phase | Scope | State |
| --- | --- | --- |
| 0 | Preserve and document the ESP32 build | Done |
| 1 | OS, HAT, mixer, and the tick gate test | Done, gate passed |
| 2 | Handset wiring: earpiece, microphone, hook switch | Hook switch done |
| 3 | Recorder application and state machine | |
| 4 | Robustness: read-only root, service supervision | |
| 5 | Home Assistant integration over MQTT | |
| 6 | Local web access to messages | |
| 7 | Fleet provisioning and management | |

Phase 1 is in `SETUP.md`. It ends with a gate test that decides whether the
premise of this migration holds, and it comes before any application code on
purpose.

## The gate test

```bash
tools/gate_test.sh
python3 tools/analyze_tick.py /tmp/gate/control.wav /tmp/gate/loaded.wav
```

Records twice, once idle and once while writing hard to the card, then reports
periodic energy spikes in each. Card-write ticks appear as spikes that are
both louder than the noise floor and evenly spaced; room noise is neither.

`analyze_tick.py` refuses to give a verdict on clipping audio. That is
deliberate. Clipping is harsh and crackly and reads as interference, and
during the ESP32 build several hours were spent judging "interference" on
recordings that were clipping 41% of their samples.

The detector was validated against a synthetic 100 ms burst injected every
3.072 seconds: it recovered a 3.070 s interval and reported no spikes in the
matching clean take.

## What carries over

Everything learned about the physical phone, unchanged:

- Handset wiring, earpiece resistance, microphone polarity and the `L3` and
  `C14` modification, all in `../TECHNICAL.md`
- Hook switch behaviour and its 10 kOhm and 100 nF debounce network
- The call state machine: lift, pause, greeting, beep, record, hang up,
  finalise, rearm
- Crash-safe file naming and the recovery rules
- The Home Assistant integration design

What does not carry over is the firmware itself: the ESP-IDF component, the
I2S DMA and ring buffer, the manual codec register writes, and the mute
machinery built to hide the tick. On this platform none of it is needed.
