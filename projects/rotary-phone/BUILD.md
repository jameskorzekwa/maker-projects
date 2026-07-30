# Step-by-Step Build Guide

This is the guide to follow while building the phone. Complete one checkpoint,
verify the result, and stop before starting the next checkpoint. Photos confirm
the controller and all of its exposed pins, so the GPIO connections below are
final for the bench prototype.

For circuit details and the complete test plan, see
[TECHNICAL.md](TECHNICAL.md).

## Important Safety Rule

Never connect this phone to a telephone wall jack again. Telephone lines can
carry dangerous ringing voltage. The finished phone will use only a regulated
5 V USB power supply.

Disconnect every USB cable and power supply before measuring resistance,
checking continuity, cutting wires, or soldering.

## Parts Used

Use these exact names when following the instructions:

| Short name | Actual part |
| --- | --- |
| ESP32 | Seeed Studio XIAO ESP32-C6 |
| Audio board | Blue Waveshare WM8960 Audio HAT |
| microSD board | Adafruit product 254, Amazon ASIN `B00NAY2NAI` |
| Hook switch | Four-white-wire switch operated by the handset cradle |
| Earpiece | Speaker inside the end of the handset held to your ear |
| Microphone | Part inside the end of the handset you speak into |

## What Is Already Known

The coiled cord and base jack use mirrored color orders. This is normal for a
4P4C telephone handset connection. Do not rearrange the wires.

| Coiled-cord wire | Base-jack wire |
| --- | --- |
| Yellow | Black |
| Green | Red |
| Red | Green |
| Black | Yellow |

Physical inspection and resistance measurements identify the two circuits:

| Function | Coiled-cord contacts | Base-jack wires | Measurement |
| --- | --- | --- | ---: |
| Earpiece | `1-4`, black and yellow | Black and yellow | 129.3 Ω |
| Microphone | `2-3`, red and green | Red and green | Directional |

The extra red/black pair connected to the small switch board is not part of the
handset connection. Its original line-powered function was not conclusively
traced; leave both wires disconnected and individually insulated.

The four white hook-switch wires were previously marked `S1`, `S2`, `S3`, and
`S4`. This build uses `S2` and `S4`:

- Handset resting on cradle: `S2` and `S4` are connected.
- Handset lifted: `S2` and `S4` are disconnected.
- `S1` and `S3` will not be connected to anything.

Photos also confirm the complete XIAO ESP32-C6 pin map. Use the labels printed
on the back of the board rather than counting pads:

<!-- markdownlint-disable MD013 -->

| XIAO label | Chip GPIO | Assigned job |
| --- | ---: | --- |
| `D0` | GPIO0 | Audio bit clock |
| `D1` | GPIO1 | Audio left/right clock |
| `D2` | GPIO2 | Recorded audio from Audio board to ESP32 |
| `D3` | GPIO21 | microSD card select |
| `D4` | GPIO22 | Audio board I2C data |
| `D5` | GPIO23 | Audio board I2C clock |
| `D6` | GPIO16 | Playback audio from ESP32 to Audio board |
| `D7` | GPIO17 | Hook switch |
| `D8` | GPIO19 | microSD clock |
| `D9` | GPIO20 | microSD data out, called `MISO` |
| `D10` | GPIO18 | microSD data in, called `MOSI` |

<!-- markdownlint-enable MD013 -->

The three power labels are:

- `VBUS`: 5 V from the USB-C connection.
- `GND`: common electrical ground.
- `3V3`: 3.3 V for the hook-switch pull-up resistor.

## Checkpoint 1: Handset Mapping Complete

The earpiece and microphone pairs are now identified. The 129.3 Ω earpiece will
use one channel of the Audio board's 3.5 mm headphone output. It will not use
the `LP`/`LN` speaker terminals.

The microphone photo and directional resistance test are consistent with a
two-wire electret-style microphone:

- Red meter probe on coiled-cord contact `2`/red and black probe on contact
  `3`/green: approximately 0.858 kΩ.
- Reversed probes: approximately 1.93 kΩ.
- The reading changes slightly while speaking.

Use this polarity:

| Location | Microphone positive | Microphone negative |
| --- | --- | --- |
| Coiled cord | Contact `2`, red | Contact `3`, green |
| Base jack | Green | Red |

The base-jack colors are reversed because of the mirrored 4P4C connection.

## Checkpoint 2: Prepare the Adafruit microSD Board

The Adafruit board has arrived. This is the current checkpoint. Complete only
the physical preparation and continuity checks below, then stop for firmware
and powered testing.

The Adafruit board has eight connections. The names printed on the board mean:

| Printed label | Plain-language meaning | Eventual connection |
| --- | --- | --- |
| `5V` | Power for the board | XIAO `VBUS` |
| `3V` | Regulated 3.3 V from the board | Leave unused |
| `GND` | Electrical ground | XIAO `GND` |
| `CLK` | SPI clock | XIAO `D8` |
| `DO` | Data coming out of the card | XIAO `D9` |
| `DI` | Data going into the card | XIAO `D10` |
| `CS` | Selects the microSD card | XIAO `D3` |
| `CD` | Optional card-present switch | Leave unused initially |

The board accepts 5 V power and converts it safely for the microSD card. Power
the XIAO through USB-C and use `VBUS` as the single 5 V source. Never connect a
second 5 V supply at the same time.

1. Solder the supplied straight header into the Adafruit board.
2. Keep the black plastic part of the header flat against the board while
   soldering.
3. Check closely for solder joining two neighboring pins.
4. Connect only the six required wires: `5V`, `GND`, `CLK`, `DO`, `DI`, and
   `CS`.
5. Keep every wire as short as practical.
6. Leave `3V` and `CD` disconnected.
7. Do not insert the microSD card until the wiring has been checked.

**Stop after soldering.** Check continuity for accidental shorts before power
is connected.

**Current status, 2026-07-30:** board received; header, wiring, card mount, and
file tests are not yet verified.

## Checkpoint 3: Prepare the Hook Switch

The hook-switch GPIO is XIAO `D7`:

```text
XIAO 3V3 ------- 10 kohm resistor ---+--- XIAO D7
                                     |
                                     +--- one side of 100 nF capacitor
                                     |
                                     +--- white wire S2

XIAO GND ----------------------------+--- other side of capacitor
                                     |
                                     +--- white wire S4

White wire S1 --------------------------- insulated and unused
White wire S3 --------------------------- insulated and unused
```

1. Keep the four-white-wire plug disconnected from the original telephone
   board permanently.
2. Insulate `S1` by itself.
3. Insulate `S3` by itself.
4. Connect `S4` to XIAO `GND`.
5. Connect `S2`, the resistor, the capacitor, and `D7` exactly as shown.

The firmware will read `LOW` while the handset is down and `HIGH` when the
handset is lifted.

**Bench result, 2026-07-29: PASS.** With `S2` connected directly to `D7`, `S4`
to ground, the XIAO's internal pull-up enabled, and 150 ms firmware debounce,
the logs consistently reported `ON CRADLE`, `LIFTED`, and the corresponding
transition events. Install the external 10 kohm resistor and 100 nF capacitor
shown above before permanent assembly.

## Checkpoint 4: Test the Unmodified Audio Board

Do not remove either silver microphone from the Audio board yet.

Place the Audio board so the words `WM8960 Audio HAT` are upright and its long
black 40-pin connector is along the bottom:

- The 3.5 mm jack on the left is audio output, not microphone input.
- The right silver microphone is the one that may later be replaced.
- The green speaker terminals read `LP`, `LN`, `RN`, `RP` from left to right.
- Raspberry Pi header pins 1 and 2 are at the far-right end, below the right
  microphone and beside the button. They are not at the earphone-jack end.

At that far-right end, the row closer to the board components contains the odd
pin numbers and the row closer to the board edge contains the even numbers:

```text
Toward board components:  pin 5  pin 3  pin 1
Toward bottom board edge: pin 6  pin 4  pin 2
                                      right end
```

### Checkpoint 4A: Detect the Codec Over I2C

This first test uses only four wires. Do not connect the speaker, handset,
microSD board, or any I2S signal yet.

| Audio board signal | Raspberry Pi header pin | XIAO connection |
| --- | ---: | --- |
| 5 V | 2 | `VBUS` |
| Ground | 6 | `GND` |
| I2C data, `SDA` | 3 | `D4` |
| I2C clock, `SCL` | 5 | `D5` |

1. Disconnect USB-C and every other power source.
2. Leave both silver Audio board microphones untouched.
3. Start at the header's far-right end, below the right microphone. Identify
   pin 1 in the row closer to the components and pin 2 directly below it in the
   row closer to the board edge. Stop and request an orientation check if this
   does not match the board.
4. Connect only the four rows in the table.
5. Inspect for reversed pins, loose strands, and solder bridges.
6. Open `esphome/rotary-phone.yaml` in HA ESPHome Device Builder.
7. Install it to the XIAO over USB and open the USB logs.
8. Confirm the scan reports I2C address `0x1A`.

**Stop if `0x1A` is absent.** Disconnect USB immediately if either board becomes
warm. Do not add I2S wiring until this probe succeeds.

**Checkpoint result, 2026-07-29: PASS.** With the four wires on header pins 2,
3, 5, and 6 at the right-microphone end, the ESPHome scan found the WM8960 at
address `0x1A`. The XIAO, Audio board power, and I2C control connection are
working.

### Checkpoint 4B: Record and Play Through I2S

This test uses the temporary firmware in `esphome/`. It initializes the codec at
low speaker volume, plays a short beep, records four seconds from an onboard
microphone, and plays the recording through both test speakers.

<!-- markdownlint-disable MD013 -->

| Audio board signal | Raspberry Pi header pin | XIAO connection |
| --- | ---: | --- |
| 5 V | 2 | `VBUS` |
| Ground | 6 | `GND` |
| I2C data, `SDA` | 3 | `D4` |
| I2C clock, `SCL` | 5 | `D5` |
| Audio bit clock | 12 | `D0` |
| Audio left/right clock | 35 | `D1` |
| Recorded audio from Audio board | 38 | `D2` |
| Playback audio to Audio board | 40 | `D6` |

<!-- markdownlint-enable MD013 -->

The bench wiring was visually verified with these jumper colors: blue pin 12
to `D0`, purple pin 35 to `D1`, orange pin 38 to `D2`, and green pin 40 to
`D6`.

1. Connect the Audio board to the XIAO exactly as shown in the table.
2. Insert the supplied speakers' keyed four-wire plug into the white `SPEAKER`
   socket. Do not also use the green screw terminals.
3. Use the single 5 V power method recorded in the final wiring table. Never
   connect separate USB and external 5 V supplies at the same time.
4. Install `esphome/rotary-phone.yaml` over USB and keep USB logs open.
5. Wait for `Playing the start beep`. After the beep, speak toward either silver
   onboard microphone for four seconds.
6. Listen as the firmware immediately plays the recording through both test
   speakers.
7. Confirm the beep and speech are audible and neither board becomes hot.
8. Press the XIAO Reset button to repeat the one-shot test if needed.

**Stop if recording or playback fails.** Do not modify a board that has not
passed this stock test.

**Checkpoint result, 2026-07-29: PASS.** The onboard microphone recording played
back clearly through the supplied speaker system. The temporary generated beep
was audible but choppy; this is a diagnostic-firmware buffering issue, not an
audio-hardware failure. Final firmware will play a buffered prompt such as
"Please record a message for the bride and groom after the beep," drain the
prompt, play a buffered beep, and only then begin recording.

## Checkpoint 5: Test microSD Storage

Start with a 16 GB or 32 GB high-endurance microSD card.

1. Format the card as FAT32 on a computer.
2. Insert it into the unpowered Adafruit board.
3. Recheck all six wires against the final wiring table.
4. Apply power and run the storage test firmware.
5. Create a test file, read it back, and verify its contents.
6. Record at least five minutes of test audio to the card.
7. Play the file on a computer and listen for missing or damaged audio.

**Stop if any write error occurs.** The guestbook must not be assembled inside
the phone until storage is reliable.

## Checkpoint 6: Connect the Original Handset

Proceed only after the unmodified Audio board passes its stock recording and
playback test.

The right silver microphone on the Audio board is a tiny surface-mounted part.
Replacing it requires hot air, flux, fine tweezers, and magnification. Have an
electronics repair technician perform this modification if you do not already
have surface-mount rework experience.

The handset connections are:

- Base-jack green is microphone positive. It connects to the biased microphone
  signal input described in the technical reference.
- Base-jack red is microphone negative. It connects to Audio board ground.
- Base-jack yellow connects to the tip of a 3.5 mm plug.
- Base-jack black connects to the sleeve of that 3.5 mm plug.
- Leave the plug's ring connection unused.
- `LN` is not ground. Never connect it to ESP32 ground.

Start every audio test at the lowest software volume. Stop immediately if the
earpiece becomes warm, distorted, or painfully loud.

**Earpiece result, 2026-07-30: PASS.** A cut TRS cable maps blue to tip, red to
ring, and gold to sleeve. Blue/tip to base yellow and gold/sleeve to base black
played both the beep and recorded speech through the original 129.3 ohm
earpiece. Red/ring remains individually insulated.

**Original microphone result, 2026-07-30: PARTIAL.** `L3` beside `MIC1` was
removed, leaving the C14/input-side pad connected to `C14` and isolated from
the original microphone output. WM8960 `MICBIAS` measures 2.9 to 3.0 V at
`C9`/`C10`; a 2.2 kohm resistor feeds base green and the C14-side signal pad,
while base red goes to ground. The microphone node measures about 0.9 V. Speech
is recognizable but very quiet, and power noise dominates on computer USB.
Battery power is substantially quieter than either laptop USB or the tested
wall supply. Added supply decoupling improved the result.

The selected next microphone is a MAX4466 amplified electret module, Amazon
ASIN `B08N4FNFTR`. Do not remove the present microphone or reassign handset
wires until that board arrives, its 20.8 x 13.8 x 7.5 mm fit is checked, and a
new wiring checkpoint is written.

## Checkpoint 7: Test Everything on the Workbench

Keep every board outside the telephone case. Repeat steps 1 through 5 ten times
without rebooting the ESP32:

1. Put the handset on the cradle. Nothing should record.
2. Lift the handset. The instructions should play through its earpiece.
3. Wait for the beep.
4. Speak a short test message.
5. Replace the handset. Recording should stop and save.
6. Disconnect power after the tenth message is saved.
7. Remove the microSD card.
8. Confirm that the card contains ten different recordings.
9. Play every recording on a computer.

Do not install the electronics until all ten recordings are present and
playable.

## Checkpoint 8: Install the Electronics

1. Remove or physically block the telephone-line jack.
2. Mount the ESP32, Audio board, and Adafruit board on standoffs or a rigid
   carrier.
3. Keep the Audio board close to the handset connector.
4. Keep the red and green microphone wires away from USB and microSD wires.
5. Keep the ESP32 antenna away from metal and wiring.
6. Secure every cable so pulling it cannot stress a solder joint.
7. Insulate every unused wire and exposed joint.
8. Make sure the cradle mechanism cannot touch any wire.
9. Keep the microSD card accessible if the case permits it.
10. Close the case without pinching wires.

Repeat the complete lift, record, and hang-up test after closing the case.

## Current Next Action

Proceed with Checkpoint 2 only. With all power disconnected, inspect the
Adafruit 254 board, solder its straight header, and stop before connecting it
to the XIAO. The first powered storage checkpoint will mount an existing FAT32
filesystem without formatting it, write and flush a known test file, and read
the file back byte-for-byte before any audio is written to the card.
