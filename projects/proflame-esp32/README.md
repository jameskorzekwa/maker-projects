# Proflame ESP32 dongle replacement

Status: **UART commissioning firmware deployed (Wi-Fi + encrypted API + manual
read-only query). No fireplace control. Appliance electrical compatibility and
physical NiceMCU header locations remain unverified.**
Created 2026-11-06 for James's home Home Assistant server (not the Casita).
This directory is the maintained project record. The network-bench configuration
is installed in the home server's `/config/esphome/`, and the firmware is running
on the board. The existing OEM dongle and fireplace integration stay unchanged.

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

## Wiring: three connections, fourth wire insulated

**[Open the SVG wiring diagram](wiring.svg)** · **[Detailed wiring and sources](wiring.md)**

| Existing X0A wire | C5 connection |
| --- | --- |
| Green, IFC TX | **GPIO4 / IO4 — RX** |
| Brown, IFC RX | **GPIO5 / IO5 — TX** |
| Yellow, GND | **GND** |
| White, IFC 3.3 V output | **Not connected; insulate** |

These are logical GPIO labels, not header positions or D4/D5 aliases. Confirm
GPIO4/5 are exposed and unused on the actual PCB. Source-derived X0A mapping
uses the installed wire-entry view with SW1 to the right; the mating face is
mirrored. Isolate mains and battery backup before wiring and unplug ESP USB.
Use separate USB power; never connect ESP power pins to X0A or parallel the OEM
dongle's transmitter. Exact unit electrical levels/grounding have not been measured.

The **Check IFC Link** HA button sends one GET-status request, with no SET or
ownership acquisition. There is no automatic boot/reconnect traffic or retry.
**Last IFC Query Result** is historical query feedback, not current burner state.
The agent has **not pressed this button** or performed an IFC-connected test.

## Firmware files and rollback

Two ESPHome configs exist in this project:

| File | Purpose | Network | API | OTA |
| --- | --- | --- | --- | --- |
| [proflame-bench.yaml](esphome/proflame-bench.yaml) + [ifc_query.h](esphome/ifc_query.h) | **Current UART commissioning image** — network diagnostics and manual read-only IFC query. Deployed. | WiFi (Improv over USB) | Encrypted (`api.encryption: {}` — zero-key bootstrap, unique key must be provisioned) | No |
| [proflame-usb-only.yaml](esphome/proflame-usb-only.yaml) | **Historical USB-only rollback** — no WiFi, no API, no UART. Diagnostic log only. | None | None | None |

The USB-only file is a safe rollback: it removes all network connectivity and
cannot drive the fireplace. The network-bench image is the current deployed
firmware.

### Network-bench build and flash (current)

This records a supervised deployment, not a self-contained provisioning tool.
The provisioning scripts used for this deployment are session-private and are
not released with this project. Arrange and verify the runtime provisioner
before enabling Wi-Fi, so the bootstrap API is not left exposed.

The firmware uses a generic `esp32c5` target, not a verified NiceMCU board
definition. `flash_size: 4MB` now matches the **4 MB detected by esptool**.
The board was confirmed as C5rev1.0 with 4 MB flash and native USB
connection; exact NiceMCU PCB header layout remains unverified. UART GPIO4/5
are now assigned in firmware. Keep the YAML and `ifc_query.h` in the same directory.

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
   # Replace PORT with the actual port on the USB-connected computer.
   esphome upload esphome/proflame-bench.yaml --device PORT
   ```

4. Close any serial log/flash view so the port is free. After upload, connect
   via USB Improv to provision Wi-Fi credentials, then
   provision a unique API encryption key into the ESP32's saved preferences
   using the native API. Keep the key in approved secure storage, then use it
   for HA adoption. This requires the supported runtime provisioning client;
   merely adding this YAML does not create a private key.
   Record resets, USB behavior and firmware version in [verification.md](verification.md).

### Network-bench operational notes

- **API encryption:** `api.encryption: {}` starts in zero-key bootstrap mode,
  accepting both the public zero-key and the deprecated plaintext API. This is
  **not** authenticated against an active LAN attacker. Immediately provision a
  unique key on a trusted network after first connection, or remove power on
  failure. Verify the unique key is installed and the zero key is rejected;
   an ordinary reboot retains the unique key (verified). Erasing preferences
   or some factory-flash operations can restore bootstrap mode.
- **Logger level NONE:** Improv debug logs can include Wi-Fi passwords; native
  USB serial logging is suppressed. Native USB baud rate remains 115200.
- **No OTA, web server, AP, or IFC actuator controls.** Network diagnostics plus
  manual IFC GET query only; details and parser limitations are in [wiring.md](wiring.md).
- **Full factory flash may erase provisioned preferences** (Wi-Fi, API key).
   Repeat Wi-Fi provisioning via physical USB Improv and API-key provisioning
   over the trusted network. USB provisions Wi-Fi, not the API key.
- **No long burn-in or IFC tests performed.**

### USB-only rollback

To remove all network connectivity, compile and flash `proflame-usb-only.yaml`.
This configuration has no WiFi, no API, no UART, and no actuator controls. It
produces no HA entities and cannot drive the fireplace.

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

Fireplace identified from its rating plate: **Montigo P52DF-F, natural gas**.
Dongle identified: **0584526_01**, connected to green-print IFC X0A.
Still needed: IFC part number/revision and handheld-remote model. Photograph
only safely accessible labels; do not open energized equipment. Add non-sensitive
photos here after review. No appliance-specific wiring has been verified.

**Runtime hardware confirmed:** C5rev1.0, 4 MB flash, native USB connection.
PCB header layout unverified; firmware assigns RX GPIO4 and TX GPIO5.

The upstream describes X0A as a four-pin JST XH connector with IFC signals
`3V3 / TX / RX / GND`, with orientation established by its printed ground dot.
That is **not a universal left-to-right hookup instruction**. Verify the actual
IFC and its manual before fabricating or plugging in a harness.

| Candidate connection, NOT installation authorization | Purpose |
| --- | --- |
| C5 USB (GPIO13 D− / GPIO14 D+) | Native USB; C5rev1.0 confirmed, exact PCB routing unverified |
| C5 UART RX GPIO4 / TX GPIO5 | Green IFC TX → GPIO4; brown IFC RX ← GPIO5 |
| C5 GND ↔ verified IFC GND | Only after confirming the interface grounding is suitable |
| IFC 3V3 | Leave disconnected and individually insulated |
| Board USB power | Separate regulated USB power; no 5 V lead to IFC |

UART GPIO4/5 are firmware assignments, not confirmation of the exact NiceMCU
header locations. Do not substitute a different board's physical header layout.
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

**Current state (network-bench):** WiFi provisioned over USB Improv; device
adopted by Home Assistant with encrypted native API. Three diagnostic entities
healthy (uptime, Wi-Fi signal, status). Friendly name: "Proflame USB Bench"
(historical; entity rename deferred). No fireplace automations were added or changed.
OEM dongle, its configuration and paired remote preserved.

**API key provisioning:** Before HA adoption, a unique API encryption key was
provisioned in device preferences. The zero-key bootstrap mode accepts both the
public zero-key and deprecated plaintext API; this is not authenticated against
an active LAN attacker. Verify the unique key is installed and the zero key is
rejected. An ordinary reboot preserves the unique key; this was verified after
a controlled USB reset. Erasing saved preferences requires reprovisioning.

Rollback to USB-only: compile and flash `proflame-usb-only.yaml` to remove all
network connectivity. For full OEM rollback: shut down using the appliance's
documented procedure, isolate power as instructed by its manufacturer, remove
the DIY harness, restore the OEM connection, then verify remote and original HA
operation. Do not hot-plug the IFC. Do not remove the current HA integration
until the replacement is accepted.

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
| 2026-11-06 | Added SVG wiring, RX GPIO4/TX GPIO5 and manual read-only IFC query. Compiled, parser-tested, independently reviewed and flashed app-only; saved credentials survived. No IFC query issued by agent. |
| 2026-11-06 | Earlier network-bench firmware deployed: WiFi via USB Improv, encrypted API, three HA diagnostics. C5rev1.0 4MB nativeUSB confirmed. |
| 2026-11-06 | Switched board from XIAO ESP32-S3 to NiceMCU ESP32-C5; removed S3 pin recommendations; C5 flash/USB unverified; USB detection attempted no device. |
| 2026-11-06 | Initial project, owner-confirmed inventory, USB-only bench firmware, upstream risk review and gated implementation plan. No live deployment. |
