# Fireplace knob battery add-on: components

List created 2026-11-07; **re-planned 2026-11-10 around the M5Stack Dial v1.1**,
which has the LiPo connector, charger (TP4057, 100 mA) and a hardware power
latch built in. The CrowPanel plan (PowerBoost 1000C + MAX17048) is retired.
Design: see `power.md` (Dial section).

| # | Component | Spec / acceptable substitute | Qty | Status | Source / option | Order ref | Notes |
|---|---|---|---|---|---|---|---|
| 1 | M5Stack Dial v1.1 | ESP32-S3, 1.28" 240×240, encoder, LiPo socket + charger, 6–36 V in | 1 | have | M5Stack / Amazon | bought 2026-11-10 | Running `fireplace-dial-living-room.yaml` |
| 2 | 3.7 V LiPo with **Micro JST 1.25 mm 2-pin** plug, with protection (PCM) | 1S, 500–1,200 mAh, ≤ ~50×20×10 mm to fit the housing; 1.25 mm plug (NOT the 2.0 mm JST-PH) | 1 | to-buy | Amazon B0FRFYJ32K 102050 1,000 mAh $8.19 (4.8★/6) ; MakerHawk 803040 1,000 mAh ×2 B0FZSR2PM9 $14.99 (4.7★/81) | | **Check polarity**: Dial socket pin 1 = +, pin 2 = GND, but the socket's pin-1 orientation is not marked; compare the pack's red wire to the board before plugging in. Charger is 100 mA, so a 1,000 mAh pack takes ~10 h to fill. |
| 3 | James's existing JST-PH 2.0 LiPo | | 1 | have | | | Usable only with a PH-2.0 → 1.25 mm adapter (item 4) or by re-terminating the plug. |
| 4 | Micro JST 1.25 mm 2-pin pigtails | for re-terminating / an adapter | 2 | need | Amazon "JST 1.25mm 2 pin" ~$6–9 | | James has 1.25 mm **4-pin** pigtails (Cermant); the 2-pin housing is different. Only needed if item 2 is skipped. |
| 5 | PowerBoost 1000C | | – | not-needed | | | Dial has its own charger/latch |
| 6 | MAX17048 fuel gauge (Newfame, B0HJ94TP3L) | I²C 0x36 | 1 | have (removed) | Amazon | 114-8797990-9903459 | Used 2026-11-10 only; voltage-only gauge cannot detect charging. Replaced by the INA219. |
| 8 | INA219 current/voltage monitor (ACEIRMC 10-pack, B0CTYB69B5) | I²C 0x40, 0.1 Ω shunt, in series with the battery + lead | 1 | have | Amazon | 2026-11-10 | Battery → VIN+, VIN− → B+, VCC from the battery tap, GND, SDA/SCL → Port A G13/G15. Measures current with direction: charging, external power, and the coulomb-counted % all come from it. |
| 7 | 2.2 kΩ resistor, UART pigtail, USB-C pigtail | | – | not-needed | | | Were for USB detection on the CrowPanel; the Dial detects battery by releasing its latch |

To buy: **one 1.25 mm-plug LiPo (~$8–15)**. Everything else is on hand or unnecessary.

## Basement knob: CrowPanel 1.46 (DHR55146D, board V1.2)

List created 2026-11-20. The V1.2 board has **no working battery circuit**:
the J8 "BAT" pads are a bare footprint (− pad = GND, + pad open to everything;
0 V on the pads with USB on; the board does not start from a cell on J8).
Elecrow lists the board as 5 V/1 A input only. Battery power therefore comes
from an external charger/boost that feeds the knob's USB-C with 5 V.

Design: USB → PowerBoost 1000C (charges the cell, load sharing) → 5 V out →
USB-C pigtail → knob. A spare INA219 in the cell's + lead (cell → VIN+,
VIN− → PowerBoost BAT) on the knob's I2C socket keeps the current firmware
(`hw-crowpanel146.yaml`) unchanged: + = charging, − = running on the cell.

| # | Component | Spec / acceptable substitute | Qty | Status | Source / option | Order ref | Notes |
|---|---|---|---|---|---|---|---|
| 1 | CrowPanel 1.46" rotary display DHR55146D | ESP32-S3, 360×360 | 1 | have | Elecrow | #321281 (2026-11-09) | Running `fireplace-knob-basement.yaml` |
| 2 | Adafruit PowerBoost 1000C (product 2465) | LiPo charger + 5.2 V/1 A boost, load sharing, no light-load shutoff; ~36×23 mm | 1 | to-buy | Amazon B00PY2YTVU $26.95 (4.4★/51); clone B0GTLXV7HC $18.99 (no reviews) | | Micro-USB input (or solder a USB-C panel socket to its USB/GND pads); JST-PH 2 mm battery socket; 5 mA quiescent |
| 3 | 3.7 V LiPo, EEMB LP103454 2,000 mAh | 1S with protection | 1 | have | | | Now wired as bare leads; check the PowerBoost JST-PH polarity before plugging |
| 4 | INA219 module (ACEIRMC 10-pack, B0CTYB69B5) | I²C 0x40, R100 (0.1 Ω) shunt | 1 | have (spare from pack) | Amazon | 2026-11-10 | The first one fitted reads garbage on IN+ (169 mV with 0 A); use a fresh one |
| 5 | USB-C male pigtail, 4-wire (JSADZKJ 90°) | PowerBoost 5 V/GND → knob USB-C | 1 | probably-have | Amazon | 112-8378582-2448255 (2026-11-08) | |
| 6 | USB-C panel-mount socket (10-pack) | optional USB-C input for the PowerBoost | 1 | probably-have | Amazon | 112-9179124-3267407 (2026-11-18) | Only if a micro-USB input is not wanted |
| 7 | MX1.25 4-pin cable to the I2C socket | GND/5V/SDA/SCL | 1 | have | in use | | |
| 8 | MAX17048 fuel gauge (Newfame) | | – | not-needed | | 114-8797990-9903459 | Voltage-only, cannot detect charging (see power.md) |
| 9 | Wire, solder, heat-shrink | 22–26 AWG | – | have | | | |

To buy: **PowerBoost 1000C**.
