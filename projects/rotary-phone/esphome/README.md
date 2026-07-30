# ESPHome Bench Firmware

`rotary-phone.yaml` is the bench configuration for the XIAO ESP32-C6 and
rotary-phone hardware. The tracked file matches the live Device Builder YAML
except for the Wi-Fi, encrypted-API, and OTA credential blocks, which exist
only on the server. The ESP-IDF `disable_vfs_support_dir: false` option is
required: ESPHome's default disables `opendir`, `rename`, and `unlink`, which
the recorder needs for message numbering and crash-safe finalizing.

## `wm8960_audio_test` component

Despite its historical name, this is now the working guestbook recorder. On
every debounced handset lift it waits two seconds so the handset can reach the
ear, initializes the WM8960, plays a beep through the earpiece via the
headphone output, and streams the handset microphone (right input, `MICBIAS`
biased, 100 Hz software high-pass) to the card as 16 kHz/16-bit mono PCM.
Replacing the handset finalizes a numbered `/sdcard/MSG#####.WAV`; recordings
stream to a `.TMP` file with roughly one `fsync` per second and are renamed on
completion, so an interruption cannot damage finished messages. Messages
shorter than half a second are discarded, a five-minute limit finalizes
automatically, and the cycle rearms after every hang-up. The first real
message, `MSG00001.WAV`, was recorded and saved on 2026-07-30.

## `sd_card_test` component

Verified 2026-07-30. Mounts the FAT filesystem over SDSPI at 4 MHz on
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

The recorder also runs an `esp_http_server` on port 80 once Wi-Fi connects.
`GET /messages` returns a JSON list of saved recordings and
`GET /messages/MSG#####.WAV` streams one; downloads answer `503` while a
recording cycle is active. A template text sensor named `Last Message`
publishes the full download URL of each newly saved message.

On the Home Assistant side, the Downloader integration (download directory
`/media`) and the `Rotary Phone: copy new message to media` automation download
every new message into `/media/rotary-phone`, where the Media browser can play
it. The server must start after the network is up: the component retries
`httpd_start` from `loop()` every five seconds until `network::is_connected()`
is true, because component setup runs before the network stack exists.

## Known limitations

- The original handset microphone is quiet and power-noise sensitive; battery
  power with supply decoupling is the quietest tested source. A MAX4466
  amplified module will replace it inside the handset.
- The automatic download automation was verified with a manual backfill of
  `MSG00001.WAV`; its live trigger from a fresh recording awaits the next
  bench test.
