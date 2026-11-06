# Proflame ESP32 dongle replacement

Status: **USB bench preparation only; NOT ready to connect to the fireplace.**
Created 2026-11-06 for James's home Home Assistant server (not the Casita).
This directory is the maintained project record. Live ESPHome configurations
eventually belong in the home server's `/config/esphome/`; nothing here has
been deployed there. The existing OEM dongle and HA integration stay unchanged.

## Goal and decisions

Replace the OEM Wi-Fi dongle with locally maintained ESPHome firmware, retaining
the original Proflame IFC and all appliance safety controls. James confirmed:

- Available board: **NiceMCU ESP32-C5**.
- **Normal handheld-remote operation must remain available.** Do not unpair it.
- USB cable/supply, JST XH four-pin pigtail, multimeter, wiring/insulation and
  enclosure/strain relief are already available; see [components.md](components.md).

Target architecture: HA native ESPHome API → ESP32 → IFC diagnostic UART.
This would replace the OEM Wi-Fi integration for this fireplace, not run two
UART masters in parallel. There is no claim of OEM feature parity, confirmed
burner feedback, or automatic shutdown when connectivity is lost.

## USB-only bench preparation

The firmware uses a generic `esp32c5` target, not a verified NiceMCU board
definition. `flash_size: 4MB` is a conservative build allocation, **not a measured
capacity**. Compile now, but **stop before flashing** until the board's C5 chip,
at least 4 MB of flash and native USB connection have been confirmed. The exact
NiceMCU model/revision and USB routing remain unknown. Laptop inspection on
2026-11-06 returned USB controllers only and no USB serial port. James reports
the board attached, but it did not enumerate during that inspection. Reconnect
with a known data cable before attempting identification; cable failure has not
been diagnosed.

1. Leave the fireplace, OEM dongle and remote exactly as they are. Disconnect
   **every external GPIO lead** from the C5; only its USB cable is connected.
   Flashing replaces whatever firmware is on this board: use a spare board or
   preserve its previous source/firmware first.
2. Use ESPHome **2026.10.2** on the computer physically connected to the board.
   Commands executed on bee2 cannot access a USB board plugged into a laptop.
3. From this directory, run:

   ```sh
   esphome version
   esphome config esphome/proflame-bench.yaml
   esphome compile esphome/proflame-bench.yaml
   # STOP: the hardware/USB identification gate above must pass first.
   # Replace PORT with the actual port on the USB-connected computer.
   esphome upload esphome/proflame-bench.yaml --device PORT
   esphome logs esphome/proflame-bench.yaml --device PORT
   ```

4. Confirm the `USB-only bench alive; no IFC connection permitted` message
   repeats every ten seconds. Observe for 15 minutes and power-cycle the board;
   record resets, USB behavior and firmware version in [verification.md](verification.md).

The bench firmware intentionally has **no Wi-Fi, API, OTA, UART, Proflame
component or actuator entities**. It needs no credentials and cannot test the
fireplace protocol or HA integration. It is not a functioning dongle.

## Why the ready-made component is not enabled

The candidate [kaechele/esphome-proflame2](https://github.com/kaechele/esphome-proflame2)
was reviewed at `bb67453c8782385e2b895274d482e60eb4808934`.
It implements the wired protocol, but source review found automatic control
acquisition and a command-queue timestamp-copy defect. Silent-link handling,
state freshness, and light feedback also require work. Details and the pinned
source are in [upstream-review.md](upstream-review.md).

**Do not copy the upstream example onto a connected fireplace.** Even a
configuration exposing only its diagnostic sensor requests control at startup.
No ready-to-flash fireplace-control configuration is included here deliberately.

## Hardware identification and proposed wiring

Still needed: fireplace make/model, IFC part number/revision and connector-label
photo, existing dongle label/connection, and handheld-remote model. Photograph
only safely accessible labels; do not open energized equipment. Add non-sensitive
photos here after review. No appliance-specific wiring has been verified.

The upstream describes X0A as a four-pin JST XH connector with IFC signals
`3V3 / TX / RX / GND`, with orientation established by its printed ground dot.
That is **not a universal left-to-right hookup instruction**. Verify the actual
IFC and its manual before fabricating or plugging in a harness.

| Candidate connection, NOT installation authorization | Purpose |
| --- | --- |
| C5 USB (GPIO13 D− / GPIO14 D+) | Native USB; exact NiceMCU PCB routing to be confirmed |
| C5 UART TX/RX | Unassigned; verify the exact PCB before choosing pins |
| C5 GND ↔ verified IFC GND | Only after confirming the interface grounding is suitable |
| IFC 3V3 | Leave disconnected and individually insulated |
| Board USB power | Separate regulated USB power; no 5 V lead to IFC |

TX/RX pins for UART IFC remain TBD pending the exact NiceMCU C5 PCB pinout; do
not assume USB connector routes to the native USB pins or any other board pinout.
Reserve GPIO13/14 for native USB; do not assign either to IFC UART. A USB-to-UART
bridge may allow flashing but will not carry native `USB_SERIAL_JTAG` logs.
Historical note: the prior XIAO ESP32-S3 bench used D6 / GPIO43 and D7 / GPIO44
as candidate UART pins—these do not apply to the C5. Do not connect RS232, RS485
or 5 V TTL to this interface. Never parallel the OEM dongle and ESP32 TX lines.
Do not power the ESP32 from the IFC accessory output: its available current has
not been verified. Connector colors and premade Grove adapters are not pinout
evidence.

## Implementation gates

1. **Identify hardware:** verify X0A, electrical levels/grounding, pin orientation,
   appliance instructions and a safe mounting/power arrangement. A qualified
   fireplace technician should resolve uncertain appliance-side electrical work.
2. **Harden and test firmware against a mock, not the fireplace:** fix queue
   expiry/bounds, stale state, unsolicited state-feedback writes and command
   acceptance. Boot and network recovery must not acquire control or replay
   actuator commands automatically. Require explicit arming and fresh state.
3. **Meet the remote requirement:** prove normal handheld control, clear ownership
   and truthful status on this IFC. Upstream says an active remote prevents UART
   control and may leave stale reads. If that cannot meet the requirement, stop
   and evaluate RF emulation separately; do not silently disable the remote.
4. **Review and compile the resulting control firmware:** no automatic upgrade
   from the pinned candidate. Add protocol/mock regression tests and independent
   review before any IFC-connected trial.
5. **Supervised commissioning:** follow [commissioning.md](commissioning.md).
   Hardware acceptance and a supported independent shutdown method are mandatory.

## HA integration and rollback

Only after these gates: add encrypted ESPHome native API connectivity using the
approved credential provisioning mechanism. Do not commit or log credentials.
Neither a Wi-Fi profile nor credentials are provisioned in this project.
Add a clearly named test device; keep all fireplace automations disabled while
commissioning. Do not reuse the old entity IDs until behavior and units are
verified. Preserve the OEM dongle, its configuration and the paired remote.

Rollback is to shut down using the appliance's documented procedure, isolate
power as instructed by its manufacturer, remove the DIY harness, restore the OEM
connection, then verify remote and original HA operation. Do not hot-plug the IFC.
Do not remove the current HA integration until the replacement is accepted.

## Safety

The IFC retains gas/ignition/flame-sensing safety; never bypass it, switch gas
valves directly, or modify appliance interlocks. Retaining the IFC does not make
a DIY accessory manufacturer-approved. Software cannot substitute for appliance
safety controls, supervision during testing, or functioning CO/smoke alarms.
Loss of HA/Wi-Fi/ESP power is **not** an established off command. Mount electronics
and cables outside hot areas in accordance with the appliance instructions.

## Revisions

| Date | Change |
| --- | --- |
| 2026-11-06 | Switched board from XIAO ESP32-S3 to NiceMCU ESP32-C5; removed S3 pin recommendations; C5 flash/USB unverified; USB detection attempted no device. |
| 2026-11-06 | Initial project, owner-confirmed inventory, USB-only bench firmware, upstream risk review and gated implementation plan. No live deployment. |
