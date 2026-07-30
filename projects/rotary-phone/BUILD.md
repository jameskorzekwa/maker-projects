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

The original telephone board labels identify the handset wires:

| Wire | Purpose |
| --- | --- |
| Red | Earpiece positive |
| Black | Earpiece negative |
| Yellow | Microphone positive |
| Green | Microphone negative |

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

## Checkpoint 1: Measure the Earpiece

The earlier handset photos already identify the four wire colors. Do not send
those wiring photos again. This measurement checks the resistance of the small
speaker inside the handset so its safe Audio board output can be selected.

1. Unplug the phone from the telephone wall jack.
2. Remove the telephone line cord from the phone.
3. Disconnect all USB cables and power supplies.
4. Keep the coiled handset cord plugged into the phone base.
5. If red and black are still attached to the original telephone board, stop
   and say so before desoldering anything.
6. Ignore any reading taken while that connector is plugged into the original
   telephone board. That reading includes the telephone electronics and is not
   the earpiece resistance.
7. After the four-wire connector is unplugged, keep its exposed contacts away
   from every other wire and part.
8. Set the multimeter to resistance, shown by the ohm symbol (`Ω`). Use the
   `200 Ω` range if the meter does not select a range automatically.
9. Touch the two meter probes together and record that reading.
10. Touch one probe to red and the other probe to black. Polarity does not
   matter for this measurement.
11. Record the stable reading, including the `Ω` symbol. Report `OL` if that is
    what the meter displays.
12. Open the speaking end of the handset only if it opens without force.
13. Send one close photograph of the actual microphone capsule and any writing
    on it. This is different from the wiring photos already received.

**Stop here.** Report only these new results:

- The reading with the meter probes touching each other.
- The red-to-black resistance reading.
- The microphone-capsule close-up, if the handset opens easily.

Do not guess the earpiece connection. The resistance determines whether red and
black use the Audio board's headphone output or speaker output.

## Checkpoint 2: Prepare the Adafruit microSD Board

Start this checkpoint only after the Checkpoint 1 results have been reviewed.

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

## Checkpoint 4: Test the Unmodified Audio Board

Do not remove either silver microphone from the Audio board yet.

Place the Audio board so the words `WM8960 Audio HAT` are upright and its long
black 40-pin connector is along the bottom:

- The 3.5 mm jack on the left is audio output, not microphone input.
- The right silver microphone is the one that may later be replaced.
- The green speaker terminals read `LP`, `LN`, `RN`, `RP` from left to right.

After the audio test firmware is ready:

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

1. Connect the Audio board to the XIAO exactly as shown in the table.
2. Connect only the small test speaker supplied with the Audio board.
3. Use the single 5 V power method recorded in the final wiring table. Never
   connect separate USB and external 5 V supplies at the same time.
4. Record five seconds through the Audio board's built-in microphone.
5. Play the recording through the test speaker.
6. Confirm the sound is clear and neither board becomes hot.

**Stop if recording or playback fails.** Do not modify a board that has not
passed this stock test.

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

This checkpoint depends on the microphone-capsule close-up and earpiece
resistance from Checkpoint 1. Do not continue with an unidentified microphone
or earpiece.

The right silver microphone on the Audio board is a tiny surface-mounted part.
Replacing it requires hot air, flux, fine tweezers, and magnification. Have an
electronics repair technician perform this modification if you do not already
have surface-mount rework experience.

The final handset connections will be selected after the measurements:

- Yellow and green will connect the handset microphone to the verified right
  microphone input circuit.
- Red and black will connect the earpiece either to the 3.5 mm output or to
  `LP` and `LN`, depending on the measured resistance.
- `LN` is not ground. Never connect it to ESP32 ground.

Start every audio test at the lowest software volume. Stop immediately if the
earpiece becomes warm, distorted, or painfully loud.

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
4. Keep the yellow and green microphone wires away from USB and microSD wires.
5. Keep the ESP32 antenna away from metal and wiring.
6. Secure every cable so pulling it cannot stress a solder joint.
7. Insulate every unused wire and exposed joint.
8. Make sure the cradle mechanism cannot touch any wire.
9. Keep the microSD card accessible if the case permits it.
10. Close the case without pinching wires.

Repeat the complete lift, record, and hang-up test after closing the case.

## Current Next Action

Complete the resistance measurement in Checkpoint 1. The ESP32 photos and
handset wiring photos have already been recorded and do not need to be sent
again. If red and black are still soldered to the original telephone board,
stop and report that before removing them. If the isolated red-to-black reading
is still `OL` with both ends of the coiled handset cord fully inserted, the next
step is measuring directly at the earpiece in the listening end of the handset.
