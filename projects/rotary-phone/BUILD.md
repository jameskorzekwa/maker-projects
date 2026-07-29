# Step-by-Step Build Guide

This is the guide to follow while building the phone. Complete one checkpoint,
verify the result, and stop before starting the next checkpoint. The exact GPIO
connections will be added after the ESP32 board labels are photographed and
verified.

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
| ESP32 | ESP32-C5 Mini marked `ESP32-C5_MINI_V1.0` |
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

## Checkpoint 1: Gather Information Before Soldering

Do not connect any boards during this checkpoint.

1. Unplug the phone from the telephone wall jack.
2. Remove the telephone line cord from the phone.
3. Disconnect all USB cables and power supplies.
4. Place the ESP32 on a well-lit surface.
5. Take one sharp photograph of the top of the ESP32.
6. Take one sharp photograph of the bottom of the ESP32.
7. Make sure every printed pin label is readable in the photographs.
8. Open the speaking end of the handset.
9. Take a sharp photograph of the microphone, its two connections, and any
   writing on it.
10. Stop rather than prying or breaking the handset if it does not open easily.
11. Leave the microphone connected for now.
12. Keep the coiled handset cord plugged into the phone base.
13. Photograph and label the red and black wire connections before removing
    them.
14. Desolder the red and black wires from the original telephone board. Do not
    cut them.
15. Keep the two bare wire ends separate from each other and every other part.
16. Set the multimeter to resistance, shown by the ohm symbol.
17. Touch one meter probe to red and the other probe to black.
18. Record the stable resistance reading.

**Stop here.** The next checkpoint requires:

- Both ESP32 photographs.
- The handset microphone photograph.
- The red-to-black resistance reading.

Do not guess the remaining connections. These three items determine the GPIO
map and the safe connections for the original handset.

## Checkpoint 2: Prepare the Adafruit microSD Board

Start this checkpoint only after the Checkpoint 1 results have been reviewed.

The Adafruit board has eight connections. The names printed on the board mean:

| Printed label | Plain-language meaning | Eventual connection |
| --- | --- | --- |
| `5V` | Power for the board | ESP32 5 V/VBUS pin, to be verified |
| `3V` | Regulated 3.3 V from the board | Leave unused |
| `GND` | Electrical ground | ESP32 `GND` |
| `CLK` | SPI clock | ESP32 pin to be selected |
| `DO` | Data coming out of the card | ESP32 `MISO` pin to be selected |
| `DI` | Data going into the card | ESP32 `MOSI` pin to be selected |
| `CS` | Selects the microSD card | ESP32 pin to be selected |
| `CD` | Optional card-present switch | Leave unused initially |

The board accepts 5 V power and converts it safely for the microSD card. Do not
connect either power pin until the ESP32 photographs have been reviewed.

When the final pins are known:

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

Build this small circuit only after a safe ESP32 GPIO has been selected:

```text
ESP32 3.3 V --- 10 kohm resistor ---+--- selected hook-switch GPIO
                                     |
                                     +--- one side of 100 nF capacitor
                                     |
                                     +--- white wire S2

ESP32 GND ---------------------------+--- other side of capacitor
                                     |
                                     +--- white wire S4

White wire S1 --------------------------- insulated and unused
White wire S3 --------------------------- insulated and unused
```

1. Keep the four-white-wire plug disconnected from the original telephone
   board permanently.
2. Insulate `S1` by itself.
3. Insulate `S3` by itself.
4. Connect `S4` to ESP32 ground.
5. Connect `S2`, the resistor, the capacitor, and the selected GPIO exactly as
   shown.

The firmware will read `LOW` while the handset is down and `HIGH` when the
handset is lifted.

## Checkpoint 4: Test the Unmodified Audio Board

Do not remove either silver microphone from the Audio board yet.

Place the Audio board so the words `WM8960 Audio HAT` are upright and its long
black 40-pin connector is along the bottom:

- The 3.5 mm jack on the left is audio output, not microphone input.
- The right silver microphone is the one that may later be replaced.
- The green speaker terminals read `LP`, `LN`, `RN`, `RP` from left to right.

After the ESP32 GPIO map and test firmware are ready:

1. Connect the Audio board to the ESP32 using the completed wiring table.
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

This checkpoint depends on the microphone photograph and earpiece resistance
from Checkpoint 1. Do not continue with an unidentified microphone or earpiece.

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

Complete Checkpoint 1. Do not solder the ESP32, Audio board, or Adafruit board
yet. The next revision will replace every remaining "to be selected" entry with
an exact pin number after the ESP32 photographs and handset measurements are
available.
