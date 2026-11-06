# X0A wiring and manual link check

![X0A to C5 wiring](wiring.svg)

[Open/download the SVG](wiring.svg).

## Connections

Use **GPIO numbers**, not header-position numbers or unverified D4/D5 aliases.
The drawing intentionally is not a physical NiceMCU header layout: exact PCB
header locations and onboard connections have not been inspected. Only use
GPIO4/IO4 and GPIO5/IO5 if those pins are exposed and otherwise unused on your board.

Looking at the installed X0A connector from the wire-entry side as in James's
close-up, with the red SW1 button on the right:

| Position left → right | Existing wire | IFC function | ESP32-C5 destination |
| --- | --- | --- | --- |
| 1 | White | 3.3 V output | **Not connected; individually insulate** |
| 2 | Green | TX (IFC output) | **GPIO4 / IO4**, UART RX |
| 3 | Brown | RX (IFC input) | **GPIO5 / IO5**, UART TX |
| 4, filled-dot end | Yellow | Ground | **GND** |

Fireplace connector housing: **JST XH four-position XHP-4**, nominal 2.5 mm
pitch. Do not substitute a PH/Grove cable or assume replacement wire colors
match. The disconnected plug's mating-face view is mirrored. Do not change
the cavity order based on an unlabelled photo of the opposite end of the cable.

Shut down and isolate fireplace mains and backup batteries according to the
appliance manual, and disconnect ESP USB power before wiring. Remove the OEM
dongle from X0A; never parallel two transmitters. Power the ESP separately by
USB; no ESP 5 V/VIN/3V3 pin connects to X0A. Do not press SW1 or unpair the remote.
Keep electronics outside hot areas and retain the original dongle for rollback.

## Evidence and limits

The fireplace rating plate identifies **Montigo P52DF-F**, natural gas. The OEM
dongle is **0584526_01**. The photograph shows the green-print IFC and X0A's
rightmost filled dot. Wire colors above are from James's photographs.

- [Montigo XG0681 Wi-Fi installation instructions](https://www.empireinsider.com/wp-content/uploads/2024/06/xg0681-montigo-manual-sit-wifi-full.pdf), pp. 2–3: green-print IFC, SIT 0.584.526 dongle, X0A connection.
- [Bonaparte firsthand reverse engineering](https://bonaparte.readthedocs.io/en/latest/system_information.html#sit-proflame-2-uart-communication): filled dot identifies GND; IFC pinout `3V3 | TX | RX | GND`, 9600–8-N-1.
- [ESPHome Proflame component hardware instructions](https://github.com/kaechele/esphome-proflame2#hardware-setup): XHP-4 and direct crossed UART. Same author as Bonaparte, not independent measurement.
- [Espressif ESP32-C5 datasheet](https://documentation.espressif.com/esp32-c5_datasheet_en.html): GPIO4/5 permit GPIO-matrix UART routing; they are also external JTAG MTCK/MTDO. Do not simultaneously use pad JTAG. Native USB uses GPIO13/14.

This is **source-derived wiring, not an electrically measured pinout on this
exact IFC**. Manufacturer sources establish OEM compatibility, not approval of
this DIY interface. Exact signal levels, grounding/isolation requirements and
NiceMCU header assignments have not been measured. The OEM dongle's ISO1/ISO2
markings are consistent with ACPL-074L optocouplers, but that alone neither proves
nor disproves a need for isolation at X0A. Do not energize uncertain wiring based
on color alone; resolve electrical/board differences before connection.

## Firmware behavior

`proflame-bench.yaml` configures **RX GPIO4, TX GPIO5, 9600 baud, 8-N-1**.
It retains runtime-provisioned Wi-Fi and encrypted API diagnostics.

The **Check IFC Link** button sends exactly one documented GET-ownership request:
`5E 00 00 00 16 48`. It does not request ownership, ignite the burner or set any
operating state. There are no automatic boot/reconnect queries, retries, SET
commands or actuator entities. Do not automate this button.

**Last IFC Query Result** records the last query attempt, not live link health
or actual burner activity. Valid responses must have six bytes, opcode AE,
register 16 and a matching XOR checksum. A non-restored pending flag and a
one-second deadline bound each query; repeated presses while the script runs
are ignored. Startup has no result until queried. Unrecognized ownership payloads
are reported without interpretation.

The debugger-based receiver is a commissioning aid, not a robust stream parser:
noise or fragmented delivery can cause a timeout, and the protocol has no
transaction ID to distinguish a delayed reply arriving during a later query.
**No valid reply does not prove incorrect wiring.** The firmware cannot establish
electrical safety or confirmed flame state. No IFC-connected query has been tested
as part of this implementation.

## Build and test

Copy **both** `esphome/proflame-bench.yaml` and `esphome/ifc_query.h` into the same
ESPHome configuration directory. Do not include the unsafe upstream component.

From this project directory:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  tests/test_ifc_query.cpp -o /tmp/proflame-query-test
/tmp/proflame-query-test
esphome config esphome/proflame-bench.yaml
esphome compile esphome/proflame-bench.yaml
```

Validated and compiled with ESPHome 2026.10.2 / ESP-IDF 5.5.6. Parser tests pass
with address/undefined-behavior sanitizers. Build success is not an appliance test.
