# Fireplace Knob

A round 2.1-inch rotary display that controls a gas fireplace through Home
Assistant. It runs ESPHome with an LVGL interface and works with the knob
alone, with no touch input.

## Screens

```
SLEEP --tap--> MAIN --tap--> SETTINGS --tap on item--> EDIT (Power/Mode/Fan/Light/Flame/Brightness)
  |             |  ^            |  ^                        |
  | turn        |  | tap/10 s   |  +---------- tap ---------+
  v             |  |            +-- "Back" + tap --> MAIN
SET POINT <--turn+  |
  +-----------------+
Any screen: 20 s with no input --> SLEEP.  Hold ~0.9 s anywhere: fireplace on/off.
```

| Screen | Shows | Turn | Tap |
| --- | --- | --- | --- |
| **Sleep** | Room temperature at the Sleep brightness; a small orange flame when the fireplace is on | Opens Set Point; the first click already changes the value | Main |
| **Main** | Room temperature, set point (or flame height in Manual), mode, fan and light state | Set Point | Settings |
| **Set Point** | Set temperature (Manual while burning: flame height) | ±1 °F (or ±1 flame) | Main (also after 10 s idle) |
| **Settings** | Power, Mode, Fan, Light, Flame, Brightness, Back. Flame appears only in Manual while burning | Choose a row | Open it (Back → Main) |
| **Power** | ON/OFF | Choose | Apply and return to Settings |
| **Mode** | Manual / Thermostat / Smart | Choose | Apply and return to Settings. While off, only picks the mode for the next turn-on |
| **Fan / Light / Flame** | Fan 0–6, light on/off, flame 1–6 | Change (sent ~1 s after you stop) | Settings |
| **Brightness** | Awake brightness, then Sleep brightness (previewed live) | ±5 % | Next step, then Settings |

- Only a long press, or Power in Settings, ever lights the fireplace. Set
  Point while the fireplace is off just stores the value ("Saved for next
  turn-on"); Flame is locked while off.
- Power and Mode changes are discarded if the screen times out before you tap.
- The Proflame 2 IFC handles the fan delays itself: the blower starts about
  5 minutes after the burner lights and runs about 12 minutes after it goes
  out. The knob shows the chosen speed, not whether the blower is running.
- The knob follows mode changes made from the remote or HA.
- The Proflame integrations report no set point (`null`) in Off/Manual, so the
  knob keeps its own last value, saved across reboots.

### Home Assistant device page (Configuration)

| Entity | Purpose |
| --- | --- |
| **Awake Brightness** (10–100 %, default 100) | Backlight on every screen except Sleep |
| **Sleep Brightness** (0–100 %, default 25) | Backlight on the Sleep screen; set to 0 for a dark screen at night |
| **Room Temperature Source** | `Fireplace` (the remote's reading, default) or `Living room` (`sensor.main_floor_temperature`). Falls back to the fireplace reading if the chosen sensor is unavailable. The option list is compiled into the firmware (`alt_temp_entity`/`alt_temp_label` substitutions); adding a sensor needs a rebuild. |

All three are remembered across reboots and can be changed from the knob
(Brightness) or by HA automations.

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
| `esphome/fireplace-knob/base.yaml` + `knob_helpers.h` | Hardware, UI, and behaviour. All entities are substitutions. |
| `esphome/fireplace-knob-living-room.yaml` | Living room: `proflame_connect_wifi` entities (`switch.living_room_fireplace`, `climate.living_room_fireplace_mode`, `number.living_room_fireplace_flame_height`, `number.living_room_fireplace_fan_speed`, `switch.living_room_fireplace_light`). |
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

1. Before connecting to Home Assistant, the Sleep screen shows `--` and Main
   shows **OFFLINE**. This confirms that the panel initialisation and backlight
   work.
2. From Sleep, turn the knob: Set Point should open and the value should move
   on that first click. Clockwise should raise it; if not, set
   `rotation_sign: "-1"`. Wait 10 s: it should return to Main, and 10 s later
   go to Sleep.
3. Tap through Main → Settings → each item and back. Hold to switch the
   fireplace on and off. Confirm each change in Home Assistant.
4. Change the fan, light, mode and brightness. Check the HA entities and the
   two brightness sliders on the device page.

## Safety Notes

- The knob only calls Home Assistant actions. Fireplace safety, including the
  IFC, the pilot, and the remote taking priority, remains with the fireplace
  controller.
- A long press (or Power in Settings) is the only input that can light the
  fireplace.

## Revisions

| Date | Change |
| --- | --- |
| 2026-11-07 | Initial firmware for the living-room fireplace, flashed over USB and adopted in HA ("Allow actions" enabled). |
| 2026-11-07 | New UI: Sleep/Main/Set Point/Settings flow, Brightness and Room Temperature Source settings in HA. OTA installed. |
| 2026-11-07 | Mode screen (Manual/Thermostat/Smart) with per-mode screen lists, a Flame screen for Manual, and Fan in every mode. OTA installed. |
| 2026-11-07 | Hardware feedback: header icons overlapped their titles, so the icon + text rows are now auto-centred flex rows. The panel drifted and glitched, so the firmware now uses PSRAM XIP, `LCD_RGB_RESTART_IN_VSYNC`, IRAM-safe RGB ISR, 64-byte cache lines, and a 16 MHz pclk. OTA installed; drift fix awaiting confirmation. |
