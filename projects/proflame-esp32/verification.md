# Verification record

2026-11-06 preparation, executed on bee2 unless stated otherwise.

| Check | Result |
| --- | --- |
| Owner hardware choice | XIAO ESP32-S3 confirmed; board not physically inspected |
| Remote requirement | Preserve normal handheld-remote operation |
| Upstream source review | Blockers recorded in upstream-review.md; no IFC tests |
| ESPHome YAML validation | PASS, ESPHome 2026.10.2 using uv-managed Python 3.12 on bee2 |
| XIAO bench firmware compile | PASS, ESP-IDF 5.5.6; RAM 49,355 / 341,760 bytes, flash 213,399 / 1,835,008 bytes |
| Independent project review | Approved for USB-only preparation; source queue defect independently confirmed. Low-severity stale verification rows and unnecessary purchase metadata corrected before publication |
| USB flash / 15-minute bench test | Not performed; board is with owner |
| IFC identification and electrical verification | Not performed |
| Fireplace control firmware | Not implemented; upstream is not approved for connection |
| HA deployment / integration adoption | Not performed |
| Physical ignition/off and failure testing | Not performed |

Compilation proves toolchain acceptance, not hardware compatibility or safe
appliance operation. Publication is not review, merge, installation or activation.

Reproduction from this project directory with `uv` installed:

```sh
uv run --python 3.12 --with esphome==2026.10.2 esphome config esphome/proflame-bench.yaml
uv run --python 3.12 --with esphome==2026.10.2 esphome compile esphome/proflame-bench.yaml
```

Both commands exited successfully on 2026-11-06. ESPHome generated ignored
factory/OTA binaries and an ELF under
`esphome/.esphome/build/proflame-bench/build/`. They were not flashed or published
as a release. The full local tool transcript is retained on bee2 at
`/srv/opencode-data/jkorzekwa/opencode/xdg/opencode/tool-output/tool_1b271f53f001n8wMV2NQ9rA2ul`.
