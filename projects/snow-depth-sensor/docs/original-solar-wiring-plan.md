# Original Solar Wiring Plan (July 12, 2026) — HISTORICAL

> **This document is preserved for reference only.** It describes the original
> plan (WatangTech solar manager, single EEMB LP103454 cell, A02YYUW sensor,
> cold-charge cutoff in the panel line) — which is **not what was built**. The
> as-built system uses a Waveshare Solar Power Manager (D) *(retired
> 2026-07-23)*, then a CN3791/CN5305 MPPT board, a Gugxiom A02-style sensor
> *(replaced 2026-07-22 by a MaxBotix MB7389)*, and a single 2,000 mAh
> low-temperature cell. See the [project README](../README.md) and the
> [MB7389 conversion & power guide](mb7389-conversion-guide.md) for current
> documentation. Several recommendations here (cold-charge cutoff, battery
> fusing, enclosure venting) remain open items worth revisiting before winter.

![Planned wiring topology](images/original-plan-system-overview.png)

## Original text (converted from the July 12 docx)

```text
Snow Depth Sensor
Solar power, battery monitoring, and charging-rate wiring guide
ATOM Echo + A02YYUW + WatangTech MPPT manager + MAX17043 + INA219
Prepared July 12, 2026

Bottom line: You have the major electronic modules. Before outdoor installation, add a hardware low-temperature charge cutoff, an outdoor enclosure/cable glands, and the small wiring items listed below. Use GPIO21 and GPIO25 for the new I2C bus; the existing A02YYUW already occupies GPIO26 and GPIO32.

Critical battery rule: The EEMB LP103454 is rated to charge only from 0 to 45 C. The WatangTech board's low-temperature operating claim does not make the battery safe to charge below freezing. The charge source must be physically disconnected when the cell is at or below 0 C.

What you still need
A hardware low-temperature charge disconnect rated for at least 2 A DC. Put its temperature sensor against the LiPo pouch; set charging OFF at 0 C and ON again around +5 C.
A weather-resistant enclosure (preferably IP65/IP67), cable glands, strain relief, and a small breathable membrane vent or desiccant strategy for condensation.
A way to access ATOM Echo GPIO21, GPIO25, 3.3 V, 5 V, and GND: an ATOM Proto/terminal breakout or securely soldered 2.54 mm header wiring.
JST-PH 2.0 pigtails/adapters, 22-24 AWG stranded wire for power, smaller wire for I2C, heat-shrink, and a multimeter.
Recommended: an inline 1-2 A fuse near the battery positive lead and an I2C enclosure-temperature sensor for Home Assistant monitoring. The temperature sensor is not a substitute for the hardware cutoff.
Recommended: conformal coating on exposed boards, while keeping connectors, switches, and the ultrasonic faces uncoated.
Compatibility findings
Item
Finding
Action
Existing A02YYUW
GPIO32 UART RX; GPIO26 trigger; 5 V/GND
Keep this wiring unchanged.
MAX17043
I2C address 0x36; 3.3 V logic
Share the new GPIO21/GPIO25 I2C bus.
INA219
Usually address 0x40; clone pull-ups follow VCC
Power it from 3.3 V, never 5 V.
EEMB LP103454
3.7 V, 2000 mAh; 4.2 V full; charge 0-45 C
Use one cell initially and add cold-charge protection.
5 V / 6 W panel
About 1.2 A maximum under ideal sun
Use the board's lowest/5 V solar setting and verify charging.
WatangTech manager
5-24 V solar input; 5 V regulated output; 3.7 V battery
Use BAT connector only; leave 18650 holder empty.

Capacity expectation: One 2000 mAh cell stores about 7.4 Wh nominal. A continuously connected Wi-Fi ESP32 may exhaust that in roughly one night or a cloudy day, depending on actual current. Use the INA219 data to measure the power balance before relying on the system unattended. Do not casually parallel loose LiPo cells to increase capacity.


System wiring overview

Figure 1. Recommended wiring topology. Grounds are common throughout.
Pin assignment
ATOM Echo pin
Use
Connects to
5 V
Power input
WatangTech regulated 5 V output
GND
Common return
Solar manager, INA219, MAX17043, A02YYUW
GPIO32
UART RX
Existing A02YYUW TX
GPIO26
Trigger/output
Existing A02YYUW mode/trigger wire
GPIO21
I2C SDA
MAX17043 SDA and INA219 SDA
GPIO25
I2C SCL
MAX17043 SCL and INA219 SCL
3.3 V
I2C module power
MAX17043 VCC and INA219 VCC

Do not use: M5Stack reserves GPIO19, GPIO22, GPIO23, and GPIO33 for the ATOM Echo internal audio circuitry and warns against external reuse. GPIO21 and GPIO25 are the appropriate additional header pins for this build.

Wiring procedure
1. Bench-check every polarity
Disconnect all power. Do not plug in the panel, battery, USB, or ATOM Echo yet.
Check the EEMB plug. Confirm the red lead is battery positive and determine which solar-manager PH2 socket contact is BAT+. Connector housings are not a reliable polarity standard.
Check the panel cable. In sunlight, identify positive and negative with a multimeter. Mark them before cutting or adapting the cable.
Inspect the INA219 shunt. Most boards are marked R100, meaning 0.1 ohm. If yours is marked differently, change the ESPHome shunt_resistance value.
2. Wire the panel and INA219
From
To
Solar-panel positive
Cold-charge cutoff input
Cold-charge cutoff output
INA219 VIN+
INA219 VIN-
WatangTech SOLAR IN +
Solar-panel negative
WatangTech SOLAR IN -
INA219 GND
Common GND
INA219 VCC
ATOM Echo 3.3 V
INA219 SDA
ATOM Echo GPIO21
INA219 SCL
ATOM Echo GPIO25

This INA219 placement reports panel voltage, input current, and input power. It does not report the exact current entering the battery because the solar manager also powers the load and has conversion losses.
5 V panel caution: Set the delivered board's MPPT switch to its lowest or explicit 5 V position. If the board has no suitable 5 V position or charging drops out in modest light, use the panel's USB-C output into the manager's USB-C input with an inline USB breakout for the INA219, or replace the panel with a higher-voltage model. Never use the solar and USB-C charge inputs simultaneously.

3. Wire the battery and MAX17043
From
To
EEMB red / positive
MAX17043 BT1/PH2 battery positive
EEMB black / negative
MAX17043 BT1/PH2 battery negative
MAX17043 P1 terminal +
WatangTech BAT/PH2 positive through a pigtail
MAX17043 P1 terminal -
WatangTech BAT/PH2 negative through the same pigtail
MAX17043 VCC
ATOM Echo 3.3 V
MAX17043 GND
Common GND
MAX17043 SDA
ATOM Echo GPIO21
MAX17043 SCL
ATOM Echo GPIO25

One battery only: For the first build, connect one EEMB LiPo through the PH2 battery path and leave the WatangTech 18650 holder empty. Do not connect multiple loose cells to separate board connectors. A parallel pack requires matched cells at equal state of charge, appropriate individual protection/fusing, and a purpose-built harness.

4. Power the ATOM Echo
From
To
WatangTech 5 V output pin
ATOM Echo 5 V header pin
WatangTech GND
ATOM Echo GND header pin

The A02YYUW remains powered through the existing Grove harness shown in the photo. Its GPIO32/GPIO26 assignments remain unchanged.
USB warning: The ATOM Echo USB 5 V rail and external 5 V rail may be tied together. Disconnect the WatangTech 5 V feed before flashing with a normal USB cable, or use a verified data-only USB cable. Avoid connecting two independent 5 V sources together.

ESPHome configuration additions
The current Home Assistant configuration was checked over SSH. It already uses GPIO32 for the A02YYUW UART and GPIO26 as the trigger output. Add the following I2C block near the existing uart block:
i2c:   id: power_bus   sda: GPIO21   scl: GPIO25   scan: true   frequency: 100kHz
Then retain the existing A02YYUW sensor entry and append these two sensor entries under the same sensor section:
  - platform: max17043     id: battery_gauge     i2c_id: power_bus     battery_voltage:       name: "Snow Sensor Battery Voltage"       accuracy_decimals: 2     battery_level:       name: "Snow Sensor Battery"       accuracy_decimals: 0    - platform: ina219     i2c_id: power_bus     address: 0x40     shunt_resistance: 0.1 ohm     max_voltage: 16.0V     max_current: 1.5A     update_interval: 60s     bus_voltage:       name: "Snow Sensor Solar Voltage"       accuracy_decimals: 2     current:       name: "Snow Sensor Solar Current"       id: solar_current       accuracy_decimals: 3     power:       name: "Snow Sensor Solar Input Power"       accuracy_decimals: 2     shunt_voltage:       name: "Snow Sensor Solar Shunt Voltage"       disabled_by_default: true
First boot: With logger enabled, confirm that the I2C scan finds devices at 0x36 and 0x40. If the INA219 current is negative in sunlight, swap VIN+ and VIN- or apply a multiply: -1 filter after confirming the physical orientation.

Commissioning checklist
[ ] Battery polarity verified at every connector before insertion.
[ ] Solar-panel polarity verified in sunlight.
[ ] Cold-charge cutoff opens the panel-positive path at 0 C or colder.
[ ] INA219 and MAX17043 VCC measure approximately 3.3 V, not 5 V.
[ ] WatangTech output measures approximately 5 V before connecting the ATOM Echo.
[ ] I2C scan reports 0x36 and 0x40.
[ ] MAX17043 voltage agrees with a multimeter within a reasonable tolerance.
[ ] INA219 current becomes positive when the panel supplies power.
[ ] USB programming does not connect a second live 5 V source.
[ ] Enclosure is sealed, strain-relieved, and arranged so condensation cannot drip onto boards.
[ ] The battery cannot press against screws, sharp plastic, or a hot regulator.
Operational notes
The A02YYUW is specified down to about -15 C. Below that, readings may be unreliable even if the electronics continue running.
Cold air changes the speed of sound. Expect a temperature-dependent distance bias unless you compensate or calibrate against a fixed reference distance.
Snow, frost, or ice on either ultrasonic face can cause false readings. Mount the faces vertically or slightly downward and shield them from direct accumulation without blocking the 60-degree beam.
The MAX17043 percentage may need time to settle after first power-up. Leave it continuously powered while the system operates.
Create Home Assistant alerts for low battery, no solar power during daylight, and enclosure temperature near freezing.
References
WatangTech solar manager Amazon listing
DFRobot MAX17043 product and reference
ESPHome MAX17043 component
ESPHome INA219 component
M5Stack ATOM Echo pin map
A02YYUW specifications
EEMB LP103454 Amazon listing
5 V / 6 W solar panel Amazon listing
```
