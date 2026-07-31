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
dtparam=i2c_arm=on
dtparam=i2s=on
dtoverlay=wm8960-soundcard
```

All three lines are required. Enabling I2S alone loads the modules and
registers the I2S controller, but the codec is addressed over I2C, so without
`i2c_arm` the driver never finds it. The failure is quiet and misleading:
`lsmod` shows `snd_soc_wm8960` loaded, `dmesg` reports no error, and
`arecord -l` simply lists nothing. Check `/sys/kernel/debug/asoc/components`
to see the I2S controller present with no codec beside it.

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

The card exposes 53 controls. Enumerate them before changing anything, since
names vary between kernel versions:

```bash
amixer -c wm8960soundcard scontrols
```

On kernel 6.18 the defaults are almost right, with one exception that blocks
capture entirely:

| Control | Default | Needed |
| --- | --- | --- |
| `Capture` | 39/63 (+12 dB), on | fine |
| `ADC PCM` | 195/255 (0 dB) | fine |
| `Right Boost Mixer RINPUT1` | on | fine |
| `Left/Right Input Mixer Boost` | **off** | **on** |

`Input Mixer Boost` connects the input PGA's output into the boost mixer that
feeds the ADC. With it off the signal path is broken after the PGA, so
recordings are silent even though every other control looks correct:

```bash
amixer -c 0 sset 'Right Input Mixer Boost' on
amixer -c 0 sset 'Left Input Mixer Boost' on
```

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

### Result on 2026-07-31

```text
control.wav (card idle)      peak 1274   floor 71   spikes 1
loaded.wav  (card hammered)  peak  395   floor 57   spikes 0
```

No spikes at all while writing hard to the card, and the loaded take carries a
*lower* noise floor than the idle one. Write current coupling into the front
end would raise it, not lower it. The single spike in the control take was
room noise.

The tick that the ESP32 build could only mask, at a cost of 3.4% of every
recording, does not occur here. The premise of the migration holds, and the
mute machinery does not need porting.

One caveat worth keeping: this ran at modest capture gain, +12 dB on the input
PGA and 0 dB on the ADC, not the gain the finished recorder will use. Repeat
the comparison once the application records at production levels.

## 7. Wire the hook switch

The HAT claims GPIO 2 and 3 for I2C, GPIO 18 to 21 for I2S, and GPIO 0 and 1
for its ID EEPROM. Confirm with `pinctrl get 0-27` before choosing a pin
rather than trusting a pinout diagram, since audio HATs sometimes take extra
GPIOs for amplifier enable or mute.

GPIO 17 is free. Wire the cradle switch between:

- **Physical pin 11**, GPIO 17, signal
- **Physical pin 9**, ground

They are adjacent on the same row, which keeps it to a clean two-wire tap. The
switch is a passive mechanical contact, so the wires are interchangeable.

Isolate both wires from the original telephone PCB. Nothing from the old line
circuit should reach a 3.3 V input.

### Measured behaviour

Enable the internal pull-up (`pinctrl set 17 ip pu`, or `pull_up=True` in
gpiozero) and the pin idles high.

| Handset | GPIO 17 |
| --- | --- |
| On cradle | LOW (switch closed to ground) |
| Lifted | HIGH (switch open, pull-up) |

This matches the low-idle, high-off-hook signal characterised during the ESP32
build, measured independently here.

Sampling the pin in a tight loop across several deliberate lift and replace
cycles produced **seven transitions and no bounce at all**, every edge
separated by more than a second. The loop samples in the tens of kHz, so
millisecond-scale chatter would have appeared.

No external pull-up or debounce capacitor is required. Keep roughly 100 ms of
software debounce anyway: the measurement used slow deliberate movements, and
a guest dropping the handset onto the cradle is a harsher test than anything
recorded here.
