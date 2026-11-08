# Fireplace knob battery add-on: components

List created 2026-11-07. Design: see `power.md`. The knob's I²C and UART
connectors on the USB-C daughter board are 4-pin MX1.25 at 5 V logic.

| # | Component | Spec / acceptable substitute | Qty | Status | Source / option | Order ref | Notes |
|---|---|---|---|---|---|---|---|
| 1 | Adafruit PowerBoost 1000C | LiPo charger + 5 V/1 A boost with load sharing and a USB-present pin (Adafruit #2465). Substitute must have load sharing and a USB-detect output. | 1 | to-buy | Adafruit $19.95 https://www.adafruit.com/product/2465 ; Amazon B09D1Y57FG $25.95 | | Powers the knob's USB-C; USB pin → knob UART RX (GPIO44) for battery detection |
| 2 | 3.7 V LiPo, 10,000 mAh, JST-PH 2.0 | 1S, protected, JST-PH 2.0 plug. 5,000 mAh is fine for ~half the runtime. | 1 | to-buy | Amazon B0GDQ8M6WP $17.99 (4.4★/58) ; 5,000 mAh B0GDQG4DYT $13.99 | | **Check plug polarity against the PowerBoost silkscreen before plugging in**: many Amazon packs are wired opposite to Adafruit's convention. Swap pins in the plug if needed. |
| 3 | MAX17048 fuel gauge breakout | I²C, with JST-PH battery in/out pass-through and 3–5 V logic (Adafruit #5679). | 1 | to-buy | Adafruit $5.95 https://www.adafruit.com/product/5679 ; Amazon B0G1MGT8F7 $13.25 | | Battery plugs into it, it passes through to the PowerBoost. I²C → knob I²C connector (5 V pull-ups; this breakout is 5 V tolerant). |
| 4 | MX1.25 4-pin pigtails | 1.25 mm pitch, 4 pin, ~100 mm | 2 | have | Cermant 40 pcs | Amazon 2026-10-26 | One for I²C (fuel gauge), one for UART (USB-present signal) |
| 5 | USB-C male bare-wire pigtail | 5 V power only, 2 wire, 20–24 AWG | 1 | to-buy | Amazon B0FNX18TNC 5 pcs $7.29 (4.6★/51) | | PowerBoost 5 V/GND → knob USB-C. Alternatively a short USB-A→USB-C cable into the PowerBoost's USB-A jack (no soldering). |
| 6 | Resistor 2.2 kΩ | any ¼ W, 1–4.7 kΩ | 1 | need | | | Pull-down on the USB-present line so it reads low when unplugged (the knob's UART pin has a 10 kΩ pull-up to 5 V) |
| 7 | JST-PH 2.0 pigtails | 2 pin, with wire | 2 | need | Amazon B0D7Q9HJLQ 20 sets $6.99 | | Only if the fuel gauge/charger kit lacks them or the pack polarity must be rebuilt |
| 8 | Hookup wire 22–26 AWG, heat-shrink | silicone wire | – | need | | | The 16 AWG from 2026-10-24 is too thick for these pads |
| 9 | Soldering iron + solder | | – | need | | | For items 5–7 |
| 10 | Enclosure / battery holder | 3D-printed back for the knob mount holding the pack, PowerBoost and gauge | 1 | need | print on the X1C | | Keep the LiPo away from the fireplace's heat; the pack is rated to ~60 °C |

Estimated to buy: $52–60 from Adafruit + Amazon (PowerBoost + gauge from
Adafruit, battery + pigtails from Amazon), or ~$75 all from Amazon.
