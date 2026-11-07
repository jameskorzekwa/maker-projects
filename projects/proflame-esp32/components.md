# Component inventory

Created 2026-11-06. Scope: one home Proflame prototype. `have` means James
confirmed availability, not that fit, electrical ratings or condition were tested.

| # | Component | Spec / acceptable substitute | Qty | Status | Source / option | Order ref | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | ESP32 board | NiceMCU ESP32-C5 | 1 | have | Owner confirmed | — | C5rev1.0 runtime-confirmed; 4 MB flash, native USB. Exact PCB pinout unverified. |
| 2 | USB data cable | Cable matching board's data-capable USB socket | 1 | have | Owner confirmed | — | Data link verified via successful flash; macOS ioreg 2026-11-06 returned controllers only, no USB serial port (pre-flash). Not a cable-failure diagnosis. |
| 3 | USB supply | Reputable regulated 5 V supply sized to board specification | 1 | have | Owner confirmed | — | USB power only; never connect 5 V to IFC |
| 4 | IFC pigtail | JST XH 2.5 mm, XHP-4-compatible housing and crimped leads | 1 | have | Owner confirmed | — | Verify actual IFC fit and each pin; colors are not evidence |
| 5 | Wire/connectors | Insulated low-voltage signal leads and secure board terminations | 1 set | have | Owner confirmed | — | Gauge, temperature rating and final length pending mounting plan |
| 6 | Insulation | Heat-shrink/individual unused-pin insulation | 1 set | have | Owner confirmed | — | Insulate unused 3V3 lead |
| 7 | Enclosure/strain relief | Nonconductive enclosure with cable retention, suitable temperature rating | 1 set | have | Owner confirmed | — | Location outside hot compartment not yet selected |
| 8 | Multimeter | DC voltage and continuity; appropriate for the work | 1 | have | Owner confirmed | — | Do not use resistance/continuity on energized equipment |
| 9 | Assembly tools | Wire stripper/cutter; soldering iron/solder or appropriate crimps as chosen | 1 set | need | Confirm exact termination method and tools | — | No soldering/crimping purchase until harness design verified |
| 10 | Mock UART interface | USB-UART adapter with verified 3.3 V TX/RX logic | 1 | to-buy | Amazon candidates below | — | Still needed for planned mock testing, not for current network bench |
| 11 | Logic analyzer | 3.3 V-compatible input, UART decode ≥9600 baud | 1 | need | Optional if mock adapter captures suffice | — | Defer purchase pending test design |
| 12 | Host computer | USB-capable computer for flashing and mock tests | 1 | have | Owner confirmed | — | Flashed on laptop through pinned SSH from bee2 |
| 13 | OEM dongle and handheld remote | Existing matched system | 1 set | need | Existing installation; retain and identify | — | Model labels and rollback check pending |

## Purchase-history check

Prior-purchase checks completed using Amazon and targeted non-Amazon Gmail
receipt searches. They did not establish any additional required equipment
beyond the owner-confirmed inventory above. Absence of a search match is not
proof of non-ownership. Personal order dates, order numbers and email contents
are intentionally not published in this public repository. There are no new
project orders.

No mock UART interface or logic analyzer purchases were made; the network-bench
firmware is deployed via USB Improv and does not require a mock UART adapter.
Future mock testing may require these items; tests against a mock precede IFC
connection. The previous Amazon-first candidates remain unapproved electrically:

| Option | Price on 2026-11-06 | Rating / reviews | Fit uncertainty |
| --- | --- | --- | --- |
| [HiLetgo CP2102](https://www.amazon.com/dp/B00LODGRV8) | $7.39 | 4.5 / 948 | TX voltage must be verified |
| [CP2102 three-pack](https://www.amazon.com/dp/B0FJRTL572) | $9.99 | 4.4 / 101 | Power-output claim does not establish logic voltage |
| [CP2102 cable](https://www.amazon.com/dp/B0GWDS3VR2) | $9.99 | 4.4 / 52 | Title claims 3.3 V TTL; isolate its 5 V power lead |

Anonymous prices exclude tax/shipping and may change. No new hardware was
purchased for this network-bench step. Confirm tool availability and electrical
fit before purchasing; reconcile future orders from 2026-11-06 onward.

## C5 reference docs

- [ESPHome ESP32 component init (2026.10.2)](https://github.com/esphome/esphome/blob/2026.10.2/esphome/components/esp32/__init__.py)
- [ESPHome logger component (2026.10.2)](https://github.com/esphome/esphome/blob/2026.10.2/esphome/components/logger/__init__.py)
- [ESP-IDF v5.5.6 ESP32-C5 USB Serial JTAG Console](https://docs.espressif.com/projects/esp-idf/en/v5.5.6/esp32c5/api-guides/usb-serial-jtag-console.html)
