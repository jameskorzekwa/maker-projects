# Waveshare-Era Wiring Guide (July 16, 2026) — HISTORICAL

> **This document describes the retired Waveshare Solar Power Manager (D)
> configuration** as built and operated 2026-07-16 through 2026-07-23. The
> Waveshare was retired on 2026-07-23 after its power-bank output stage
> (SW6106) shut the ESP32 down under light load, on mode dropouts, and on
> charge-source events; the "double-press low-current mode" ritual described
> below was the workaround for a problem that no longer exists. Current
> wiring: the [MB7389 conversion & power guide](mb7389-conversion-guide.md).
> Note this guide also predates the MB7389 conversion (it references the
> A02YYUW trigger wiring) and assumes a parallel battery pack that was never
> built (a single 2,000 mAh cell is installed). The I2C monitoring wiring
> (INA219/MAX17043, GPIO21/25) it documents remains accurate.

## Original text (converted from the Google Doc)

```text
Snow Depth Sensor
Hardware wiring guide
ATOM Echo + A02YYUW + Waveshare Solar Power Manager (D) + MAX17043 + INA219
Updated July 16, 2026

This guide shows how to connect the solar panel, INA219, Waveshare Solar Power Manager (D), low-temperature-rated 1S lithium battery pack, MAX17043, ATOM Echo, and the existing A02YYUW sensor.

System wiring overview

Power path: Solar panel + -> INA219 VIN+ -> INA219 VIN- -> Waveshare SOLAR IN+ screw terminal.
Battery path: Matched 3.7 V cells -> fused parallel harness -> MAX17043 BT1/PH2; MAX17043 P1 -> Waveshare BAT+/BAT- screw terminals.
I2C/power: MAX17043 and INA219 share GPIO21 (SDA), GPIO25 (SCL), 3.3 V, and GND; the Waveshare 5 V screw-terminal output powers the ATOM Echo.

Pin assignment
  5 V        Power input     Waveshare regulated 5 V OUT screw terminal
  GND        Common return   Solar manager, INA219, MAX17043, A02YYUW
  GPIO32     UART RX         Existing A02YYUW TX
  GPIO26     Trigger/output  Existing A02YYUW mode/trigger wire
  GPIO21     I2C SDA         MAX17043 SDA and INA219 SDA
  GPIO25     I2C SCL         MAX17043 SCL and INA219 SCL
  3.3 V      I2C module pwr  MAX17043 VCC and INA219 VCC

Do not use: M5Stack reserves GPIO19, GPIO22, GPIO23, and GPIO33 for the ATOM Echo internal audio circuitry and warns against external reuse. GPIO21 and GPIO25 are the appropriate additional header pins for this build.

Wiring procedure

1. Bench-check every polarity
Disconnect all power. Do not plug in the panel, battery, USB, or ATOM Echo yet.
Check every battery and the completed pack. Confirm each cell's positive and negative leads with a multimeter. Before joining cells in parallel, bring them to nearly the same voltage. Confirm the finished pack polarity and use the clearly labeled Waveshare BAT+ and BAT- screw terminals. If you instead use its PH2.0 4-pin battery connector, verify its polarity with a multimeter; connector housings are not a reliable polarity standard.
Check the panel cable. In sunlight, identify positive and negative with a multimeter. Mark them before cutting or adapting the cable.
Inspect the INA219 shunt. Most boards are marked R100, meaning 0.1 ohm. If yours is marked differently, change the ESPHome shunt_resistance value.

2. Wire the panel and INA219
  Solar-panel positive -> INA219 VIN+
  INA219 VIN- -> Waveshare SOLAR IN+ screw terminal
  Solar-panel negative -> Waveshare SOLAR IN- screw terminal
  INA219 GND -> Common GND
  INA219 VCC -> ATOM Echo 3.3 V
  INA219 SDA -> ATOM Echo GPIO21
  INA219 SCL -> ATOM Echo GPIO25

This INA219 placement reports panel voltage, input current, and input power. It does not report the exact current entering the battery because the solar manager also powers the load and has conversion losses.
Solar-input requirement: The Waveshare SOLAR IN terminals require a raw 6-24 V panel output. Do not connect a regulated 5 V USB panel output to SOLAR IN. If the panel only provides regulated 5 V USB, feed that into the Waveshare Type-C charging port instead; measuring that input with the INA219 requires an inline USB breakout. Do not feed the solar terminals and Type-C charging input simultaneously.

3. Wire the parallel battery pack and MAX17043
  Each matched battery positive, through its own fuse -> join as common pack positive -> MAX17043 BT1/PH2 positive
  Each matched battery negative -> join as common pack negative -> MAX17043 BT1/PH2 negative
  MAX17043 P1 terminal + -> Waveshare BAT+ screw terminal through a pigtail
  MAX17043 P1 terminal - -> Waveshare BAT- screw terminal through the same pigtail
  MAX17043 VCC -> ATOM Echo 3.3 V
  MAX17043 GND -> Common GND
  MAX17043 SDA -> ATOM Echo GPIO21
  MAX17043 SCL -> ATOM Echo GPIO25

Parallel-pack rule: Use identical low-temperature-rated 3.7 V cells of the same model, capacity, approximate age, and similar condition. Bring every cell to nearly the same voltage before connecting them. Join each positive lead through its own fuse into one common pack-positive lead; join all negative leads into one common pack-negative lead. Connect only the completed pack to the MAX17043 and treat it as a permanent 1S2P or 1S3P pack. Do not connect an additional battery through the optional Waveshare battery holder, and do not mix an 18650 with the LiPo pouch cells. Parallel capacity adds while voltage remains 3.7 V nominal/4.2 V full; the charger current does not increase, so the larger pack takes longer to recharge. The MAX17043 monitors the parallel group as one 1S battery.

4. Power the ATOM Echo
  Waveshare 5 V OUT+ screw terminal -> ATOM Echo 5 V header pin
  Waveshare 5 V OUT- screw terminal -> ATOM Echo GND header pin

The A02YYUW remains powered through the existing Grove harness. Its GPIO32/GPIO26 assignments remain unchanged. After connecting the battery, double-press the Waveshare function button to enter low-current mode so the 5 V output remains active while the ATOM Echo is in deep sleep. Verify that the ATOM completes at least one full sleep-and-wake cycle before installing it outdoors.
USB warning: The Waveshare Type-C connector can be used for charging or 5 V output, while its 5 V screw terminals power the ATOM Echo. The ATOM Echo USB 5 V rail and external 5 V rail may be tied together. Disconnect the Waveshare 5 V OUT+ feed before flashing with a normal USB cable, or use a verified data-only USB cable. Never connect two independent 5 V sources together.

ESPHome Builder configuration
The live snow-depth-sensor.yaml file in ESPHome Builder has already been updated and validated with ESPHome 2026.6.5. Keep the existing A02YYUW entry and use the following additions for the future power-monitoring boards:

i2c:
  id: power_bus
  sda: GPIO21
  scl: GPIO25
  scan: true
  frequency: 100kHz

sensor:
  # Keep the existing A02YYUW entry above these entries.
  - platform: max17043
    i2c_id: power_bus
    battery_voltage:
      name: "Snow Sensor Battery Voltage"
      accuracy_decimals: 2
    battery_level:
      name: "Snow Sensor Battery"
      accuracy_decimals: 0

  - platform: ina219
    i2c_id: power_bus
    address: 0x40
    shunt_resistance: 0.1 ohm
    max_voltage: 16.0V
    max_current: 1.5A
    update_interval: 60s
    bus_voltage:
      name: "Snow Sensor Solar Voltage"
      accuracy_decimals: 2
    current:
      name: "Snow Sensor Solar Current"
      accuracy_decimals: 3
    power:
      name: "Snow Sensor Solar Input Power"
      accuracy_decimals: 2
    shunt_voltage:
      name: "Snow Sensor Solar Shunt Voltage"
      disabled_by_default: true

Current behavior without the solar or battery hardware: the ATOM Echo can still be powered by USB and the A02YYUW depth sensor continues reporting normally. The MAX17043 and INA219 entities will be unavailable, and the log may show missing I2C devices, until those boards are connected. After the ATOM Echo is powered and online, install the updated configuration from ESPHome Builder. When the boards are connected, the I2C scan should find 0x36 for the MAX17043 and 0x40 for the INA219.
```
