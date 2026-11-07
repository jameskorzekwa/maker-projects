# Fireplace Knob

A round 2.1-inch rotary display that controls a gas fireplace through Home
Assistant. It runs ESPHome with an LVGL interface and works with the knob
alone, with no touch input.

| Input | Action |
| --- | --- |
| Hold the knob (~0.9 s) | Fireplace on/off. A "Hold to turn ON/OFF" hint appears after 0.2 s. |
| Short press | Next screen: **Set Temp → Fan Speed → Light → Set Temp …** |
| Turn (Set Temp) | Set point ±1 °F per detent (55–85 °F). Sent 0.9 s after you stop turning. |
| Turn (Fan Speed) | Fan 0 (OFF) to 6. Sent 0.6 s after you stop turning. |
| Turn (Light) | Clockwise turns the light on; counter-clockwise turns it off. |

The top of every screen shows the fireplace state: an orange flame with **ON**,
a grey flame with **OFF**, or **OFFLINE**/**UNAVAILABLE**. Three dots at the
bottom show the current screen. The screen dims after 20 s and turns off after
2 minutes, then returns to Set Temp. When the screen is dark, the first press
or turn only wakes it. A long press always works.

### Temperature behaviour

- Setting a temperature calls `climate.set_temperature`. Neither fireplace
  integration uses a set point in **Manual** (flame-height) mode. If the
  fireplace is burning in Manual when you set a temperature, the knob also
  switches it to the **Thermostat** preset.
- If the fireplace is off, the set point is only stored. The knob never lights
  a fireplace except through a long press.
- The Proflame integrations report the set point as `null` in Off/Manual. The
  knob therefore keeps its own last value, saved across reboots, and shows that
  value until Home Assistant reports a real one.

## Parts

| Component | Quantity | Notes | Purchase link |
| --- | ---: | --- | --- |
| ELECROW CrowPanel 2.1" ESP32-S3 rotary display (480×480 IPS, ST7701S RGB, CST826 touch, PCF8574 expander, rotary encoder with push) | 1 per fireplace | ESP32-S3, 16 MB flash, 8 MB octal PSRAM, USB-C power | [Amazon B0G3TZGRC4](https://www.amazon.com/dp/B0G3TZGRC4) |
| USB-C 5 V supply | 1 | Wall powered; no battery | — |

## Hardware Notes

The pinout and the ST7701S init sequence come from the working community
firmware
[Jastreb07/elecrow-crowpanel-esphome-thermostat](https://github.com/Jastreb07/elecrow-crowpanel-esphome-thermostat)
(`thermostat_480.yaml`, MIT).

| Function | Pin |
| --- | --- |
| I2C (PCF8574 @0x21, touch @0x15) | SDA GPIO38, SCL GPIO39 |
| Backlight PWM | GPIO6 |
| Panel SPI (init only) | CLK GPIO2, MOSI GPIO1, CS GPIO16 |
| RGB timing | DE 40, HSYNC 15, VSYNC 7, PCLK 41 |
| RGB data | R 46,3,8,18,17 · G 14,13,12,11,10,9 · B 5,45,48,47,21 |
| Encoder A / B | GPIO4 / GPIO42 |
| Encoder push | PCF8574 P5 (active low) |
| LCD power / LCD reset / touch reset / touch INT | PCF8574 P3 / P4 (inverted) / P0 / P2 |

Touch is not configured. The CST826 controller has no upstream ESPHome
component, and this interface does not use touch.

## Firmware

| File | Purpose |
| --- | --- |
| `esphome/fireplace-knob/base.yaml` | Hardware, UI, and behaviour. All entities are substitutions. |
| `esphome/fireplace-knob-living-room.yaml` | Living room: `proflame_connect_wifi` entities (`switch.living_room_fireplace`, `climate.living_room_fireplace_mode`, `number.living_room_fireplace_fan_speed`, `switch.living_room_fireplace_light`). |
| `esphome/fireplace-knob-basement.yaml` | Basement: ESP32 X0A controller entities (`*.basement_fireplace_*`), 0.5° steps. Not deployed. |

`base.yaml` sits in a subdirectory so the Device Builder does not list it as a
device. Credentials come from Device Builder `secrets.yaml`:
`fireplace_knob_api_key`, `fireplace_knob_ota_password`,
`fireplace_knob_ap_password`, and the shared `wifi_ssid`/`wifi_password`.

Tunable substitutions in `base.yaml` include `temp_min`/`temp_max`,
`temp_step`, `long_press`, `dim_after_s`, `off_after_s`, `dim_brightness`,
`thermostat_preset`, and `rotation_sign`. Set `rotation_sign` to `"-1"` if
clockwise turns the value down.

## First Flash

The board has no ESPHome firmware when it arrives, so the first flash must be
over USB.

1. In the ESPHome Device Builder, open **fireplace-knob-living-room** and
   choose **Install → Manual download → Factory format**. Alternatively, use the
   prebuilt `.factory.bin`.
2. Connect the CrowPanel to a computer by USB-C. Open
   <https://web.esphome.io> in Chrome or Edge, click **Connect**, select the
   serial port, and choose **Install** with the factory `.bin`. If the port does
   not appear or the flash fails, hold the board's **BOOT** button while
   plugging it in.
3. The device joins Wi-Fi with the shared credentials. In Home Assistant, it
   appears under **Settings → Devices & services** as a discovered ESPHome
   device. Add it with the API encryption key from `secrets.yaml`.
4. **Required:** open the ESPHome integration entry for the knob, choose
   **Configure**, and enable **Allow the device to perform Home Assistant
   actions**. Without this setting the screen updates, but every command is
   dropped.
5. Later updates install over Wi-Fi (OTA) from the Device Builder.

## Testing

1. Before connecting to Home Assistant, the screen should show the ring, the
   grey flame, and **OFFLINE / Connecting to Home Assistant**. This confirms
   that the panel initialisation and backlight work.
2. Turn the knob on Set Temp. Clockwise should raise the number. If it lowers
   the number, set `rotation_sign: "-1"`.
3. Short-press through all three screens, then hold to switch the fireplace on
   and off. Confirm each change in Home Assistant.
4. Turn the fan up and down, and switch the light on and off. Check each Home
   Assistant entity.

## Safety Notes

- The knob only calls Home Assistant actions. Fireplace safety, including the
  IFC, the pilot, and the remote taking priority, remains with the fireplace
  controller.
- A long press is the only input that can light the fireplace.

## Revisions

| Date | Change |
| --- | --- |
| 2026-11-07 | Initial firmware for the living-room fireplace, prepared before the hardware arrived. Validated and compiled, but not yet run on hardware. |
