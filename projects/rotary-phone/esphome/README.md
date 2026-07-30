# ESPHome Bench Tests

`rotary-phone.yaml` is a temporary bench configuration for testing the XIAO
ESP32-C6 and rotary-phone hardware. The tracked file records an earlier
checkpoint; the live Device Builder YAML now also has encrypted Wi-Fi API
access and password-protected OTA.

The hook-switch bench setup on GPIO17/`D7` uses:

- Internal pull-up enabled
- 150 ms delayed-on and delayed-off debounce
- `ON CRADLE` and `LIFTED` status every two seconds
- Immediate lift and replacement event logs

Connect hook wire `S2` to `D7` and `S4` to ground; insulate `S1` and `S3`
individually. This internal-pull-up setup is for bench testing. The permanent
circuit still requires the 10 kohm pull-up and 100 nF capacitor in
[Checkpoint 3](../BUILD.md#checkpoint-3-prepare-the-hook-switch).

Audio hardware testing passed on 2026-07-29 with clear stock spoken-audio
playback. The live experimental `wm8960_audio_test` has since been adapted for
the handset earpiece and right `MIC1` input. It enables `MICBIAS`, applies
software high-pass filtering, reports raw and filtered levels, and compiles
with repeatable 150 ms debounced hook-triggered cycles. Those live changes are
diagnostic and have not yet been promoted to production guestbook firmware.

The original handset microphone remains quiet and power-noise limited. Battery
power and added supply decoupling are substantially quieter than laptop USB.
A MAX4466 amplified microphone module is planned for the handset after the
microSD checkpoint.

The Adafruit 254 microSD board has arrived. Its first test will use `D3`/GPIO21
for CS, `D8`/GPIO19 for clock, `D9`/GPIO20 for MISO, and `D10`/GPIO18 for MOSI.
No card wiring or filesystem test has passed yet.
