# Verification record

2026-11-06 preparation, executed on bee2 unless stated otherwise.

| Check | Result |
| --- | --- |
| Owner hardware choice | NiceMCU ESP32-C5 reported; exact model not established; do not assume NiceMCU 2.8IPS; board not physically inspected |
| Remote requirement | Preserve normal handheld-remote operation |
| Upstream source review | Blockers recorded in upstream-review.md; no IFC tests |
| ESPHome C5 YAML validation | PASS, ESPHome 2026.10.2 using uv-managed Python 3.12 on bee2 |
| XIAO bench firmware compile | Historical S3: PASS, ESP-IDF 5.5.6; RAM 49,355 / 341,760 bytes, flash 213,399 / 1,835,008 bytes |
| C5 bench firmware compile | PASS, ESP-IDF 5.5.6; image 207,256 bytes; HP SRAM 59,190 / 320,928 bytes from ESP-IDF size table |
| Independent project review | C5 change approved for USB-only bench preparation; minor USB-evidence wording, version-matched reference and public metadata findings corrected |
| USB detection (macOS ioreg 2026-11-06) | Owner reports board attached; inspection returned USB controllers only and no USB serial port. Device did not enumerate; cause not established |
| USB flash / 15-minute bench test | Not performed; board is with owner; data link unverified |
| IFC identification and electrical verification | Not performed |
| Fireplace control firmware | Not implemented; upstream is not approved for connection |
| HA deployment / integration adoption | Not performed |
| Physical ignition/off and failure testing | Not performed |

Compilation proves toolchain acceptance, not hardware compatibility or safe
appliance operation. Publication is not review, merge, installation or activation.

The S3 compile above is historical evidence only. C5 `variant: esp32c5` with
`flash_size: 4MB` validated and compiled successfully, but has not been flashed.
The 4 MB build allocation does not establish the physical board's flash capacity.

Reproduction from this project directory with `uv` installed:

```sh
uv run --python 3.12 --with esphome==2026.10.2 esphome config esphome/proflame-bench.yaml
uv run --python 3.12 --with esphome==2026.10.2 esphome compile esphome/proflame-bench.yaml
```

Both commands exited successfully for C5 on 2026-11-06. ESPHome generated ignored
factory/OTA binaries and an ELF under
`esphome/.esphome/build/proflame-bench/build/`. They were not flashed or published
as a release. Full C5 and historical S3 tool transcripts are retained in the
private session evidence on bee2; internal transcript paths are not published.
ESPHome reported `WARNING Skipping RAM summary: no DRAM/DIRAM region in`
the generated `esp_idf_size.json`; its ESP-IDF size table reports HP SRAM instead.
Compilation completed successfully.
