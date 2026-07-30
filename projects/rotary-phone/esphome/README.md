# ESPHome Audio HAT Test

`rotary-phone.yaml` is a temporary, offline configuration for testing the XIAO
ESP32-C6 and unmodified WM8960 Audio HAT.

Checkpoint 4A confirmed the codec at I2C address `0x1A`. The current
configuration performs the Checkpoint 4B stock audio test. It uses the local
`wm8960_audio_test` external component to:

1. Initialize the HAT's 24 MHz clock path, onboard microphones, ADC, DAC, and
   class-D outputs at low gain.
2. Play a short, quiet start beep.
3. Record four seconds from one onboard microphone into RAM at 16 kHz.
4. Play that recording through both supplied test speakers.
5. Shut down the codec outputs and wait for Reset before repeating.

It retains USB logging and the I2C scan but has no Wi-Fi, API, OTA, microSD, or
handset configuration. Follow [Checkpoint 4B](../BUILD.md#checkpoint-4b-record-and-play-through-i2s)
for the exact I2S and speaker wiring. Do not modify either onboard microphone
until the stock record/playback test passes.
