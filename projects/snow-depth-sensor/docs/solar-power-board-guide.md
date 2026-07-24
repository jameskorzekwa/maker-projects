# Solar Power Board Guide — CN3791/CN5305 MPPT Manager

Setup, wiring, and acceptance testing for the **WatangTech** MPPT solar
manager board (["MPPT Solar Charge Controller for 18650"](https://a.co/d/08eKKVoD),
CN3791 charger + CN5305 boost) that powers the snow depth sensor. This board
replaced the Waveshare Solar Power Manager (D) on 2026-07-23 — and is, in a
full-circle twist, the board the project's original July 2026 plan was
designed around.

## Complete power wiring

```text
POWER (all grounds are one common net)
18650 cell ──> MPPT board onboard holder (PH-2P socket unused)
MPPT board 3.3 V header ──> ATOM Echo 3.3 V pin  (ALWAYS ON — the ATOM 5 V pin
                                                  is no longer used)
MPPT board 5 V header ──> Pololu #2810 VIN       (switched MB7389 supply)
MPPT board GND ──┬──> ATOM Echo GND
                 ├──> Pololu #2810 GND
                 └──> MB7389 pin 7 (GND)
Solar panel + ──> MPPT board solar terminal +   (direct; MPPT DIP at 5 V)
Solar panel − ──> MPPT board solar terminal −

MONITORING HEADER (reclaimed ATOM audio pins; 21 µA divider drain)
MPPT VBAT ── 100 kΩ ──┬── 100 kΩ ── GND
                     └──> ATOM G33 (battery voltage ADC, ×2 in firmware)
MPPT CHRG ── 10 kΩ ──> ATOM G19   (charging, active low)
MPPT DONE ── 10 kΩ ──> ATOM G22   (charge complete, active low)
```

## Background and procedure

*Documented 2026-07-23.*

**Why:** the Waveshare (D)'s output stage is a power-bank SoC (SW6106). Within 48 hours it shut the ESP32 down three ways: light-load auto-off during deep sleep, silent low-current-mode dropout, and output kill on a charge-source event — the last with the battery at 9.5%, causing a deep discharge and protection latch. These are design behaviors and cannot be disabled. The replacement board (CN3791 MPPT solar charger + CN5305 plain boost — ["MPPT Solar Charge Controller for 18650"](https://a.co/d/08eKKVoD)) has no load detection, explicitly supports simultaneous charge and discharge, and specifies <4 mA standby.

### Board setup before wiring

1. MPPT SET DIP switch: **5 V position ON, all other switches OFF** (set and confirmed 2026-07-24). One switch only, never two at once. The 5 V set-point matches the panel, so the raw solar-terminal route is used — no USB-C input, no cable splice.
2. Battery switch: OFF until wiring is complete.
3. Battery: 18650 in the onboard holder (negative end first, observe polarity); the PH-2P socket is unused.
4. Never feed the solar input and the USB Type-C charge input at the same time.

### Rewire procedure

1. Power down: unplug any USB-C charger; disconnect the battery pigtail from the Waveshare BAT terminals; remove the Waveshare 5 V OUT → ATOM 5 V wire entirely (the ATOM 5 V pin is no longer used); free the INA219 VIN− wire from the Waveshare SOLAR IN+.
2. Battery: plug the pigtail from the MAX17043 P1 terminals into the MPPT board's PH-2P battery socket. **Verify polarity with a meter before connecting.**
3. ESP32 power: MPPT board 3.3 V header (+) → ATOM Echo 3.3 V pin; MPPT board GND → ATOM GND. This rail is always on: no chip logic can shut the ESP32 off.
4. Sensor power: MPPT board 5 V header (+) → Pololu #2810 VIN (reuse the existing wire). Pololu VOUT → MB7389 pin 6, GPIO26 → Pololu ON, 100 kΩ pulldown: all unchanged.
5. Solar: panel + → INA219 VIN+; INA219 VIN− → MPPT board solar terminal +; panel − → terminal −. Alternative: panel USB-C into the board's Type-C with the INA219 spliced into the cable's VBUS; then leave the terminal block empty.
6. Grounds: all common (board GND, ATOM, Pololu, MB7389 pin 7, INA219, MAX17043).
7. Battery switch ON. If the OUTPUT ON indicator does not light, press the activation button once. That press is needed only after connecting a battery or after a 2.9 V deep-discharge cutoff — not routinely.
8. The ESP32 boots immediately and resumes the normal 60 s / 15 min cycle. **No firmware changes are required.**

**USB flashing caveat (updated):** the ATOM is now fed through its 3.3 V pin. Disconnect the 3.3 V feed from the MPPT board before plugging USB into the ATOM, and reconnect it afterward.

### Burn-in acceptance test before outdoor install

- [ ] 24 hours of unattended wake/sleep cycles on battery only — outputs must survive near-zero-load sleep windows (the failure that killed the Waveshare on 2026-07-21).
- [ ] Plug and unplug the USB-C charger during a sleep window — output must survive charge events (the failure of 2026-07-23).
- [ ] Standby drain ~4–6 mA total, confirmed by overnight voltage slope (at ~5 mA the single 2,000 mAh cell gives about two weeks without sun).
- [ ] MAX17043 stays continuously powered on the always-on 3.3 V rail — battery percentage should finally hold calibration.

### Notes

The Waveshare is fully retired (usable as a spare USB-C charger board). The CHRG/DONE/VBAT monitoring header is in service on reclaimed ATOM audio pins (see the monitoring section of the wiring diagram). The hardware low-temperature charge cutoff for winter remains an open item.

## Related documents

- Sensor-side wiring, ESPHome configuration, mounting, and calibration: [MB7389 sensor guide](mb7389-sensor-guide.md)
- The retired Waveshare configuration this replaces: [Waveshare-era wiring guide](waveshare-wiring-guide.md)
