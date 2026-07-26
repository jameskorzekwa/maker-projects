# Build Instructions

These instructions take the project from unopened components through a tested
bench prototype and permanent installation. Complete the stages in order. Do
not make permanent modifications until the stock audio board has passed its
record and playback tests.

The repository does not contain firmware yet. Hardware assembly can proceed
through the documented checkpoints, but the final GPIO table and flashing
commands must be added after the received ESP32-C5 board is photographed and
its exposed pins are verified.

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
- ESP32-C5 Mini marked `ESP32-C5_MINI_V1.0`
- BFab WM8960 Audio HAT, Amazon ASIN `B098R7TTM4`
- Adafruit-compatible 3.3 V SPI microSD breakout
- High-endurance microSD card
- Regulated 5 V power supply
- Included BFab test speaker

Before connecting anything, take readable photographs of:

1. Both sides of the ESP32-C5 board.
2. Both sides of the BFab board, including every silkscreen label.
3. The unopened phone base and all external connectors.
4. The phone base immediately after opening it, before disconnecting wires.
5. Both handset capsules, their markings, terminals, and wire colors.

Record board revisions and connector labels in the worksheet at the end of
this file. Compare the BFab board with the
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

A conventional 4P4C handset normally uses the center pair for the earpiece and
the outer pair for the microphone. Treat this only as a clue.

1. Open both ends of the handset.
2. Use continuity mode to map each capsule terminal to `H1` through `H4`.
3. Mark the earpiece pair and microphone pair in the worksheet.
4. Measure the earpiece resistance with the handset disconnected.
5. Identify the microphone part number and construction.
6. If it is a two-wire electret capsule, identify its positive and negative
   terminals from markings or the original PCB, not wire color alone.

Expected result: two isolated earpiece wires and two isolated microphone wires.
Stop if the handset has fewer than four active conductors, shared grounds,
active electronics, or an unidentified capsule.

## 4. Map the Hook Switch

Perform these tests with the original phone unpowered and disconnected:

1. Identify the switch terminals actuated by the handset cradle.
2. Measure every terminal pair with the handset lifted and replaced.
3. Select a pair that is open in one state and below 2 ohms in the other.
4. Disconnect that pair from the original telephone PCB.
5. Verify that both selected terminals are isolated from every line-jack and
   handset contact.
6. Record whether the pair closes when the handset is lifted or replaced.

If no isolated pair is available, install a small microswitch under the cradle
instead of sharing the original telephone circuit.

The eventual GPIO circuit is:

```text
3.3 V --- 10 kohm ---+--- ESP32 hook GPIO
                     |
                     +--- 100 nF --- GND
                     |
                     +--- hook switch --- GND
```

Choose the final GPIO only after the ESP32 pinout is verified. Avoid ESP32-C5
boot-strapping GPIO2 and GPIO7.

## 5. Verify the Audio Board Before Modifying It

The BFab board should match the Waveshare WM8960 Audio HAT pin assignment:

| BFab signal | Raspberry Pi physical pin | Direction relative to ESP32 |
| --- | ---: | --- |
| 5 V | 2 or 4 | Power input |
| GND | 6 or another ground pin | Common ground |
| SDA | 3 | ESP32 to/from codec |
| SCL | 5 | ESP32 to codec |
| I2S CLK | 12 | ESP32 to codec |
| I2S LRCLK | 35 | ESP32 to codec |
| I2S ADC | 38 | Codec to ESP32 |
| I2S DAC | 40 | ESP32 to codec |

Confirm these connections against the delivered PCB before use. The BFab board
is powered from 5 V but uses 3.3 V logic. It contains a 24 MHz oscillator, so
the Raspberry Pi header does not require an ESP32 MCLK connection.

Check whether SDA and SCL already have pull-up resistors to 3.3 V. If they do
not, add one 4.7 kohm pull-up from SDA to 3.3 V and another from SCL to 3.3 V.
Do not allow either I2C signal to be pulled up to 5 V.

Do not create two competing 5 V power paths. During programming, either power
the system from the ESP32 USB connection and a verified 5 V/VBUS pin, or use a
single external 5 V rail with a common ground. Determine which method is safe
from the delivered ESP32 board before connecting the HAT.

Before modifying `MIC1`:

1. Inspect for solder bridges and shipping damage.
2. Measure resistance between 5 V and ground; stop on a near short.
3. Connect I2C, I2S, 5 V, and ground using the verified GPIO worksheet.
4. Connect only the included speaker to the marked speaker connector.
5. Flash the audio bring-up firmware when it is available.
6. Record five seconds with an onboard microphone and play it back.
7. Confirm clean audio, stable power, and normal board temperature.

Do not alter the board unless both stock recording and playback work.

## 6. Adapt the Original Handset Microphone

Proceed only if the handset microphone is confirmed to be a two-wire electret
capsule and the delivered BFab input matches the reference schematic.

The reference path is:

```text
MIC1 DAT --- L3 --- C14 (10 uF) --- WM8960 RINPUT1
```

The recommended modification is:

```text
WM8960 MICBIAS --- 2.2 kohm --- microphone positive
                                          |
                                          +--- MIC1 DAT-side signal pad

BFab GND ------------------------- microphone negative
```

1. Disconnect all power.
2. Use continuity mode to identify the non-ground side of `C9`/`C10`; this is
   the reference-design `MICBIAS` node. Verify it against the schematic.
3. Remove `MIC1` or isolate only its `DAT` output. Do not leave its output
   driving the external microphone node.
4. Confirm that the selected input pad still connects through `L3` and `C14`
   to WM8960 `RINPUT1`.
5. Solder a 2.2 kohm resistor from `MICBIAS` to the handset microphone positive
   wire.
6. Connect that positive wire to the isolated `MIC1 DAT` input path.
7. Connect the handset microphone negative wire to BFab ground.
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

Choose the output only after measuring earpiece resistance:

- For approximately 16 ohms or higher, begin with the 3.5 mm headphone output.
  Connect one channel, such as tip/left, and sleeve/ground.
- For an approximately 8 ohm dynamic earpiece, use one BTL speaker channel,
  such as `LP` and `LN`.
- For an open circuit, very low resistance, or an unidentified capsule, stop
  and inspect the handset before connecting it.

Never connect `LN` or `RN` to ground. They are driven BTL outputs.

1. Set firmware output volume to its minimum.
2. Play a quiet test tone or spoken prompt.
3. Increase volume gradually while listening for distortion or heating.
4. Use the headphone output unless testing proves that the speaker output is
   necessary.
5. Add strain relief to the earpiece wires.

## 8. Connect microSD

Format the high-endurance card as FAT32 and test it in a computer first. Wire
the breakout only after selecting safe, exposed ESP32 GPIOs:

| microSD signal | ESP32-C5 connection |
| --- | --- |
| 3.3 V | `TBD 3V3 pin after board verification` |
| GND | `GND` |
| SCK | `TBD` |
| MOSI | `TBD` |
| MISO | `TBD` |
| CS | `TBD` |

Use short wires and keep them away from the microphone pair. Confirm the
breakout's exact power-input requirements before powering it. Some breakouts
have a `VIN` regulator and level shifting, while others expose only a `3V3`
input. Never connect 5 V to a pin marked `3V3`.

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
2. Mount the ESP32, BFab board, and microSD breakout on standoffs or a rigid
   carrier.
3. Keep the BFab board close to the handset jack.
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

Fill this in as the hardware is inspected. Replace each `TBD` in the repository
only with a measured or documented value.

| Measurement | Result |
| --- | --- |
| ESP32 board revision | `TBD` |
| BFab board revision | `TBD` |
| BFab matches Waveshare schematic | `TBD: yes/no/differences` |
| Handset connector orientation photo | `TBD` |
| Earpiece contacts | `TBD` |
| Earpiece resistance | `TBD ohms` |
| Microphone contacts | `TBD` |
| Microphone type and marking | `TBD` |
| Microphone positive contact | `TBD` |
| Microphone negative contact | `TBD` |
| Hook-switch contacts | `TBD` |
| Switch closed when | `TBD: lifted/replaced` |
| ESP32 SDA GPIO | `TBD` |
| ESP32 SCL GPIO | `TBD` |
| ESP32 I2S CLK GPIO | `TBD` |
| ESP32 I2S LRCLK GPIO | `TBD` |
| ESP32 I2S input GPIO | `TBD` |
| ESP32 I2S output GPIO | `TBD` |
| ESP32 microSD SCK GPIO | `TBD` |
| ESP32 microSD MOSI GPIO | `TBD` |
| ESP32 microSD MISO GPIO | `TBD` |
| ESP32 microSD CS GPIO | `TBD` |
| ESP32 hook-switch GPIO | `TBD` |
| 5 V power distribution method | `TBD` |
