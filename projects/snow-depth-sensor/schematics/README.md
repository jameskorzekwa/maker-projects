# Schematics and Manufacturer References

This directory catalogs official engineering documents for the current snow-depth sensor and the planned MB7389 conversion. The documents remain on manufacturer servers because publication does not grant permission to redistribute them in this public repository.

## Current Hardware

### M5Stack ATOM Echo / Atom Voice

- [Official product documentation and pin map](https://docs.m5stack.com/en/atom/atomecho)
- [Official schematic image](https://static-cdn.m5stack.com/resource/docs/products/atom/atomecho/atomecho_sch_01.webp)
- [Official printable product manual](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/atom/atomecho.pdf)
- [Official mechanical-size drawing](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/products/atom/atomecho/module%20size.jpg)
- [Espressif ESP32-PICO series datasheet](https://documentation.espressif.com/esp32-pico_series_datasheet_en.pdf)

M5Stack publishes the board schematic as an image, not as a standalone schematic PDF or editable PCB project.

### Controlled A02-Style Ultrasonic Sensor

The installed Amazon item is branded Gugxiom, model `Gugxioms32yompzgb`, ASIN `B0CFFQNXDT`. No official manufacturer website, schematic, datasheet, mechanical drawing, or protocol manual was found.

Its behavior resembles the controlled-UART DYP-A02YYTW family, but its OEM identity is not confirmed. These DYP documents are candidate-family references only and must not be represented as exact documentation for the installed sensor:

- [DYP A02 family product page](https://www.dypcn.com/3cm-blind-zone-ip67-high-precision-ultrasonic-sensor-dyp-a02-product/)
- [DYP A02 family datasheet](https://www.dypcn.com/uploads/A02-Datasheet.pdf)
- [DYP A02 output protocols](https://www.dypcn.com/uploads/A02-Output-Interfaces.pdf)
- [DYP enclosed-sensor mechanical drawing](https://www.dypcn.com/uploads/644c7545.pdf)

No exact board or sensor circuit schematic is available for the installed marketplace item.

### Waveshare Solar Power Manager (D)

- [Official product page](https://www.waveshare.com/solar-power-manager-d.htm)
- [Official wiki/manual](https://www.waveshare.com/wiki/Solar_Power_Manager_(D))
- [Original board schematic](https://files.waveshare.com/wiki/Solar-Power-Manager-(D)/Solar_Power_Manager_D_Schematic.pdf)
- [V2 board schematic](https://files.waveshare.com/wiki/Solar-Power-Manager-(D)/Solar_Power_Manager_D_Schematic_V2.pdf)
- [Mechanical dimension drawing](https://www.waveshare.com/w/upload/2/25/Solar_Power_Manager_%28D%29_Dim.jpg)
- [SW6106 controller datasheet](https://files.waveshare.com/wiki/Solar-Power-Manager-(D)/SW6106_Datasheet_EN.pdf)
- [CN3791 solar charger datasheet](https://files.waveshare.com/wiki/Solar-Power-Manager-(D)/CN3791_Datasheet_EN.pdf)

Use the schematic matching the physical board revision. Waveshare does not publish editable PCB source, Gerbers, or a BOM for this board.

### DFRobot Gravity MAX17043, DFR0563

- [Official product page](https://www.dfrobot.com/product-1734.html)
- [Official wiki/manual](https://wiki.dfrobot.com/dfr0563/)
- [DFR0563 V1.0 board schematic](https://dfimg.dfrobot.com/wiki/20187/DFR0563_gravity-max17043-37v-lithium-battery-fuel-gauge-module_schematics_V1.0.pdf)
- [DFR0563 V1.0 dimension drawing](https://dfimg.dfrobot.com/wiki/20187/DFR0563_gravity-max17043-37v-lithium-battery-fuel-gauge-module_dimension_V1.0.pdf)
- [DFR0563 V1.0 board/layout drawing](https://dfimg.dfrobot.com/wiki/20187/DFR0563_gravity-max17043-37v-lithium-battery-fuel-gauge-module_layout_V1.0.pdf)
- [MAX17043 IC datasheet](https://dfimg.dfrobot.com/wiki/20187/DFR0563_gravity-max17043-37v-lithium-battery-fuel-gauge-module_datasheet_V1.0.pdf)
- [MIT-licensed driver source](https://github.com/DFRobot/DFRobot_MAX17043)

The DFR0563 schematic is a board-level schematic. The MAX17043 datasheet describes the IC and is not a substitute for the module schematic.

### Generic INA219 Breakout

- [Texas Instruments INA219 product page](https://www.ti.com/product/INA219)
- [Official INA219 IC datasheet](https://www.ti.com/lit/gpn/ina219)
- [TI INA219EVM product page](https://www.ti.com/tool/INA219EVM)
- [TI INA219EVM board schematic](https://www.ti.com/lit/pdf/sbor006)

The installed breakout's manufacturer, PCB revision, and shunt marking are not recorded. The TI datasheet covers the INA219 IC, and the EVM schematic covers TI's evaluation module only; neither is an exact board schematic for a generic marketplace breakout.

### NEWCONNY YXC-001 Solar Panel Assembly

- [Amazon product listing, ASIN B0DZC69ZHL](https://www.amazon.com/dp/B0DZC69ZHL)

No independent manufacturer engineering page, downloadable datasheet, schematic, controlled dimension drawing, or manual was found. The listing identifies model `YXC-001`, part `NCY002`, and two 5 V/8 W panels advertised as 5 V/16 W combined.

### Parallel Battery Pack

The pack contains two matching low-temperature-rated 3.7 V cells, but the exact manufacturer and model are intentionally not recorded yet. No battery datasheet can be identified reliably until the model is recorded.

## Planned MB7389 Conversion

### MaxBotix MB7389-100

- [Official HRXL-MaxSonar-WR datasheet page](https://maxbotix.com/pages/hrxl-maxsonar-wr-datasheet)
- [Current datasheet PDF, document 11500](https://cdn.shopify.com/s/files/1/0550/8091/0899/files/11500.pdf)
- [Full-horn IGES mechanical CAD archive](https://cdn.shopify.com/s/files/1/0550/8091/0899/files/HRXL-MaxSonar-WR-igs.zip)

The datasheet contains the pinout, serial protocol, mechanical drawings, power specifications, and target-specific beam plots. MaxBotix does not publish an internal circuit schematic for the sealed sensor.

### Pololu Item 2810 High-Side Switch

- [Official product and operation guide](https://www.pololu.com/product/2810)
- [Official resources index](https://www.pololu.com/product/2810/resources)
- [Board schematic](https://www.pololu.com/file/0J888/mini-mosfet-slide-switch-schematic-diagram.pdf)
- [Dimension drawing](https://www.pololu.com/file/0J1103/mini-mosfet-slide-switch-with-reverse-voltage-protection-dimension-diagram.pdf)
- [STEP model](https://www.pololu.com/file/0J1371/mini-mosfet-slide-switch-with-reverse-voltage-protection.step)
- [DXF drill guide](https://www.pololu.com/file/0J1098/psw04a-drill.dxf)

Pololu publishes a board-level schematic and mechanical files, but no editable KiCad/EAGLE project, Gerbers, or open-hardware license was found.
