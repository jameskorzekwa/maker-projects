# Electrical findings and wiring withdrawal — 2026-11-06

## Confirmed by owner measurement

Montigo P52DF-F, green-print Proflame IFC, OEM SIT dongle 0584526_01:
**+6.26 V DC**, red probe on white, black on yellow, **dongle plugged in**.
This is a loaded measurement. It contradicts applying the older 3.3 V supply
description to this unit. It does not establish a nominal voltage/tolerance,
available current, serial-signal voltage, or isolation from mains/earth.

**Earlier direct wiring and its SVG were withdrawn.** Do not connect white to
3V3, GPIO, USB/5V/VIN, or assume a generic divider/converter makes the whole
interface suitable. Keep the C5 separately powered and disconnected from X0A.
Signal directions remain hypotheses from the older source, not unit-level proof.
The firmware's GPIO4/GPIO5 assignments and successful compilation are unrelated
to electrical compatibility. No live firmware change or query accompanied this
documentation correction; the manual query button must not be used on an
unverified connection.

## What the document search established

| Source | Relevant finding | Limit |
| --- | --- | --- |
| [SIT 584 IFC product page](https://www.sitgroup.it/en/product/584-ifc-2/) | Green-print versions support Proflame Connect; diagnostic serial exists | No X0A supply/current/logic-level specification |
| [Montigo XG0681](https://www.empireinsider.com/wp-content/uploads/2024/06/xg0681-montigo-manual-sit-wifi-full.pdf) | Green-print IFC, OEM 0.584.526 and X0A relationship | OEM compatibility, not DIY electrical ratings |
| [SIT IFC catalogue](https://www.sitgroup.it/wp-content/uploads/2021/07/ProflameIFC-Catalogue.pdf), p.7 | 6 Vdc / 200 mA maximum listed under **BB supply voltage** | Battery-backup specification, **not an X0A accessory current rating** |
| [SIT IFC FFRT-5 instructions](https://www.sitgroup.it/wp-content/uploads/2021/04/584-IFC-FFRT-5-Use-and-Installation-Instructions.pdf) | Battery supply 6 V | Not an X0A specification |
| [OEM Wi-Fi instructions hosted by Mendota](https://mendotahearth.com/resources/wifi-manual-installation-16c75310.pdf) | Dongle attaches to X0A with supplied cable | No electrical thresholds or board schematic found |
| [Bonaparte](https://bonaparte.readthedocs.io/en/latest/system_information.html) | Older Napoleon eFIRE reverse engineering: 3V3/TX/RX/GND, 9600–8-N-1 | Not verified for this green-print controller; supply claim contradicted here |

No public exact-board schematic or X0A current/logic-level specification was
located for 0584526_01 / PCB 372_01A. This is a search limit, not proof that none
exists. The original proflame.sitgroup.it route failed DNS resolution; manufacturer
hosted/mirrored manuals were used instead.

## OEM components

- **FT232RL** is clearly marked in the photograph: USB-to-UART bridge, not proof
  of the X0A interface levels or an available supported USB control API.
- **ISO1/ISO2, 074L:** consistent with **ACPL-074L** dual CMOS optocouplers. The
  [Broadcom datasheet](https://docs.broadcom.com/doc/AV02-0963EN) specifies powered
  detector circuitry at 3.0–3.6 V or 4.5–5.5 V, and a 6.0 V absolute maximum.
  Do not conclude the measured 6.26 V is applied directly to those pins. The
  optocouplers alone do not establish board-level galvanic isolation or its purpose.
- The apparent **F100A** marking on U2 could not be matched reliably. Official TI
  package addenda rule out claiming LMR16010 (mark SB1P), LMR14020 (DB2SP),
  LMR16020 (SB2P), TPS5410 (5410/TPS5410) or LM5160A (12-pin WSON) as confirmed
  matches. Example: [LMR16010 datasheet/addendum](https://www.ti.com/lit/ds/symlink/lmr16010.pdf).
  Inductor/diode/capacitor placement is consistent with power conversion, but
  does not establish exact topology, output voltage or current rating.

## Decision and remaining work

Separate two design questions:

1. **Power:** an accessory supply around the measured voltage could power a C5
   through a suitable regulator, but allowable current, minimum/maximum voltage,
   battery-mode behavior and the board's permitted input must be established.
   No specific regulator is approved or required purchase yet.
2. **Signals/ground:** determine whether X0A uses 3.3 V logic, another level, or
   a current-driven/isolated interface, and whether a shared ground is permitted.
   A power buck converter does not answer this question.

No protocol capture is required to answer these electrical questions. The
remaining evidence must come from an exact interface specification or a reliable
circuit trace and electrical characterization. A DMM average on a switching
signal alone is insufficient to certify its maximum voltage or signaling type.

Possible alternative, **not implemented**: retain the OEM hardware and investigate
custom firmware on its ESP32, preserving its existing supply/interface circuitry.
That still requires verified pin assignments and recoverability and could lose
OEM functionality; it is not an approved shortcut or instruction to reflash it.
