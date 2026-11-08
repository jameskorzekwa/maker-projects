# Power and battery notes (2026-11-07)

Measured and documented facts for running the CrowPanel 2.1" knob on a battery.
**[P]** = primary source (schematic/datasheet/official docs), **[A]** = forum report.

## Board facts (Elecrow schematic `ESP32_Display_2.1(K)_Main_V1.0.sch`) [P]

Repo: https://github.com/Elecrow-RD/CrowPanel-2.1inch-HMI-ESP32-Rotary-Display-480-480-IPS-Round-Touch-Knob-Screen

- **PCF8574 INT is not connected to the ESP32** (only a 4.7 kΩ pull-up). The
  push button (P5) and touch INT (P2) can therefore never wake the ESP32 from
  sleep without a hardware change. Encoder: A → GPIO42, B → GPIO4.
- **GPIO4 is an RTC pin (can wake from deep sleep); GPIO42 is not.** A twist
  can wake the board from deep sleep; a tap cannot.
- No battery connector or charger. 5 V arrives through the 12-pin FPC from
  the USB-C daughter board. 3.3 V comes from an **RY3420 buck** (input
  2.5–5.5 V, ~40 µA quiescent), so a single LiPo could feed the 5 V rail
  directly if the USB path is cut.
- Always-on green power LED, 1 kΩ from 3.3 V: ≈1.2 mA, which dominates any
  sleep budget unless removed.
- Backlight: GPIO6 → NPN sink; 0 mA when off. Four LEDs, ≈60–80 mA at full.
- Panel VDD is switched by PCF8574 P3 through a PMOS, but LCD pin 39 (also
  VCC) is wired straight to 3.3 V, so whether the panel can really be
  powered down needs a meter test.

## ESP32-S3 numbers [P]

Datasheet v2.2 §5.6: deep sleep 7–8 µA; light sleep 240 µA (+140 µA for the
8 MB octal PSRAM); modem-sleep 27–33 mA; Wi-Fi TX peaks 280–340 mA.
ESP-IDF low-power guide (measured averages): modem-sleep DTIM1 40 mA; auto
light-sleep with Wi-Fi connected 2.45 mA (DTIM1) to 0.93 mA (DTIM10).

## Measured on similar boards [A]

ESP32-S3 + ST7701S 480×480 (Guition 4848S040): backlight off 70–110 mA, full
brightness 230–270 mA; backlight off + LVGL paused ≈90 mA; + ST7701S Sleep
In (0x10) ≈60 mA. XIAO ESP32-S3 (octal PSRAM): deep sleep 13 µA, light sleep
0.7–2.9 mA.

## ESPHome support

- `deep_sleep` supports `wakeup_pin`/`esp32_ext1_wakeup` on RTC pins (GPIO4
  works). Wake is a full reboot: 5–10 s before Wi-Fi + HA are back.
- Automatic light sleep with Wi-Fi (`power_management: esp32_power_management`)
  landed in ESPHome 2026.11 but **aborts on ESP32-S3 with IDF 5.5.x**
  (esphome/esphome#21890; needs IDF 6.0.1). GPIO wake from light sleep is an
  open PR (#21074). This is the mode to watch: ~2–3 mA with HA connected and
  near-instant wake.
- ESPHome's st7701s driver has no panel sleep-in command; feature request
  esphome/feature-requests#2987.

## Current firmware modes

| Mode | Approx. draw | Notes |
| --- | ---: | --- |
| Awake, 100 % backlight | 200–270 mA | |
| Sleep screen, 25 % backlight | ~120 mA | |
| **Display Off** (fireplace off, 30 s in Sleep) | ~90–110 mA | backlight off + Wi-Fi modem sleep, instant wake |
| Panel sleep-in (not implemented) | ~60 mA | needs a custom SPI write |
| Auto light sleep (ESPHome not ready on S3) | ~3 mA | instant wake, HA stays connected |
| Deep sleep (not implemented) | ~1.3 mA with LED, ~50 µA without | twist-only wake, 5–10 s cold start |

## Battery hardware options

- DFRobot Solar Power Manager 5V (DFR0559): LiPo charger + 5 V/1 A boost,
  quiescent < 1 mA, $7.90. Plugs into the knob's USB-C.
- Adafruit PowerBoost 1000C: 5.2 V/1 A, load sharing, 5 mA quiescent, $19.95.
- Seeed LiPo Rider Plus: 5 V/2.4 A, always-on 3.3 V rail, < $5.
- Power banks mostly switch off below ~50 mA load; Voltaic V50 has a
  documented always-on mode ($74).

## Rough battery life, 5,000 mAh LiPo, 85 % converter efficiency

| Idle mode | Idle life | Each hour of fire (~150 mA) |
| --- | ---: | ---: |
| Display Off (today) | ~1.8 days | −3.5 % |
| Light sleep (future) | ~2 months | −3.5 % |
| Deep sleep, LED removed | > 1 year | −3.5 % |

The idle floor can be made excellent; the real limit is the time the fire
is on with the screen awake. A fireplace that burns 3 h a night costs ~10 %
of a 5,000 mAh battery per night regardless of the sleep mode.
