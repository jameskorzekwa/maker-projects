# Technical Reference

This document records the electrical details, measurements, and complete test
plan. It is not the starting point. Follow the plain-language
[build guide](BUILD.md) one checkpoint at a time.

The repository does not contain firmware yet. Hardware assembly can proceed
through the documented checkpoints. Photos identify the controller as a Seeed
Studio XIAO ESP32-C6 and confirm all exposed pin labels, so the GPIO table is
now complete.

## Safety

- Unplug the telephone line cord before opening the phone.
- Never reconnect the modified phone to a telephone wall socket. Ringing
  voltage can exceed 70 VAC.
- Remove or physically block the telephone-line jack in the final assembly.
- Only route isolated, regulated 5 V power into the completed phone.
- Disconnect all power before continuity or resistance measurements.
- Do not connect an unidentified handset pair to the audio board.
- Do not connect either negative BTL speaker output to ground.
- Stop if a board becomes hot, draws unexpected current, smells, or produces
  smoke.

## Tools and Supplies

- Digital multimeter with continuity, resistance, and DC-voltage modes
- Temperature-controlled soldering iron with a fine tip
- Fine solder, flux, solder wick, tweezers, and magnification
- Small screwdrivers, wire cutters, and wire strippers
- Heat-shrink tubing and insulated stranded hookup wire
- 2.2 kohm resistor for the handset electret microphone bias
- 10 kohm resistor and 100 nF capacitor for the hook-switch input
- 3.5 mm TRS plug or sacrificial stereo audio cable for the handset earpiece
- Perfboard, connectors, standoffs, and strain relief for final assembly
- Computer with a USB data cable and microSD reader
- Optional current-limited bench supply for initial testing

Do not use loose jumper wires in the final enclosure.

## 1. Inventory and Photograph Everything

Confirm that these parts are present:

- Dyna-Living telephone, model `Dyna-JJ0TOP12254-FBA`
- Seeed Studio XIAO ESP32-C6
- Waveshare WM8960 Audio HAT, sold through Amazon ASIN `B098R7TTM4`
- [Adafruit MicroSD Card Breakout Board+ product 254](https://www.adafruit.com/product/254),
  Amazon ASIN `B00NAY2NAI`
- High-endurance microSD card
- Regulated 5 V power supply
- Included Waveshare test speaker

The received photographs record:

1. Both sides of the XIAO ESP32-C6, including all exposed pin labels.
2. Both sides of the Waveshare board, including every silkscreen label.
3. The unopened phone base and all external connectors.
4. The phone base immediately after opening it, before disconnecting wires.
5. Both handset capsules, their markings, terminals, and wire colors.

Record board revisions and connector labels in the worksheet at the end of
this file. Compare the Waveshare board with the
[Waveshare reference schematic](https://files.waveshare.com/upload/f/fa/WM8960_Audio_HAT_Schematic.pdf).
Stop if the microphone input around `MIC1`, `L3`, and `C14` does not match the
reference design.

## 2. Make the Original Phone Safe

1. Unplug the line cord and handset cord.
2. Open the phone base without pulling or cutting wires.
3. Photograph the original PCB and every connector.
4. Label the handset jack, line jack, hook switch, and rotary-dial wires.
5. Disconnect the line jack from the circuitry or remove it completely.
6. Verify with continuity mode that neither line-jack contact connects to the
   handset jack, hook switch, new electronics, or enclosure metal.
7. Leave the rotary dial disconnected for the first version.

Do not remove the original PCB until the handset jack and hook-switch wiring
have been traced. It may be useful as a mechanical mounting reference even
though its telephone circuit will not be powered.

## 3. Map the Handset

Number the four handset contacts `H1` through `H4` from left to right while
looking directly into the base jack with its latch opening down. Record this
orientation with a photograph; reversing the viewing direction reverses the
numbers.

The received 4P4C cord and base jack use mirrored color orders. This is a normal
plug/jack orientation and must not be rewired:

| Coiled-cord wire | Base-jack wire |
| --- | --- |
| Yellow | Black |
| Green | Red |
| Red | Green |
| Black | Yellow |

Measurements and direct inspection establish:

| Function | Coiled-cord contacts and wires | Base-jack wires | Resistance |
| --- | --- | --- | ---: |
| Earpiece | `1-4`, black/yellow | Black/yellow | 129.3 ohms |
| Microphone | `2-3`, red/green | Red/green | Directional |

The extra red/black pair on the small switch PCB is a separate base circuit and
is not part of either handset pair. Leave it untouched.

The microphone photo and directional resistance test are consistent with a
two-wire electret-style microphone. With the red meter probe on coiled-cord
contact `2`/red and black probe on contact `3`/green, it measures approximately
0.858 kohm. Reversing the probes produces approximately 1.93 kohm, and the
reading changes slightly in response to speech. Use this polarity:

| Location | `MIC+` | `MIC-` |
| --- | --- | --- |
| Coiled cord | Contact `2`, red | Contact `3`, green |
| Base jack | Green | Red |

## 4. Map the Hook Switch

Photos of the received phone confirm a mechanically actuated, multi-contact
hook switch mounted on a small PCB. Four white wires run from this assembly to
a dedicated four-pin plug on the original main board. Reuse the mechanical
assembly, but disconnect its plug from the telephone electronics before testing
or attaching it to the ESP32.

Perform these tests with the telephone line and all power disconnected:

1. Unplug the four-wire hook-switch connector from the original main board.
2. Mark one edge of the cable plug with tape or a permanent marker.
3. Photograph the marked plug in a fixed orientation and label its contacts
   `S1` through `S4` from left to right.
4. Set the multimeter to continuity or its lowest resistance range.
5. Measure all six contact pairs with the handset resting on the cradle.
6. Repeat all six measurements with the handset lifted.

Record `OL` for an open circuit or the measured resistance for a closed circuit:

| Pair | Handset down | Handset lifted |
| --- | --- | --- |
| `S1-S2` | `OL` | 0.8 ohm |
| `S1-S3` | `OL` | `OL` |
| `S1-S4` | `OL` | `OL` |
| `S2-S3` | `OL` | `OL` |
| `S2-S4` | 0.8 ohm | `OL` |
| `S3-S4` | `OL` | 0.5 ohm |

Select one pair that measures below 2 ohms in one cradle position and `OL` in
the other. Move the cradle repeatedly while watching the meter to confirm that
the pair changes reliably. Either electrical orientation is usable:

- A pair closed with the handset down makes the GPIO low when idle and high
  when lifted.
- A pair closed with the handset lifted makes the GPIO low when lifted; invert
  the state in firmware.

Use `S2-S4` for this build. It is closed with the handset down and open with the
handset lifted, producing the preferred low-idle/high-off-hook signal with the
pull-up circuit below. The switch is a changeover network: when lifted, `S1-S2`
and `S3-S4` close. This is harmless when unused `S1` and `S3` are individually
insulated and left electrically floating.

After selecting the pair:

1. Keep the four-pin harness permanently disconnected from the original main
   board.
2. Connect `S2` to the ESP32 hook GPIO node.
3. Connect `S4` to ESP32 ground.
4. Individually insulate unused `S1` and `S3`.
5. Verify that none of the four switch conductors has continuity to the line
   jack, handset conductors, original PCB, or metal enclosure.

If no pair switches cleanly and repeatably, install a separate microswitch under
the cradle instead of sharing or probing the original powered circuitry.

The hook-switch circuit uses XIAO `D7`, which is ESP32-C6 GPIO17:

```text
XIAO 3V3 --- 10 kohm ---+--- XIAO D7
                         |
                         +--- 100 nF --- XIAO GND
                         |
                         +--- S2 switch S4 --- XIAO GND
```

Debounce the signal in firmware for 100 to 200 ms. A stable transition to the
lifted state starts prompt playback; a stable transition to the replaced state
stops recording and finalizes the WAV file.

## 5. Verify the Audio Board Before Modifying It

The delivered board is a Waveshare-branded WM8960 Audio HAT and visually
matches the published layout. It uses this Raspberry Pi header assignment:

| Audio HAT signal | Raspberry Pi physical pin | XIAO connection |
| --- | ---: | --- |
| 5 V | 2 or 4 | `VBUS` |
| GND | 6 or another ground pin | `GND` |
| SDA | 3 | `D4`, GPIO22 |
| SCL | 5 | `D5`, GPIO23 |
| I2S CLK | 12 | `D0`, GPIO0 |
| I2S LRCLK | 35 | `D1`, GPIO1 |
| I2S ADC | 38 | `D2`, GPIO2 |
| I2S DAC | 40 | `D6`, GPIO16 |

With the Raspberry Pi header along the bottom edge and the component labels
upright, `MIC2` is the left onboard microphone and `MIC1` is the right onboard
microphone. Modify the right-side `MIC1` channel for the handset. The 3.5 mm
jack is output-only. The screw-terminal outputs are labeled left to right as
`LP`, `LN`, `RN`, and `RP`; `LN` and `RN` are driven outputs, not ground.

In this same orientation, Raspberry Pi header pins 1 and 2 are the far-right
pair below `MIC1`, beside the button. Pin 1 is in the row closer to the board
components; pin 2 is directly below it in the row closer to the board edge.
Numbering continues right-to-left, with odd pins in the component-side row and
even pins in the edge-side row. The far-left pair by the earphone jack is pins
39 and 40, not pins 1 and 2.

Confirm these connections against the delivered PCB before use. The Audio HAT
is powered from 5 V but uses 3.3 V logic. It contains a 24 MHz oscillator, so
the Raspberry Pi header does not require an ESP32 MCLK connection.

Check whether SDA and SCL already have pull-up resistors to 3.3 V. If they do
not, add one 4.7 kohm pull-up from SDA to 3.3 V and another from SCL to 3.3 V.
Do not allow either I2C signal to be pulled up to 5 V.

Power the XIAO through USB-C. Its documented `VBUS` pad is the single 5 V rail
for the Audio HAT and Adafruit board. Use a common `GND` rail. Do not connect an
external 5 V source while USB-C is connected.

Before modifying `MIC1`:

1. Inspect for solder bridges and shipping damage.
2. Measure resistance between 5 V and ground; stop on a near short.
3. Connect I2C, I2S, 5 V, and ground using the verified GPIO worksheet.
4. Connect only the included speaker to the marked speaker connector.
5. Flash the audio bring-up firmware when it is available.
6. Record five seconds with an onboard microphone and play it back.
7. Confirm clean audio, stable power, and normal board temperature.

Do not alter the board unless both stock recording and playback work.

The temporary Checkpoint 4B firmware is implemented as the local ESPHome
external component `esphome/components/wm8960_audio_test/`. It uses ESP-IDF's
standard full-duplex I2S driver with the XIAO as clock controller and the
WM8960 as peripheral. The HAT's onboard 24 MHz oscillator supplies MCLK, so no
MCLK wire is connected to the XIAO. The one-shot test uses 16 kHz, 16-bit I2S,
keeps the class-D gain low, records one onboard microphone into RAM for four
seconds, plays the recording through both supplied speakers, and then powers
down the codec outputs. This is diagnostic firmware, not the final guestbook
audio implementation.

## 6. Adapt the Original Handset Microphone

The handset's directional resistance and response to speech are consistent with
a two-wire electret-style microphone. Proceed only after the unmodified Audio
HAT passes its stock test and its input matches the reference schematic.

The base-jack microphone pair is green `MIC+` and red `MIC-`. Isolate both from
the original PCB before attaching them to the Audio HAT.

The reference path is:

```text
MIC1 DAT --- L3 --- C14 (10 uF) --- WM8960 RINPUT1
```

The recommended modification is:

```text
WM8960 MICBIAS --- 2.2 kohm --- microphone positive
                                          |
                                          +--- MIC1 DAT-side signal pad

Audio HAT GND -------------------- microphone negative
```

1. Disconnect all power.
2. Use continuity mode to identify the non-ground side of `C9`/`C10`; this is
   the reference-design `MICBIAS` node. Verify it against the schematic.
3. Remove `MIC1` or isolate only its `DAT` output. Do not leave its output
   driving the external microphone node.
4. Confirm that the selected input pad still connects through `L3` and `C14`
   to WM8960 `RINPUT1`.
5. Solder a 2.2 kohm resistor from `MICBIAS` to base-jack green `MIC+`.
6. Connect base-jack green `MIC+` to the isolated `MIC1 DAT` input path.
7. Connect base-jack red `MIC-` to Audio HAT ground.
8. Add strain relief so handset movement cannot pull an SMD pad from the board.
9. Inspect under magnification and clean flux residue.

Before powering, verify:

- 5 V is not shorted to ground.
- 3.3 V is not shorted to ground.
- `MICBIAS` is not shorted to ground or 3.3 V.
- The former `MIC1 DAT` output is isolated from the input node.
- The microphone pair is isolated from the original telephone PCB.

Enable WM8960 `MICBIAS` in firmware before judging microphone operation. Start
with low input gain, record speech, then increase gain until normal speech is
clear without clipping or excessive noise.

Stop if the handset uses a carbon transmitter. It needs a different bias and
preamplifier circuit; do not connect it to this input.

## 7. Connect the Original Earpiece

The measured earpiece resistance is 129.3 ohms. Use one channel of the 3.5 mm
headphone output; do not use the BTL speaker terminals.

The base-jack earpiece pair is yellow and black. Isolate both wires from the
original telephone PCB. Connect yellow to the 3.5 mm plug's tip and black to its
sleeve. Leave the ring unused. Polarity does not affect a single mono earpiece,
but this convention keeps black as the return wire.

Never connect `LN` or `RN` to ground. They are driven BTL outputs and are not
used for this earpiece.

1. Set firmware output volume to its minimum.
2. Play a quiet test tone or spoken prompt.
3. Increase volume gradually while listening for distortion or heating.
4. Use the headphone output unless testing proves that the speaker output is
   necessary.
5. Add strain relief to the earpiece wires.

## 8. Connect the Adafruit 254 microSD Board

Format the high-endurance card as FAT32 and test it in a computer first. The
Adafruit 254 board includes a regulator and logic-level shifting. Use its `5V`
power input with this 5 V system; leave its `3V` pin unused. Wire it only after
using the assigned XIAO SPI pins:

| Adafruit label | Meaning | XIAO connection |
| --- | --- | --- |
| `5V` | Board power input | `VBUS` |
| `3V` | Regulated 3.3 V | Leave disconnected |
| `GND` | Ground | `GND` |
| `CLK` | SPI clock | `D8`, GPIO19 |
| `DO` | Data out from card | `D9`, GPIO20/MISO |
| `DI` | Data into card | `D10`, GPIO18/MOSI |
| `CS` | Card select | `D3`, GPIO21 |
| `CD` | Optional card detect | Leave disconnected for initial build |

Use short wires and keep them away from the microphone pair. Do not substitute
the pin names from a different microSD module. The label `DO` maps to ESP32
`MISO`; `DI` maps to ESP32 `MOSI`.

After storage firmware exists:

1. Mount the card.
2. Create and read back a test file.
3. Record at least five minutes of PCM audio without write errors.
4. Remove power during a disposable test recording and verify boot-time file
   recovery.

## 9. Complete the Bench Prototype

Keep all parts outside the phone until this sequence passes:

1. Power on and mount microSD.
2. Simulate off-hook with the isolated hook switch.
3. Play the instruction prompt through the handset earpiece.
4. Play the beep.
5. Record through the original handset microphone.
6. Replace the handset and verify that the WAV file closes cleanly.
7. Play the WAV file on a computer.
8. Repeat ten times without resetting the ESP32.

Fix noise, gain, power, and file-handling problems on the bench. Do not install
an unreliable prototype into the phone.

## 10. Install in the Phone

1. Remove or block the telephone-line jack.
2. Mount the ESP32, Audio HAT, and microSD breakout on standoffs or a rigid
   carrier.
3. Keep the Audio HAT close to the handset jack.
4. Twist the microphone pair and route it away from USB, SPI, Wi-Fi antenna,
   and switching power wires.
5. Keep the ESP32 antenna clear of metal and wiring.
6. Make the microSD card accessible without disturbing soldered connections.
7. Add strain relief to power and handset wiring.
8. Insulate every exposed joint with heat-shrink tubing or a rigid cover.
9. Secure all connectors so vibration cannot loosen them.
10. Verify that the cradle and rotary mechanism cannot strike any new wiring.
11. Close the enclosure without pinching cables.

Before final power-up, repeat all rail-to-ground resistance checks and confirm
that the line jack remains electrically isolated.

## 11. Final Acceptance Tests

Complete these tests before relying on the phone at the wedding:

1. Perform 100 lift, prompt, beep, record, and hang-up cycles.
2. Confirm 100 unique, playable files.
3. Test rapid cradle movement and switch bounce.
4. Hang up during the prompt and during the beep.
5. Record one full five-minute message.
6. Test a missing, full, and read-only card.
7. Remove power during a disposable recording and verify recovery.
8. Run an eight-hour powered soak test.
9. Test near loud music and conversation.
10. Confirm that microphone gain does not clip normal or loud speech.
11. Verify that Wi-Fi failure never prevents local recording.

Do not deploy the phone until every completed normal call creates exactly one
playable WAV file and previously completed files survive power interruption.

## Build Worksheet

The controller and GPIO entries below are confirmed from the board photographs
and Seeed's official pin map. The handset capsule measurements remain open.

<!-- markdownlint-disable MD013 -->

| Measurement | Result |
| --- | --- |
| ESP32 board | Seeed Studio XIAO ESP32-C6 |
| Audio board identity | Waveshare WM8960 Audio HAT; no revision marking visible |
| Audio board matches Waveshare schematic | Visual layout confirmed; electrical and stock functional tests pending |
| Handset connector orientation photo | Received; `H1` through `H4` not yet assigned |
| Earpiece contacts | Coiled cord `1-4` black/yellow; base jack black/yellow |
| Earpiece resistance | 129.3 ohms; use 3.5 mm headphone output |
| Microphone contacts | Coiled cord `2-3` red/green; base jack red/green |
| Microphone type | Electrical behavior consistent with a two-wire electret-style microphone |
| Microphone positive contact | Coiled red/contact `2`; base-jack green |
| Microphone negative contact | Coiled green/contact `3`; base-jack red |
| Hook-switch assembly | Four-wire mechanical assembly confirmed; harness disconnects from main PCB |
| Hook-switch contacts | `S2` to GPIO node, `S4` to ground; insulate `S1` and `S3` |
| Switch closed when | Handset down: 0.8 ohm; lifted: `OL` |
| ESP32 SDA GPIO | `D4`, GPIO22 |
| ESP32 SCL GPIO | `D5`, GPIO23 |
| ESP32 I2S CLK GPIO | `D0`, GPIO0 |
| ESP32 I2S LRCLK GPIO | `D1`, GPIO1 |
| ESP32 I2S input GPIO | `D2`, GPIO2 |
| ESP32 I2S output GPIO | `D6`, GPIO16 |
| ESP32 microSD SCK GPIO | `D8`, GPIO19 |
| ESP32 microSD MOSI GPIO | `D10`, GPIO18 |
| ESP32 microSD MISO GPIO | `D9`, GPIO20 |
| ESP32 microSD CS GPIO | `D3`, GPIO21 |
| ESP32 hook-switch GPIO | `D7`, GPIO17 |
| 5 V power distribution method | USB-C input to XIAO; `VBUS` to Audio HAT and Adafruit `5V` |

<!-- markdownlint-enable MD013 -->

## References

- [Seeed Studio XIAO ESP32-C6 documentation](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [ESP-IDF stable ESP32-C6 documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/)
- [Adafruit product 254](https://www.adafruit.com/product/254)
- [Adafruit product 254 wiring guide](https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-wiring)
- [Waveshare WM8960 Audio HAT](https://www.waveshare.com/wiki/WM8960_Audio_HAT)
- [Waveshare WM8960 Audio HAT schematic](https://files.waveshare.com/upload/f/fa/WM8960_Audio_HAT_Schematic.pdf)
