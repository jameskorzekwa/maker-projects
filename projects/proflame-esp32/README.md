# Proflame ESP32 dongle replacement

A NiceMCU ESP32-C5 running ESPHome that replaces the SIT Proflame Connect Wi-Fi
dongle on James's home fireplace, talking directly to the Proflame 2 IFC over
its X0A diagnostic serial port. It exposes the fireplace to Home Assistant
through the native ESPHome API.

**Status (2026-11-07): working.** Communication, all supported controls and the
ESP32-side thermostat were tested on the fireplace with James present.

| | |
| --- | --- |
| Fireplace | Montigo P52DF-F (natural gas), green-print Proflame 2 IFC |
| Replaced dongle | SIT 0.584.526 (`0584526_01`), kept for rollback |
| Board | NiceMCU ESP32-C5 (chip rev 1.0, 4 MB flash, native USB) |
| Firmware | [esphome/proflame-bench.yaml](esphome/proflame-bench.yaml) + [esphome/ifc_query.h](esphome/ifc_query.h), ESPHome 2026.10.2 |
| HA device | **Proflame USB Bench** (`proflame-bench`), encrypted API, Wi-Fi logs, password OTA |

## Wiring

![X0A to ESP32-C5 wiring](wiring.svg)

| X0A wire (installed view, SW1 on the right) | Function | ESP32-C5 |
| --- | --- | --- |
| White | IFC 3.3 V output (owner measured 3.3 V) | **Not connected**, insulated |
| Green | IFC TX | **GPIO4** (RX) |
| Brown | IFC RX | **GPIO5** (TX) |
| Yellow | Ground | **GND** |

The ESP32 is powered separately over USB. UART 9600 baud, 8-N-1. Disconnect
the OEM dongle (never run both on X0A) and isolate fireplace mains and backup
batteries before touching the harness. Details: [wiring.md](wiring.md).

An early 6.26 V supply reading was a measurement error and was retracted; see
[electrical-findings.md](electrical-findings.md) for the history.

## Home Assistant entities

| Entity | Function |
| --- | --- |
| **Thermostat** (climate) | Heat/Off; presets Off, Manual, Thermostat, Smart; target temperature; room temperature |
| **Fireplace** (switch) | On = Manual at the last flame level; Off = everything off |
| **Mode** (select) | Off / Manual / Thermostat / Smart (same behavior as the climate presets) |
| **Flame Height**, **Fan Speed** (numbers, 0–6) | Burner flame and comfort fan |
| **Light** (switch) | The fireplace light, wired to the IFC AUX relay on this unit (on/off only) |
| **Light Follows Fireplace** (config switch) | Light turns on/off with the fireplace |
| **Pilot** (select), **Split Flow** (switch) | Continuous/intermittent pilot; split-flow valve |
| Fireplace Status, Thermostat Status, Room Temperature (+ Source) | Status |
| IFC Control, Last Command Result, Control Request Result, IFC Link, raw registers, Remote Room Temperature / Setpoint, Scan IFC Registers | Diagnostics |
| Lamp Output Level (unused) | IFC lamp output; nothing is connected to it here, hidden by default |

## Thermostat

The IFC's own thermostat modes depend on the handheld remote, which measures
the room and calls for heat. Selected from the Wi-Fi side they turn the light
on but never light the burner. So **Thermostat and Smart run on the ESP32**:

- Room temperature: `sensor.main_floor_temperature` from Home Assistant; falls
  back to the remote's last report to the IFC (register 0x1F) if that is
  unavailable or HA is disconnected. The fallback is only used if the remote's
  value changed within the last 2 hours. **Room Temperature Source** shows which
  is in use.
- Heats (Manual mode at the stored flame level) at ≥0.5 °F below target; goes
  idle at ≥0.5 °F above. **Idle = Manual with flame 0**, so the fan and light
  keep running. Smart lowers the flame one level per degree as the room
  approaches target.
- 5-minute minimum between automatic on/off changes. Changes you make (selecting
  a preset, changing the target) apply immediately.
- Turns everything off if no room temperature is available. Off requests are
  retried until the IFC confirms. A failed ignition cancels the thermostat.
  Using the remote cancels the thermostat. Mode and target persist across
  reboots.

## Protocol

Six-byte frames `[cmd, p0, p1, p2, register, XOR]`; GET `5E` → reply `AE`,
SET `5D` → echo `AD`.

| Register | Meaning | Source |
| --- | --- | --- |
| 0x16 | Control ownership. `C1 A0 00` = UART, `00 00 00` = remote | Documented |
| 0x17 | p1: mode bits 0–1 (0 off, 1 manual, 2 thermostat, 3 thermostat+power), undocumented bits 2–3, lamp bits 4–6, pilot bit 7 | Documented; bit layout corrected on this unit |
| 0x18 | p1: flame bits 0–2, AUX bit 3, fan bits 4–6, split flow bit 7 | Documented |
| 0x1E (0x21 mirror) | Remote thermostat setpoint, whole °F | Found by register scan |
| 0x1F | Remote's room temperature report, tenths °F | Found by register scan (provisional) |
| 0x00–0x24 | All answer GET; the rest are not yet identified | Register scan |

Behavior on this IFC:

- Using the remote switches control to the remote; 0x16–0x18 then read all
  zeros, shown in HA as unknown. Commands are refused until the remote turns
  the fireplace off, after which the ESP32 requests control once.
- The IFC echoes a control request even when refusing it. The grant is
  confirmed by the next 0x16 read.
- 0x1E/0x1F read correctly even while the remote has control.

## Firmware safety rules

- Nothing is written at boot or reconnect; background traffic is GET-only
  (the thermostat, when active, sends commands as described above).
- Each command is sent once, never queued or replayed. A command is refused
  while another is running.
- Before writing, the firmware verifies UART control and reads fresh state
  (control checks before and after the state read). Writes change only the
  targeted fields and keep undocumented bits. Every command is confirmed by
  read-back (**Last Command Result**).

## Build, update, test

Device Builder's `secrets.yaml` holds `wifi_ssid`/`wifi_password` and
`proflame_bench_api_key`/`proflame_bench_ota_password` (also in bee2 Secret
Service). No credentials are in this repository.

```sh
# Parser/encoder tests (ASan/UBSan)
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  tests/test_ifc_query.cpp -o /tmp/proflame-test && /tmp/proflame-test
# Build and install over Wi-Fi (needs secrets.yaml next to the YAML)
esphome compile esphome/proflame-bench.yaml
esphome upload esphome/proflame-bench.yaml --device proflame-bench.local
```

[esphome/proflame-usb-only.yaml](esphome/proflame-usb-only.yaml) is a minimal
USB-only image without networking or IFC communication.

## Rollback

Shut down and isolate the fireplace (mains and backup batteries), unplug the
ESP32 harness from X0A, reconnect the original Proflame Connect dongle, restore
power and confirm the remote and the original `proflame_connect_wifi`
integration work.

## Known limitations

- No dimmable lamp on this fireplace (the light is on the AUX relay).
- While the remote has control, HA can't see or control the fireplace.
- Room temperature staleness of `sensor.main_floor_temperature` is not detected
  unless the sensor goes unavailable.
- The meaning of most registers 0x00–0x24 is still unknown.

## Further documents

- [wiring.md](wiring.md): connector details and sources
- [electrical-findings.md](electrical-findings.md): supply measurements and component research
- [upstream-review.md](upstream-review.md): why the kaechele/esphome-proflame2 component isn't used as-is
- [commissioning.md](commissioning.md), [verification.md](verification.md), [components.md](components.md)
