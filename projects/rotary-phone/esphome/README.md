# ESPHome Audio HAT Probe

`rotary-phone.yaml` is a temporary, offline configuration for the first safe
test of the XIAO ESP32-C6 and unmodified WM8960 Audio HAT.

It intentionally enables only USB logging and an I2C scan. It does not contain
Wi-Fi, API, OTA, audio, microSD, or handset configuration.

Connect only the four signals documented in the main
[build guide](../BUILD.md), install the configuration over USB from ESPHome
Device Builder, and open USB logs. A working Audio HAT appears at I2C address
`0x1A`.

Do not add the Audio HAT's I2S signals or modify either onboard microphone until
this probe succeeds.
