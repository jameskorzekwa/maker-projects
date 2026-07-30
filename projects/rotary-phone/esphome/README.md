# ESPHome Bench Tests

`rotary-phone.yaml` is a temporary, offline configuration for testing the XIAO
ESP32-C6 and rotary-phone hardware.

The current configuration performs the hook-switch bench test on GPIO17/`D7`:

- Internal pull-up enabled
- 150 ms delayed-on and delayed-off debounce
- `ON CRADLE` and `LIFTED` status every two seconds
- Immediate lift and replacement event logs

Connect hook wire `S2` to `D7` and `S4` to ground; insulate `S1` and `S3`
individually. This internal-pull-up setup is for bench testing. The permanent
circuit still requires the 10 kohm pull-up and 100 nF capacitor in
[Checkpoint 3](../BUILD.md#checkpoint-3-prepare-the-hook-switch).

The local `wm8960_audio_test` component remains in this directory as the record
of the completed audio test but is not loaded by the current YAML. Audio
hardware testing passed on 2026-07-29 with clear spoken-audio playback.
