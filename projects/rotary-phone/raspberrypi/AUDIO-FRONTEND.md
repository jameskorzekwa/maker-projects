# Audio Front End

Why the WM8960 HAT cannot capture this handset, what working projects use
instead, and what is actually known about making a telephone handset
microphone sound good.

This exists because a long session was spent trying to make a modified WM8960
HAT record a handset microphone, and almost none of that difficulty was
necessary. The conclusions here are drawn from kernel source, measurements on
this build, and other people's published results.

## 1. The WM8960 HAT is the wrong board, structurally

Two independent reasons, either of which is disqualifying.

### It has no microphone input

Waveshare's own description of the board lists dual onboard MEMS microphones,
an earphone jack, and speaker terminals. There is no external microphone
input. Removing `L3`, tapping `C9` for MICBIAS, and injecting through `C14`
was manufacturing an input path the board was never designed to have.

### Its Linux driver cannot power microphone bias

In `sound/soc/codecs/wm8960.c`, the string `MICB` appears exactly once:

```c
SND_SOC_DAPM_SUPPLY("MICB", WM8960_POWER1, 1, 0, NULL, 0),
```

That is the widget declaration. There is no DAPM route making any input widget
depend on it, and no mixer control exposing it. DAPM powers supply widgets only
when something in the audio path requires them, so an unreferenced supply is
never enabled. **MICBIAS cannot be turned on from userspace on this codec.**
Not with `amixer`, not with a stored ALSA state, not with a device tree
property. Only a driver patch or an external bias supply.

This is not a Waveshare fault. It is a property of the upstream driver.

Compare two codecs that do it correctly:

```c
/* da7213.c - the DA7212 on the Raspberry Pi Codec Zero */
{"MIC1", NULL, "Mic Bias 1"},
{"MIC2", NULL, "Mic Bias 2"},

/* wm8731.c - the Audio Injector Zero */
{"Input Mux", "Mic", "Mic Bias"},
{"Mic Bias", NULL, "MICIN"},
```

In both, the input widgets depend on the bias supply, so opening a capture
stream powers it automatically.

Of the three candidate codecs, this build used the only one whose driver
structurally cannot do what the project needs.

## 2. What working projects actually use

Every Raspberry Pi audio guestbook found uses a cheap USB audio adapter. None
use an I2S HAT.

The dominant project is
[nickpourazima/rotary-phone-audio-guestbook][guestbook-repo], which runs the
same OS as this build. Its entire audio bill of materials is a USB audio
adapter and an OTG cable, about eight dollars. Searching that project's issues
for I2S HATs returns nothing; the maintainer's troubleshooting is always
USB-shaped, and users report success with generic C-Media and Realtek dongles.

A USB Audio Class device needs no driver, no overlay, and no kernel module
compiled on a 1 GHz ARMv6 core. More importantly, a C-Media class dongle puts
roughly 4.5 V of microphone bias on its input jack **in hardware,
unconditionally**, with no driver involvement. That is exactly the thing the
WM8960 cannot do.

It also moves the entire analog front end off the Pi's circuit board and away
from its Wi-Fi antenna, which is worth something given section 5.

## 3. Microphone options, ranked

<!-- markdownlint-disable MD013 -->

| Option | Cost | Notes |
| --- | --- | --- |
| Original handset capsule into a USB adapter | free | Untested here at 4.5 V bias. Try first: it costs nothing and the bias is far better than what this build was giving it. See section 4. |
| Telephone transmitter capsule, e.g. EMS-94 | ~$8 | Purpose built for handsets: 1.5-10 V operating range, 1500 ohm output impedance, drops into the mouthpiece. Used successfully by a builder who could not get any lavalier working ([issue #81][issue-81]). |
| Lavalier microphone | ~$20 | What most successful builds use. Must be omnidirectional, 3.5 mm TRS, and sold for PC or camera use rather than for a specific wireless transmitter. |
| Adafruit MAX4466 preamp | ~$7 | **Do not.** No successful project uses an external op-amp preamp. See section 6. |

<!-- markdownlint-enable MD013 -->

Note that this handset's capsule is **not** a carbon transmitter. It measures
about 0.86 kohm one way and 1.93 kohm reversed and responds to speech, which
is a two-wire electret with an internal JFET. Most telephone-hacking material
concerns carbon transmitters and does not apply.

Its measured output impedance is close to the EMS-94's 1500 ohm specification,
which suggests it may already be a telephone-grade transmitter rather than a
generic electret.

## 4. Bias and headroom: the measurement nobody takes

An electret's internal JFET should be biased so its drain sits near half the
supply voltage, which is where it has symmetric room to swing. Bias it too low
and it clips early on one polarity, producing soft saturation rather than hard
clipping.

**This build had it badly wrong and never checked.** Measured on the ESP32
wiring: MICBIAS about 2.9 V, through 2.2 kohm, with the capsule node sitting at
**0.9 V**. Half of 2.9 V is 1.45 V. The capsule was starved and asymmetric,
with very little room to swing down, which is a textbook cause of the early
distortion that dominated testing.

The same mistake, and its symptoms, are documented by the live-recording
community:

> You get better loud signal handling capacity and lower distortion using a
> battery box than with plug-in-power inputs... the recorder provides 2.2V via
> 6.8k for the mic, which is simply nowhere near enough.
> -- [megalithia battery box][megalithia]

Choose the series resistor so the drain sits near half the supply. For a
capsule drawing 0.3-0.5 mA from 4.5 V that is roughly 4.7k-6.8k; for one
drawing closer to 1 mA it is nearer 2.2k. The correct value depends on the
capsule, which is why it must be measured rather than copied.

**Diagnostic: with the capsule connected and powered, measure the DC voltage
across it. It should be near half the bias voltage.** No published guestbook
build appears to have done this.

Raising the supply voltage fixes electrical starvation and clipping. It does
**not** raise the capsule's acoustic overload point, which is set by the
diaphragm and the JFET's own limits. Treat those as separate budgets.

## 5. Acoustic overload is real and quantifiable

A handset puts the microphone one to two centimetres from the mouth. Using the
ITU telephony reference, normal speech at handset distance is about 90 dB SPL
active level, and speech peaks run 12-18 dB above that, so **102-108 dB SPL
peaks for a normal talker** and 112-118 dB for a loud one.

Generic electret capsules are typically acceptable to around 100 dB SPL, with
distortion rising quickly above it ([Elliott Sound Products][elliott]).

So a cheap electret at handset distance is marginal for normal speech and
overloaded for loud speech. The distortion observed on this build when talking
close is physically expected, not a wiring fault.

Mitigations, in order of documented effectiveness:

1. Correct the bias point first, since it is cheap and was wrong here.
2. Mount the capsule deeper in the mouthpiece chamber and keep the original
   perforated grille. Telephone mouthpieces are engineered acoustic chambers,
   and one builder's single largest improvement was discovering the capsule was
   **facing the wrong way**.
3. Use a capsule intended for telephone use, which is designed for this SPL.
4. Limit or compress in software rather than chasing the last few dB in analog.

One myth to discard: the muffled quality when talking close is **not**
proximity effect. Handset capsules are omnidirectional and omnidirectional
microphones have no proximity effect. It is distortion products plus cavity
acoustics, which means equalisation alone will not fix it.

## 6. The MAX4466 was a mistake

An Adafruit MAX4466 was added to boost a quiet capsule. In hindsight the
quietness was caused by the ESP32 firmware's gain structure and the starved
bias described above, not by the capsule needing a preamp.

It caused three problems of its own:

- Its minimum gain is 25x, which is more than a close-talked electret needs, so
  there was no useful setting.
- It **oscillated intermittently** when moved to a 5 V supply. A recording made
  in that state has a narrow 4136 Hz tone with a quiet-passage RMS of 4062,
  against 191-270 for normal recordings. An amplifier drifting in and out of
  oscillation produces measurements that do not repeat, which wasted a great
  deal of debugging time.
- The fix for that oscillation is a 0.1 uF ceramic capacitor directly across
  its supply pins, which was never fitted. Long supply leads to an op-amp
  without local decoupling are a well known cause.

No published guestbook project uses an external op-amp preamp. A USB adapter's
own microphone input has enough gain.

## 7. Wi-Fi interference

Recordings on this build contain a tick at roughly 102 ms intervals, which is
the standard Wi-Fi beacon interval of 100 time units. Disabling the radio
removes it: the noise floor falls from 1434 to 331 RMS and the periodicity
correlation from 0.669 to 0.022.

It survived an isolated battery supply for the preamp and a move of the preamp
ground to the codec's analog ground, so it is radiated rather than conducted.

Mitigations: a USB adapter physically separates the analog front end from the
Pi's antenna; shielded microphone cable is what the most successful comparable
build used; and a small capacitor across the microphone input shunts radio
frequency energy before it can be rectified.

## 8. Diagnostics that worked, and one that misled

Worth recording because two of these cost hours.

**Measure where clipping occurs, not just that it occurs.** Flat-topped peaks
that cluster at a single amplitude indicate clipping. Whether that amplitude
**moves when a gain control is changed** tells you which side of that control
the clipping happens on. A fixed ceiling means the stage after it; a ceiling
that scales means the stage before.

**Peak occupancy is not a clipping test.** Counting samples near digital full
scale misses soft saturation entirely. A recording with a crest factor of 1.7,
which is nearly a square wave, was reported as clean by that method.

**Grounding an input proves less than it appears.** Shorting a codec input to
ground silenced the recording, which was read as proof the noise arrived from
upstream. It proves no such thing: it also shorts out anything induced on the
interconnect, and a purely digital artefact would look identical because a
discontinuity in a stream of zeros is still zeros. Several later experiments
were designed inside that false conclusion.

## References

- [nickpourazima/rotary-phone-audio-guestbook][guestbook-repo]
- [Its hardware notes, and why the original capsule was
  abandoned][hardware-md]
- [A builder who used an EMS-94 telephone capsule instead of a
  lavalier][issue-81]
- [playfultechnology/audio-guestbook, the Teensy original][teensy-repo]
- [Elliott Sound Products on electret capsules and their SPL limits][elliott]
- [Megalithia on electret bias starvation][megalithia]
- [Raspberry Pi Codec Zero, the only I2S HAT with a real electret
  input][codec-zero]

[guestbook-repo]: https://github.com/nickpourazima/rotary-phone-audio-guestbook
[hardware-md]: https://github.com/nickpourazima/rotary-phone-audio-guestbook/blob/main/docs/hardware.md
[issue-81]: https://github.com/nickpourazima/rotary-phone-audio-guestbook/issues/81
[teensy-repo]: https://github.com/playfultechnology/audio-guestbook
[elliott]: https://sound-au.com/articles/mic-electret.htm
[megalithia]: http://www.megalithia.com/elect/battbox.html
[codec-zero]: https://www.raspberrypi.com/documentation/accessories/audio.html
