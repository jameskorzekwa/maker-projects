# ESPHome Bench Firmware

`rotary-phone.yaml` is the bench configuration for the XIAO ESP32-C6 and
rotary-phone hardware. The tracked file matches the live Device Builder YAML
except for the credential blocks, which exist only on the server: the
encrypted-API key, the OTA password, and the `wifi:` block. The live `wifi:`
block now contains **no station credentials** — only the fallback AP
(`RotaryPhone-Setup` with a per-unit password) — plus a top-level
`captive_portal:`, so the device provisions itself exactly like the planned
rental product. The ESP-IDF `disable_vfs_support_dir: false` option is
required: ESPHome's default disables `opendir`, `rename`, and `unlink`, which
the recorder needs for message numbering and crash-safe finalizing.

## Wi-Fi provisioning (verified 2026-07-31)

The full renter flow passed end-to-end on the bench: with no stored network,
the phone broadcasts its setup AP; scanning a standard `WIFI:` QR code joins
it; the OS captive-portal sheet opens automatically; the user picks a scanned
SSID and enters its password; the device saves the credentials to flash and
joins the network. Recording works fully offline — Wi-Fi only gates uploads.

The local `captive_portal` component is a copy of ESPHome 2026.7.3's component
with a custom, branded provisioning page in `captive_index.h` (wedding-themed
card layout, tappable network list with signal bars, password show/hide, and a
plain-language success screen). Endpoints are unchanged (`/config.json`,
`/wifisave`, `/update`); keep the YAML `captive_portal:` compression at its
gzip default because the embedded page is stored gzip-only. The setup QR code
generation command and per-unit password live outside the repository.

## `wm8960_audio_test` component

Despite its historical name, this is now the working guestbook recorder. On
every debounced handset lift it waits two seconds, plays `/sdcard/PROMPT.WAV`
(greeting with an embedded beep) through the earpiece, and streams the handset
microphone to the card as 16 kHz/16-bit mono PCM. Replacing the handset
finalizes a numbered, crash-safe `/sdcard/MSG#####.WAV`. Messages shorter than
half a second are discarded, a five-minute limit finalizes automatically, and
the cycle rearms after every hang-up.

Capture details: right input with `MICBIAS`, +30 dB PGA and +20 dB boost, a
300–3800 Hz telephone band-pass in Q13 fixed point (the C6 has no FPU), and
2.5x digital makeup gain. Audio flows through a two-second RAM ring buffer so
card stalls cannot drop samples; writes happen in large ~1.5 s bursts with an
fsync every 8 s. `auto_clear` is enabled on the I2S TX channel — without it, a
transmit underflow endlessly replays the last buffered audio (this was the
endless/choppy beep bug). Wi-Fi transmit power drops to minimum during
recording and restores on hang-up.

The greeting is uploaded over Wi-Fi with `PUT /prompt` (16 kHz/16-bit mono WAV
with a canonical 44-byte header; 8.3 filename rules apply). If no greeting file
exists, a generated beep is the fallback.

## `sd_card_test` component

Verified 2026-07-30. Mounts the FAT filesystem over SDSPI (now 8 MHz) on
`D8`/`D9`/`D10` with CS on `D3`, without formatting, then writes and reads back
`SDTEST.TXT` byte-for-byte. It leaves `/sdcard` mounted for the recorder. File
names must stay in DOS 8.3 form because long-filename support is disabled in
the default FAT configuration.

## Hook switch

The bench setup on GPIO17/`D7` uses the internal pull-up with 150 ms debounce
in the recorder component, plus a 2-second status log from the YAML. Connect
hook wire `S2` to `D7` and `S4` to ground; insulate `S1` and `S3` individually.
The permanent circuit still requires the 10 kohm pull-up and 100 nF capacitor
in [Checkpoint 3](../BUILD.md#checkpoint-3-prepare-the-hook-switch).

## Home Assistant playback

The recorder also runs an `esp_http_server` on port 80 once Wi-Fi connects:

- `GET /` — self-contained player page listing every message with inline audio
  controls, newest first.
- `GET /messages` — JSON list; `GET /messages/MSG#####.WAV` — stream one.
- `PUT /prompt` — replace the greeting.
- Requests answer `503` while a recording cycle is active. The server starts
  from `loop()` once `network::is_connected()` because component setup runs
  before the network stack exists.

A template text sensor named `Last Message` publishes each new message's URL.
On the Home Assistant side, the Downloader integration (`/media`) plus the
`Rotary Phone: copy new message to media` automation mirror every message into
`/media/rotary-phone` for the Media browser (works remotely and in the app),
and the `Rotary Phone: notify on new message` automation sends a phone
notification that deep-links there.

## Known limitations

- The original handset microphone is quiet and power-noise sensitive; battery
  power with supply decoupling was the quietest tested source.
- Card-write current still couples an audible tick into the analog microphone
  path. Full-window muting suppresses it but removes roughly 3.4 percent of the
  recording.
- The MAX4466 experiment was rejected because its minimum gain was excessive
  and the module oscillated intermittently without local decoupling.
- Analog sidetone remains disabled because the high-gain handset path feeds
  back acoustically.

No further production development happens in this firmware. See
[`jameskorzekwa/heirloom-hotline`](https://github.com/jameskorzekwa/heirloom-hotline)
for the Raspberry Pi implementation.
