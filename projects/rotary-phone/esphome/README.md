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

## Known limitations

- The original handset microphone is quiet and power-noise sensitive; battery
  power with supply decoupling is the quietest tested source. A MAX4466
  amplified module will replace it inside the handset.
- Playback of saved messages from Home Assistant's media browser is planned
  but not yet implemented.
