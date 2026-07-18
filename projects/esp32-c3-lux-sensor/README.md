# ESP32-C3 Lux Sensor

This compact lux sensor uses a BH1750 ambient-light sensor and an ESP32-C3 development board. An adjustable DROK step-down converter allows the assembly to run from an 18 V DC supply.

## Parts

| Component | Quantity | Notes | Purchase link |
| --- | ---: | --- | --- |
| ESP32-C3 development board | 1 | Powered with 5 V through `VUSB` | [Amazon](https://a.co/d/08KFqkEg) |
| BH1750 ambient-light sensor module | 1 | Powered from the ESP32-C3's 3.3 V output | Purchase link needed |
| DROK adjustable DC step-down converter | 1 | Converts the 18 V input to 5 V | [Amazon](https://a.co/d/06YKp1IT) |
| Hookup wire | As needed | Suitable for short power and I2C connections | |
| Solder | As needed | | |

The original parts list included `https://a.co/d/08KFqkEg` twice. The BH1750 purchase link should be added when the correct third link is available.

## Tools and Supplies

- Soldering iron
- Multimeter
- Wire cutters and strippers
- Insulating mounting material or adhesive

## Power

The DROK converter receives 18 V DC and reduces it to 5 V for the ESP32-C3. Its 5 V output connects to the ESP32-C3 `VUSB` and `GND` pins.

Set and verify the converter output with a multimeter before connecting it to the ESP32-C3. Never connect the 18 V supply directly to the ESP32-C3 or BH1750.

## Wiring

### Power Converter

| From | To | Notes |
| --- | --- | --- |
| 18 V supply positive | DROK input positive | Observe input polarity |
| 18 V supply ground | DROK input ground | |
| DROK 5 V output positive | ESP32-C3 `VUSB` | Verify 5.0 V before connecting |
| DROK output ground | ESP32-C3 `GND` | All grounds are common |

### BH1750

| BH1750 pin | ESP32-C3 pin | Purpose |
| --- | --- | --- |
| `VCC` | `3.3V` | Sensor power |
| `GND` | `GND` | Ground |
| `SCL` | `D8` | I2C clock |
| `SDA` | `D9` | I2C data |

## Assembly

1. With power disconnected, adjust the DROK converter for a 5.0 V output using the 18 V supply and a multimeter. Disconnect power again after adjustment.
2. Mount the BH1750 on top of the ESP32-C3 as shown below. Keep the light-sensing component exposed and unobstructed.
3. Mount the DROK converter underneath the ESP32-C3 as shown below.
4. Add insulating material where needed so the stacked circuit boards and solder joints cannot short against one another.
5. Cut and route the hookup wires between the boards.
6. Solder all power and signal connections listed in the wiring tables.
7. Inspect the assembly for solder bridges, loose wire strands, reversed polarity, and mechanical strain.

## Photos

### BH1750 Top Mount

![BH1750 module mounted on top of the ESP32-C3](photos/bh1750-top.jpg)

### DROK Converter Bottom Mount

![DROK step-down converter mounted underneath the ESP32-C3](photos/drok-converter-bottom.jpg)

## Final Checks

Before applying power:

- Verify the 18 V input polarity.
- Confirm that the DROK output is 5.0 V.
- Confirm that 5 V connects to `VUSB`, not the `3.3V` pin.
- Confirm that the BH1750 receives 3.3 V.
- Check continuity between all ground connections.
- Confirm that power and ground are not shorted.
- Ensure the BH1750 sensing component is not covered by adhesive, wiring, or an enclosure.

After completing these checks, apply 18 V and verify that the ESP32-C3 starts normally and reports BH1750 lux readings.

## Safety Notes

- Disconnect power before soldering or changing wiring.
- Verify converter output voltage before attaching sensitive electronics.
- Insulate the exposed assembly before permanent installation.
- Add suitable input protection and strain relief when installing the sensor in its final location.

## Revisions

| Date | Change |
| --- | --- |
| 2026-07-18 | Initial build documentation |
