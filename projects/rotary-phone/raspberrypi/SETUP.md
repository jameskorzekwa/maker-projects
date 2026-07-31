# Raspberry Pi Bring-Up

Phase 1 of the migration described in `README.md`. The goal is a Pi that can
record and play audio through the WM8960 HAT, and a decision on whether the
microSD write tick that limits the ESP32 build survives the platform change.

Target hardware is a Raspberry Pi Zero W v1.1. That is the original Zero W:
single-core ARM1176 on ARMv6, 512 MB RAM, 32-bit only. See `README.md` for
what that costs.

## 1. Flash the operating system

Use Raspberry Pi Imager and choose **Raspberry Pi OS Lite (32-bit)**.

The 32-bit build is not optional on this board. The Zero W is ARMv6 and cannot
run the 64-bit images, which target ARMv8. Choosing a 64-bit image produces a
board that never appears on the network and gives no other symptom.

Before writing, open the Imager's settings and preconfigure:

- Hostname, for example `rotary-phone`
- SSH enabled with a public key
- Wi-Fi SSID, password, and country
- Locale and timezone

Doing this in the Imager avoids needing a keyboard and monitor, which the Zero
W cannot easily provide without adapters.

First boot on this board takes several minutes. It expands the filesystem and
reboots. Do not assume failure until at least five minutes have passed.

## 2. Enable the audio HAT

The Raspberry Pi kernel already contains an overlay for this board, listed in
the kernel's overlay reference as `wm8960-soundcard`, described there as
"Overlay for the Waveshare wm8960 soundcard".

Waveshare's own `WM8960-Audio-HAT` installer is therefore not needed. That
installer builds an out-of-tree module against the running kernel and has a
long history of breaking after kernel updates, which on a rented device means
a unit that stops recording after an unattended upgrade. Prefer the in-tree
overlay.

Edit `/boot/firmware/config.txt`:

```ini
dtparam=i2s=on
dtoverlay=wm8960-soundcard
```

On Bookworm this file is at `/boot/firmware/config.txt`. Older guides say
`/boot/config.txt`, which is now a compatibility symlink at best.

In the same file, comment out the onboard audio so the HAT is the only card:

```ini
# dtparam=audio=on
```

Reboot.

## 3. Confirm the card is present

```bash
aplay -l      # playback devices
arecord -l    # capture devices
```

Both should list a `wm8960soundcard`. If neither does:

- Re-check that the HAT is fully seated on all 40 pins
- Confirm `dtparam=i2s=on` came before the overlay line
- Run `dmesg | grep -i wm8960` and read the failure

The codec answers on I2C address `0x1A`, the same address confirmed during the
ESP32 build. `i2cdetect -y 1` is a quick way to separate "HAT not seated" from
"overlay not loaded".

## 4. Set the mixer

The WM8960 comes up with capture muted and its input routing unset. Nothing
records until the mixer is configured, and a silent recording at this stage
almost always means mixer state rather than wiring.

Enumerate the controls before changing anything, because names vary between
kernel versions:

```bash
amixer -c wm8960soundcard scontrols
```

Expect to set, at minimum:

- The capture switch, to unmute
- The input boost mixer, to connect `RINPUT1` to the ADC
- The capture volume

**This board's microphone is on the right channel.** The handset microphone
reaches the codec through `RINPUT1` by way of the `L3` and `C14` modification
described in `../TECHNICAL.md`. The left channel carries the HAT's onboard
microphone and is not used.

Persist the settings once they work:

```bash
sudo alsactl store
```

## 5. Record and play

```bash
arecord -D hw:CARD=wm8960soundcard -f S16_LE -r 16000 -c 2 -d 5 test.wav
aplay -D hw:CARD=wm8960soundcard test.wav
```

Record stereo even though only the right channel matters. The codec is a
stereo part, and forcing mono here tends to fail with confusing errors rather
than doing the obvious thing.

## 6. Run the gate test

This is the decision point for the whole migration, and it should happen
before any application code is written.

The ESP32 build's remaining defect is a loud tick in recordings, caused by the
microSD card's write-current surge coupling into the analog front end. It was
never solved, only masked, at a cost of about 3.4% of the audio being muted.
See section 12 of `../TECHNICAL.md`.

The migration assumes Linux removes this, because the kernel buffers capture
and performs writeback on a separate thread rather than blocking inside the
audio path as the firmware did. That assumption is worth testing directly.

```bash
tools/gate_test.sh
python3 tools/analyze_tick.py /tmp/gate/control.wav /tmp/gate/loaded.wav
```

The script records twice: once idle, once while writing hard to the card. The
analyzer reports periodic energy spikes in each.

Interpretation:

- **Similar in both** - the platform change fixed it. Proceed to Phase 2.
- **Spikes only under load** - the coupling is physical and follows the
  hardware, not the software. The mitigations in section 12.5 of
  `../TECHNICAL.md` become necessary on this platform too. Better to know now
  than after building the application.

Speak nothing during either recording. The analyzer looks for periodic
structure, and speech is not periodic but is loud enough to mask what is.

## 7. Wire the hook switch

Deferred to Phase 2, but note the constraint now: the HAT occupies the I2S
pins (GPIO 18, 19, 20, 21) and I2C (GPIO 2, 3). Choose a hook switch pin clear
of both. GPIO 17 or GPIO 27 are free and convenient.

The debounce network characterised during the ESP32 build carries over
unchanged: 10 kOhm pull-up with a 100 nF capacitor across the switch.
