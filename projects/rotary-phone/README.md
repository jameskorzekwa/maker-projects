# Rotary Phone Wedding Audio Guestbook

An ESP32-C6-based audio guestbook built into a rotary-style telephone. When a
guest lifts the handset, the phone plays prerecorded instructions and a beep.
It then records the guest's message until the handset is replaced. Every
message is saved to microSD, with optional background upload to cloud storage.

Follow the plain-language [step-by-step build guide](BUILD.md) one checkpoint at
a time. Component-level explanations, measurements, and the complete test plan
are kept separately in [TECHNICAL.md](TECHNICAL.md). Do not skip a stop point:
the original handset parts must be measured before final wiring or irreversible
modifications.

## Project Status

Hardware characterization and bench bring-up. The hook switch and handset wire
colors are known. The handset earpiece resistance and microphone type still
need to be recorded. The controller and GPIO table are now confirmed.

## Selected Hardware

- [Dyna-Living vintage rotary-style telephone](https://a.co/d/09yagHqx)
- [Seeed Studio XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [Waveshare WM8960 Audio HAT](https://www.amazon.com/dp/B098R7TTM4),
  sold through the BFab listing
- [Adafruit MicroSD Card Breakout Board+ product 254](https://www.adafruit.com/product/254),
  Amazon ASIN `B00NAY2NAI`

The phone is a modern, line-powered novelty telephone rather than a passive
vintage telephone. Its listing says the handset and telephone cords plug into
the base and that it requires no batteries. The original telephone-line
circuit will not be used.

The controller's front label identifies it as a Seeed Studio XIAO ESP32-C6.
Photos of its back confirm the complete `D0` through `D10`, `VBUS`, `GND`, and
`3V3` pin labels. It provides USB-C power and programming, 2.4 GHz Wi-Fi 6, an
onboard antenna, and a u.FL antenna connector.

## Requirements

- Play instructions when the handset is lifted.
- Play a short beep after the instructions.
- Record mono voice audio immediately after the beep.
- Stop and safely finalize the recording when the handset is replaced.
- Store every completed recording on removable microSD.
- Continue recording when Wi-Fi or cloud storage is unavailable.
- Optionally upload completed files while the phone is idle.
- Recover as much as possible from a recording interrupted by power loss.
- Give recording higher priority than networking or uploads.
- Operate unattended for the duration of a wedding reception.

## Recommended Architecture

```text
Handset microphone ---+
                      +-- WM8960 audio codec -- I2S/I2C -- ESP32-C6
Handset earpiece ------+                              |
                                                     +-- SPI microSD
Cradle hook switch ----------------------------------+
                                                     +-- status LED
                                                     +-- Wi-Fi backup
```

MicroSD is the authoritative storage location. Cloud storage is a secondary,
asynchronous backup and must never be required to accept a new message.

The ESP32-C6 has one I2S peripheral with separate transmit and receive channels
and DMA support. That is sufficient because playback and recording happen in
sequence. It also supports microSD through the SD SPI host driver.

## Bill of Materials

<!-- markdownlint-disable MD013 -->

| Item | Purpose | Notes |
| --- | --- | --- |
| Seeed Studio XIAO ESP32-C6 | Main controller | Verified from received photos |
| Waveshare WM8960 Audio HAT | Microphone ADC and earpiece DAC/amplifier | Delivered Waveshare-branded board sold through the BFab listing |
| Adafruit MicroSD Card Breakout Board+ product 254 | Local storage interface | Use its `5V`, `GND`, `CLK`, `DO`, `DI`, and `CS` connections |
| 16 GB or 32 GB high-endurance microSD card | Message storage | Buy and test one spare |
| 4P4C/RJ9 breakout or handset cable | Handset connection | Final connector depends on the phone |
| Certified 5 V, 2 A USB supply | Power | Keep mains voltage outside the phone |
| 10 kohm resistor and 100 nF capacitor | Hook-switch pull-up and debounce | Values may be adjusted during testing |
| Perfboard or custom PCB, connectors, and standoffs | Permanent assembly | Do not use loose jumpers in the final build |
| Optional electret microphone and 8/32 ohm earpiece | Replacement transducers | Only needed if the originals are unsuitable |
| Optional concealed RGB LED | Status indication | Ready, recording, upload, and error states |

<!-- markdownlint-enable MD013 -->

The delivered board is Waveshare-branded and visually matches the
[Waveshare WM8960 Audio HAT](https://www.waveshare.com/wiki/WM8960_Audio_HAT)
reference design. Complete its stock record and playback test before modifying
it.

## Phase 1: Characterize the Phone

The Amazon listing does not document the phone's internal circuitry. Complete
these checks before designing the final wiring:

1. Open the base and photograph all original connections.
2. Identify the mechanical hook-switch contacts with a multimeter.
3. Disconnect the hook switch completely from the telephone-line circuitry.
4. Identify the handset microphone pair and earpiece pair.
5. Measure the earpiece resistance.
6. Determine whether the microphone is an electret capsule and establish its
   polarity.
7. Test both transducers through the audio codec at low gain and volume.
8. Replace incompatible transducers inside the handset while retaining the
   original four-wire coiled cord.

Photos of the received phone confirm four separately terminated handset wires.
The original PCB silkscreen identifies red as receiver positive (`R+`), black
as receiver negative (`R-`), yellow as microphone positive (`M+`), and green as
microphone negative (`M-`). Earpiece resistance and microphone construction
still require physical measurements.

Photos also confirm a mechanically actuated hook-switch assembly on a small
PCB. A dedicated four-wire harness connects it to the original main board. The
harness will be unplugged from the telephone electronics, continuity-mapped in
both cradle positions, and one switching pair will drive an ESP32 GPIO. The
measured matrix selects `S2-S4`: it closes at approximately 0.8 ohm with the
handset down and opens when the handset is lifted. With a pull-up, the GPIO is
therefore low while idle and high when recording should begin.

### Handset and Codec Research

No manufacturer service schematic or model-specific handset pinout was found
for Dyna-Living model `Dyna-JJ0TOP12254-FBA` (Amazon ASIN `B08GR4KCPF`). The
listing confirms that it is a modern, line-powered telephone with a detachable
handset cord. A conventional 4P4C handset cord normally assigns the center two
contacts to the receiver and the outer two contacts to the transmitter. The
received phone's PCB labels confirm the following wire functions and polarity:

| Wire | Original PCB label | Function |
| --- | --- | --- |
| Red | `R+` | Earpiece positive |
| Black | `R-` | Earpiece negative |
| Yellow | `M+` | Microphone positive |
| Green | `M-` | Microphone negative |

Continuity to the handset capsules must still be checked before disconnecting
the original PCB.

The purchased Waveshare board has two onboard analog MEMS microphones and no
external microphone connector; its 3.5 mm jack is output-only. In the matching
Waveshare reference schematic:

- `MIC1` is an `AOS3729A-T42` analog MEMS microphone.
- `MIC1 DAT` passes through ferrite bead `L3` and coupling capacitor `C14`
  (10 uF) to WM8960 `RINPUT1`.
- `MIC2 DAT` passes through `L5` and `C21` (10 uF) to `LINPUT1`.
- The WM8960 `MICBIAS` output is decoupled by `C9` and `C10`, but is not exposed
  on a connector.
- The board operates at 5 V power and 3.3 V logic, with I2C control and I2S
  audio.

This topology should support the phone's original microphone if it is a
two-wire electret capsule, as expected for a modern electronic telephone. The
proposed modification for one channel is:

```text
WM8960 MICBIAS --- 2.2 kohm --- handset microphone +
                                      |
                                      +--- isolated MIC1 DAT/input pad

Board GND ----------------------- handset microphone -
```

Remove the `MIC1` package or isolate its `DAT` output before attaching the
handset microphone. Reuse the board's existing `L3`/`C14` signal path into the
codec. Enable `MICBIAS` and tune the WM8960 input gain in firmware. The exact
solder point must be selected after comparing both sides of the delivered
Waveshare PCB with the reference schematic.

If inspection shows a carbon transmitter rather than an electret capsule, do
not connect it using this circuit. A carbon transmitter requires a different
bias and preamplifier arrangement, or replacement with an electret capsule.

Before applying power:

1. Unplug the telephone from the wall line and keep it permanently isolated
   from PSTN wiring.
2. Photograph the handset capsule, its markings, terminals, and wire colors.
3. Identify the receiver and transmitter pairs by continuity instead of wire
   color alone.
4. Establish electret microphone polarity.
5. Isolate the handset jack from the original telephone PCB before connecting
   it to the WM8960 board.
6. Photograph both sides of the delivered Waveshare board and confirm that its
   input circuit matches the Waveshare schematic.

Leave the rotary dial disconnected and decorative in the first version. Dial
pulse support can be added later without affecting the core guestbook workflow.

## Phase 2: Build a Bench Prototype

Assemble the XIAO ESP32-C6, codec, microSD breakout, mapped hook switch, and a
test microphone and speaker outside the phone.

Prove the following in order:

1. Detect stable off-hook and on-hook transitions.
2. Play a WAV prompt from internal flash through I2S.
3. Record audio from the codec to a WAV file on microSD.
4. Switch cleanly from playback to recording.
5. Finalize a valid WAV file on a simulated hang-up.
6. Repeat without rebooting or leaking resources.

The received board photos and Seeed's official pin map establish this complete
allocation:

<!-- markdownlint-disable MD013 -->

| Job | XIAO pin |
| --- | --- |
| Audio I2S bit clock, left/right clock, input, output | `D0`, `D1`, `D2`, `D6` |
| Audio I2C data and clock | `D4`, `D5` |
| microSD card select, clock, MISO, MOSI | `D3`, `D8`, `D9`, `D10` |
| Hook switch | `D7` |

<!-- markdownlint-enable MD013 -->

Expected signal requirements are:

- Four I2S signals: bit clock, word clock, codec-to-ESP data, and ESP-to-codec data.
- Two I2C signals for codec configuration.
- Four SPI signals for microSD.
- One hook-switch GPIO.
- One optional status LED GPIO.

## Phase 3: Firmware

Use the current stable ESP-IDF release with ESP32-C6 support. The initial
implementation should use a small event-driven state machine rather than audio
or networking frameworks that obscure file handling.

### State Machine

<!-- markdownlint-disable MD013 -->

| State | Behavior |
| --- | --- |
| `BOOT` | Initialize hardware, mount microSD, recover interrupted files, and run self-tests |
| `IDLE` | Monitor the hook switch and process pending uploads |
| `PLAY_PROMPT` | Play instructions from internal flash after the handset is lifted |
| `PLAY_BEEP` | Play a short 800 to 1,000 Hz tone |
| `RECORD` | Stream microphone PCM data to a temporary WAV file |
| `FINALIZE` | Update the WAV header, flush, close, and rename the file |
| `UPLOAD` | Upload completed files while idle and Wi-Fi is available |
| `ERROR` | Play an unavailable message and indicate a fatal recording error |

<!-- markdownlint-enable MD013 -->

### Runtime Behavior

- Debounce the hook switch for approximately 100 to 200 ms.
- Abort without creating a recording if the handset is replaced during the prompt.
- Begin recording immediately after the beep.
- Limit messages to approximately five minutes.
- At the limit, finalize the file and play a short thank-you message.
- Stop or pause cloud activity immediately when the handset is lifted.
- Continue accepting messages without Wi-Fi.
- Never delete local recordings automatically during the event.
- Use watchdog and brownout handling to recover from unexpected failures.

Suggested task separation:

- A control task owns the state machine and hook-switch events.
- An audio task handles I2S DMA and block writes to microSD.
- A low-priority upload task runs only while idle.
- A status task or event handler updates the concealed LED and diagnostic log.

## Audio Format

Use uncompressed PCM WAV for the first version:

- Mono
- 16 kHz sample rate
- 16-bit samples
- Approximately 1.9 MB per minute

A 16 GB card holds well over 100 hours at this rate. WAV is simple to finalize,
recover, and play on any computer. Compression can be performed after the files
are copied or uploaded.

Store the instruction, thank-you, and error prompts in internal flash. Generate
the beep in firmware. Normalize prompts below full scale and tune microphone
gain in the actual event environment to avoid clipping.

## Crash-Safe Storage

Use a monotonically increasing message number stored in ESP32 nonvolatile
storage. Do not depend on network time for unique filenames.

Open each recording as a temporary file:

```text
/messages/.MSG_000123.tmp
```

Write a placeholder WAV header, stream audio in buffered blocks, and flush at a
reasonable interval. On hang-up:

1. Drain the final DMA buffer.
2. Update the WAV data length.
3. Flush and close the file.
4. Rename it atomically to `/messages/MSG_000123.wav`.
5. Add it to the upload queue.

At boot, scan for temporary files. Derive the recoverable audio length from the
file size, repair the header, and rename the file with a `_recovered` suffix.
A power failure may truncate the active message, but it must not damage any
previously finalized message.

## Optional Cloud Backup

Implement cloud upload only after local recording passes all reliability tests.
A small HTTPS endpoint backed by Cloudflare R2 or Amazon S3 is preferable to
direct Google Drive or Dropbox integration because it avoids complex OAuth and
token refresh flows on the microcontroller.

Cloud requirements:

- Give the device a narrowly scoped, write-only event token.
- Never store general cloud-account credentials on the ESP32.
- Use deterministic object names so retries are idempotent.
- Verify HTTPS certificates.
- Retry failures with exponential backoff.
- Preserve local files after successful upload.
- Interrupt upload immediately for an off-hook event.
- Rotate or disable the device token after the wedding.

An optional status endpoint can report the number of locally recorded and
successfully uploaded messages without providing access to their audio.

## Phase 4: Install in the Enclosure

- Remove or physically block the original telephone-line jack.
- Route only low-voltage 5 V power into the phone.
- Add strain relief to the power and handset wiring.
- Mount the codec close to the handset connector.
- Keep digital and antenna wiring away from the microphone pair.
- Mount the Wi-Fi antenna away from the phone's metal bracket.
- Confirm the board's antenna-selection resistor before using the u.FL antenna.
- Make the microSD card accessible through the bottom if practical.
- Secure boards with standoffs or a rigid internal carrier.
- Provide a concealed ready/error indicator visible to the operator.

Do not connect the modified phone to a telephone wall socket. Telephone lines
can carry hazardous ringing voltage. The original line circuitry must remain
disconnected from the new electronics.

## Verification Plan

Complete these tests at least one week before the event:

- 100 consecutive lift, record, and hang-up cycles with 100 unique playable files.
- Rapid lift/replacement and hook-switch bounce.
- Hang-up during the prompt and beep.
- Five-minute maximum-length recording.
- Missing, full, read-only, and corrupted microSD behavior.
- Power removal during recording followed by file recovery.
- Wi-Fi unavailable for several hours.
- Wi-Fi disconnecting during upload.
- A new recording while an upload is active.
- Eight-hour powered soak test.
- Operation in a room with loud music and conversation.
- Prompt volume, microphone gain, hum, clipping, and handset handling noise.

Acceptance criteria:

- The prompt begins within 500 ms of a stable off-hook event.
- Every normal hang-up produces one uniquely named, playable WAV file.
- Previously completed messages survive an arbitrary power interruption.
- The phone accepts messages indefinitely without Wi-Fi.
- Recording always takes priority over cloud upload.
- The device returns to ready state without manual intervention after
  recoverable errors.

## Event-Day Procedure

1. Install a tested and empty high-endurance card.
2. Power the phone and confirm the ready indication.
3. Record and play back a test message from the card.
4. Confirm cloud upload if the optional service is enabled.
5. Secure the enclosure, card, and power cable.
6. Keep the spare ESP32 board, card, cable, and power supply nearby.
7. Check the status indication periodically during the reception.
8. Copy the entire card to two separate locations immediately after the event.

## Open Decisions

- Whether internal handset transducers may be replaced if necessary.
- Whether cloud backup is required and, if so, whether to use R2 or S3.
- Whether the device will use venue Wi-Fi, a dedicated hotspot, or local
  storage only.
- Whether to add a small external status panel or keep indicators concealed.

## Technical References

- [Seeed Studio XIAO ESP32-C6](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [ESP-IDF stable ESP32-C6 documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/)
- [ESP32-C6 I2S documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/i2s.html)
- [ESP32-C6 SD SPI host documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-reference/peripherals/sdspi_host.html)
- [Waveshare WM8960 Audio Board](https://www.waveshare.com/wiki/WM8960_Audio_Board)
- [Waveshare WM8960 Audio HAT](https://www.waveshare.com/wiki/WM8960_Audio_HAT)
- [Waveshare WM8960 Audio HAT schematic](https://files.waveshare.com/upload/f/fa/WM8960_Audio_HAT_Schematic.pdf)
- [4P4C handset wiring](https://en.wikipedia.org/wiki/Modular_connector#Handset_wiring)
