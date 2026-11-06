# Verification record

> **Electrical correction:** owner measured **+6.26 V DC white-to-yellow with
> OEM dongle connected**. Earlier 3.3 V supply assumption is false for this unit.
> Direct hookup/diagram withdrawn; firmware tests below do not establish signal
> levels or safe electrical compatibility. No live reflash or IFC query was made
> for this correction. See [electrical findings](electrical-findings.md).

2026-11-06 preparation, executed on bee2 unless stated otherwise.

| Check | Result |
| --- | --- |
| Owner hardware choice | NiceMCU ESP32-C5 reported; exact model not established; do not assume NiceMCU 2.8IPS; board not physically inspected |
| Remote requirement | Preserve normal handheld-remote operation |
| Upstream source review | Blockers recorded in upstream-review.md; no IFC tests |
| ESPHome C5 YAML validation | PASS, ESPHome 2026.10.2 using uv-managed Python 3.12 on bee2 |
| XIAO bench firmware compile | Historical S3: PASS, ESP-IDF 5.5.6; RAM 49,355 / 341,760 bytes, flash 213,399 / 1,835,008 bytes |
| C5 bench firmware compile (network-bench) | PASS, ESPHome 2026.10.2, ESP-IDF 5.5.6; image 837,564 bytes; HP SRAM 129,612 / 320,928 bytes |
| C5 bench firmware compile (USB-only predecessor) | Historical PASS: image 207,256 bytes; HP SRAM 59,190 / 320,928 bytes |
| Runtime hardware confirmation | C5rev1.0, 4 MB flash, native USB connection confirmed |
| PCB pinout / TXRX assignment | Firmware RX GPIO4 / TX GPIO5; physical NiceMCU header layout unverified |
| Compiled image flash (laptop USB, esptool 5.1) | Hash verified, flashed to board |
| Current DeviceBuilder config hash | `fe2e25a2204fc0522198e1f969eaa13026ab6e84a627be0b430ab0969b60e1d2` |
| Deployed version | 2026.10.2 |
| WiFi + HA adoption | Wi-Fi provisioned via USB Improv; HA adopted through encrypted native API; three entities healthy |
| Entity status | Signal -23 dBm; uptime 31 sec after controlled reset |
| API encryption | Unique key works and zero key rejected with encryption-key error, before and after controlled USB reset |
| Independent project review | Network-bench source and deployment record approved; minor provisioning-client, serial-port and image-size wording corrected |
| IFC identification and electrical verification | Not performed |
| Fireplace control firmware | Not implemented; upstream is not approved for connection |
| Physical ignition/off and failure testing | Not performed |
| Long burn-in / IFC tests | Not performed |

## UART commissioning revision

- RX GPIO4, TX GPIO5, 9600–8-N-1; manual GET register16 only. No boot/reconnect
  commands, SET, ownership acquisition, actuator entities or retries.
- `ifc_query.h` parser tests passed under g++ C++17, warnings-as-errors,
  AddressSanitizer and UndefinedBehaviorSanitizer. Includes known ownership replies,
  bad lengths/checksums, wrong registers and SET echoes.
- ESPHome 2026.10.2 config/full compile passed, ESP-IDF 5.5.6: application image
  857,330 bytes, HP SRAM 130,254 / 320,928. Padded app binary: 857,680 bytes.
- App-only flash at **0x10000**, verified against the generated partition table;
  NVS at 0x390000 left untouched. esptool verified the written hash.
- After reboot: encrypted API works, zero key rejected, HA entry loaded; original
  diagnostics healthy. Query-result and button entities registered as `unknown`,
  expected before the first query. **No button press or IFC-connected test made.**
- Live YAML and header match source hashes. Header SHA-256:
  `a3e91e80f9e8251bc8d75904d71c8e286ba8e84a4de6448522794757f087afd1`.
- SVG XML parsed, rendered and visually checked; independent static review found
  no blocking issues. Receiver framing/late-reply and electrical caveats documented.

Compilation proves toolchain acceptance, not hardware compatibility or safe
appliance operation. Publication is not review, merge, installation or activation.

The S3 compile above is historical evidence only. C5 `variant: esp32c5` with
`flash_size: 4MB` validated and compiled successfully. The 4 MB build allocation
does not establish the physical board's flash capacity.

Reproduction from this project directory with `uv` installed:

```sh
uv run --python 3.12 --with esphome==2026.10.2 esphome config esphome/proflame-bench.yaml
uv run --python 3.12 --with esphome==2026.10.2 esphome compile esphome/proflame-bench.yaml
```

Both commands exited successfully for C5 on 2026-11-06. ESPHome generated ignored
factory/OTA binaries and an ELF under
`esphome/.esphome/build/proflame-bench/build/`. The application image is 837,564
bytes; the combined factory image written at 0x0 was 903,456 bytes. esptool 5.1
flashed the laptop-attached board and verified the written hash.
It was not published as a release. Full C5 tool transcripts are retained in the
private session evidence on bee2; internal transcript paths are not published.
ESPHome reported `WARNING Skipping RAM summary: no DRAM/DIRAM region in`
the generated `esp_idf_size.json`; its ESP-IDF size table reports HP SRAM instead.
Compilation completed successfully.
