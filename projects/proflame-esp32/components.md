# Component inventory

Created 2026-11-06. Scope: one home Proflame prototype. `have` means James
confirmed availability, not that fit, electrical ratings or condition were tested.

| # | Component | Spec / acceptable substitute | Qty | Status | Source / option | Order ref | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | ESP32 board | Seeed Studio XIAO ESP32-S3 | 1 | have | Owner confirmed | — | Exact spare-board identity and existing firmware to check |
| 2 | USB data cable | XIAO USB-C, data-capable | 1 | have | Owner confirmed | — | Bench upload and logging |
| 3 | USB supply | Reputable regulated 5 V supply sized to board specification | 1 | have | Owner confirmed | — | USB power only; never connect 5 V to IFC |
| 4 | IFC pigtail | JST XH 2.5 mm, XHP-4-compatible housing and crimped leads | 1 | have | Owner confirmed | — | Verify actual IFC fit and each pin; colors are not evidence |
| 5 | Wire/connectors | Insulated low-voltage signal leads and secure board terminations | 1 set | have | Owner confirmed | — | Gauge, temperature rating and final length pending mounting plan |
| 6 | Insulation | Heat-shrink/individual unused-pin insulation | 1 set | have | Owner confirmed | — | Insulate unused 3V3 lead |
| 7 | Enclosure/strain relief | Nonconductive enclosure with cable retention, suitable temperature rating | 1 set | have | Owner confirmed | — | Location outside hot compartment not yet selected |
| 8 | Multimeter | DC voltage and continuity; appropriate for the work | 1 | have | Owner confirmed | — | Do not use resistance/continuity on energized equipment |
| 9 | Assembly tools | Wire stripper/cutter; soldering iron/solder or appropriate crimps as chosen | 1 set | need | Confirm exact termination method and tools | — | No soldering/crimping purchase until harness design verified |
| 10 | Mock UART interface | USB-UART adapter with verified 3.3 V TX/RX logic | 1 | to-buy | Amazon candidates below | — | Owner did not select UART test equipment; not required for USB-only test |
| 11 | Logic analyzer | 3.3 V-compatible input, UART decode ≥9600 baud | 1 | need | Optional if mock adapter captures suffice | — | Defer purchase pending test design |
| 12 | Host computer | USB-capable computer for flashing and mock tests | 1 | need | Confirm which computer will be used | — | bee2 cannot flash laptop-attached USB directly |
| 13 | OEM dongle and handheld remote | Existing matched system | 1 set | need | Existing installation; retain and identify | — | Model labels and rollback check pending |

## Purchase-history check

Prior-purchase checks completed using Amazon and targeted non-Amazon Gmail
receipt searches. They did not establish any additional required equipment
beyond the owner-confirmed inventory above. Absence of a search match is not
proof of non-ownership. Personal order dates, order numbers and email contents
are intentionally not published in this public repository. There are no new
project orders.

## Amazon-first mock-interface candidates — NOT approved electrical substitutes

Anonymous search snapshot 2026-11-06; prices/delivery may differ by account and
location. A 3.3 V *power output* does not establish 3.3 V *TX logic*. Verify the
datasheet and measure the actual adapter before using it; leave adapter power
leads disconnected when the peer is separately powered. These are candidates,
not an instruction to connect them to the IFC.

| Option | Price | Rating / reviews | Fit uncertainty |
| --- | --- | --- | --- |
| [HiLetgo CP2102](https://www.amazon.com/dp/B00LODGRV8) | $7.39 | 4.5 / 948 | Search title does not specify TX voltage; must verify |
| [CP2102 three-pack](https://www.amazon.com/dp/B0FJRTL572) | $9.99 | 4.4 / 101 | 3.3 V/5 V output claim may refer to power, not logic |
| [CP2102 cable](https://www.amazon.com/dp/B0GWDS3VR2) | $9.99 | 4.4 / 52 | Title explicitly claims 3.3 V TTL, but includes a 5 V power lead: isolate it |

Immediate USB-only preparation: **$0 additional parts if confirmed items are
suitable**. Later mock-interface allowance: $7.39–$9.99 before tax/shipping,
excluding any still-unconfirmed tools. No final connected-build cost yet.
No purchases made. Before buying, confirm rows 9, 11 and 12 and verify the
adapter's logic levels. Reconcile future orders from 2026-11-06 onward.
