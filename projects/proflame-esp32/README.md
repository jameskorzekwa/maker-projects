# Proflame ESP32 dongle replacement

> **CURRENT STATUS (2026-11-07) — communication with the fireplace works.**
> James re-measured white-to-yellow at **3.3 V**; the earlier 6.26 V reading was
> a measurement error and is retracted. Wiring as documented upstream: green
> (IFC TX) → GPIO4, brown (IFC RX) → GPIO5, yellow → GND, white unconnected,
> C5 USB-powered. The deployed firmware polls registers 0x16/0x17/0x18 every
> 5 s with **GET requests only** (no SET, no control acquisition, no actuators)
> and the IFC returns valid checksummed replies. Encrypted Wi-Fi logs and
> password-authenticated OTA are enabled and verified with a real OTA upload.
> Sections below that describe the direct hookup as withdrawn, the 6.26 V
> reading, or RX-only firmware are **superseded history**.

Live readback, first polling session:

| Register | Raw reply | Decoded |
| --- | --- | --- |
| 0x16 control | `AE C1 A0 00 16 D9` | UART control reported |
| 0x17 state 1 | `AE 00 04 00 17 BD` | mode nibble **4** (outside documented 0–3), night light 0, pilot off |
| 0x18 state 2 | `AE 00 5C 00 18 EA` | flame 4, comfort fan 5, AUX on, split flow off |

Decoding follows the published register layout; mode value 4 and the actual
fireplace state at the time still need physical confirmation.
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

## Wiring: previous direct connection withdrawn

**[Open the SVG wiring diagram](wiring.svg)** · **[Detailed wiring and sources](wiring.md)**

| Existing X0A wire | Current finding / C5 connection |
| --- | --- |
| Green | Suspected signal; direction/voltage unverified — **do not connect** |
| Brown | Suspected signal; direction/voltage unverified — **do not connect** |
| Yellow | Reference for white-to-yellow measurement; isolation unverified — **do not connect** |
| White | **+6.26 V measured relative to yellow under OEM load; do not connect** |

GPIO4/5 remain logical firmware assignments, not approved electrical connections.
The older source-derived pinout is contradicted at least for supply voltage.
Colors refer to the photographed harness, not a standard. Keep the C5 USB-powered
and disconnected from X0A. Isolate mains and battery backup before disconnecting
any existing experimental wiring. Retain the OEM dongle.

The former **Check IFC Link** button and query firmware were removed from the
current device. Both GPIO4 and GPIO5 are now receive-only at 9600–8-N-1, with
per-channel byte count, age since latest batch and latest candidate hex batch
(up to 32 bytes), publishing every two seconds. **RX UARTs Ready** indicates no
reported UART initialization failure, not verified signal reception. There is no
configured TX pin or UART write path. Input pulls are disabled in configuration.

Initial post-flash observation: GPIO4 count zero; GPIO5 received repeated `00`
bytes. These are **not validated IFC frames**. Noise, a low/floating input,
framing errors or electrical mismatch may produce candidate bytes. Silence with
the OEM removed and no query transmitter may be normal. This does not approve
direct wiring, prove safe voltages or establish current physical hookup.

## Firmware files and rollback

Two ESPHome configs exist in this project:

| File | Purpose | Network | API | OTA |
| --- | --- | --- | --- | --- |
| [proflame-bench.yaml](esphome/proflame-bench.yaml) | **Current RX-only image** — network diagnostics and two input-only UART receivers. Deployed; old query header no longer included. | WiFi (Improv over USB) | Encrypted (`api.encryption: {}` — zero-key bootstrap, unique key must be provisioned) | No |
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
connection; exact NiceMCU PCB header layout remains unverified. GPIO4/5 are now
both configured as RX. The old `ifc_query.h` is retained only as historical code.

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
- **No OTA, web server, AP, transmitter or actuator controls.** Candidate bytes
  are not voltage measurements or validated messages. An oscilloscope/appropriate
  electrical interface is still required to establish electrical compatibility.
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
| C5 UART RX GPIO4 / RX GPIO5 | Both receive-only; direct X0A hookup withdrawn |
| C5 GND / IFC return | No connection until grounding/isolation established |
| IFC white supply | +6.26 V measured to yellow under OEM load; leave disconnected |
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
| 2026-11-06 | Owner's loaded +6.26 V measurement contradicts earlier supply label. Withdrew direct wiring/diagram; recorded manufacturer-document and component-search limits. No live reflash or IFC query. |
| 2026-11-06 | Added SVG wiring, RX GPIO4/TX GPIO5 and manual read-only IFC query. Compiled, parser-tested, independently reviewed and flashed app-only; saved credentials survived. No IFC query issued by agent. |
| 2026-11-06 | Earlier network-bench firmware deployed: WiFi via USB Improv, encrypted API, three HA diagnostics. C5rev1.0 4MB nativeUSB confirmed. |
| 2026-11-06 | Switched board from XIAO ESP32-S3 to NiceMCU ESP32-C5; removed S3 pin recommendations; C5 flash/USB unverified; USB detection attempted no device. |
| 2026-11-06 | Initial project, owner-confirmed inventory, USB-only bench firmware, upstream risk review and gated implementation plan. No live deployment. |
