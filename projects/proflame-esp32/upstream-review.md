# Candidate component review

Read-only source review, 2026-11-06; not a hardware test or safety certification.
Pinned candidate: `kaechele/esphome-proflame2` at
`bb67453c8782385e2b895274d482e60eb4808934`.

Sources:

- [README and wiring limitations](https://github.com/kaechele/esphome-proflame2/blob/bb67453c8782385e2b895274d482e60eb4808934/README.md)
- [Protocol engine](https://github.com/kaechele/esphome-proflame2/blob/bb67453c8782385e2b895274d482e60eb4808934/components/proflame2/proflame2.cpp)
- [State and setters](https://github.com/kaechele/esphome-proflame2/blob/bb67453c8782385e2b895274d482e60eb4808934/components/proflame2/proflame2.h)
- [Light feedback](https://github.com/kaechele/esphome-proflame2/blob/bb67453c8782385e2b895274d482e60eb4808934/components/proflame2/light/proflame2_lights.cpp)
- [Original packet captures](https://bonaparte.readthedocs.io/en/latest/system_information.html)
- [ESPHome ESP32 platform](https://esphome.io/components/esp32/)
- [XIAO ESP32-S3 hardware reference](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

## Findings to resolve before adapting for a real fireplace

1. `setup()` requests control by sending a SET to register 0x16. Polling also
   attempts to acquire control when cached control is not UART. There is no
   passive-mode or explicit-arm option. Removing actuator entities is insufficient.
2. `process_command_queue_()` uses `auto cmd = command_queue_.front()` and changes
   the copied timestamp, not the queued entry. The queued timestamp stays zero;
   an unanswered front command can be resent instead of reaching its intended
   timeout path. Later commands can queue behind it. Reproduce against a mock,
   then fix and test bounded queue/expiry/no stale replay, not just the copy.
3. Link silence does not invalidate cached control and actuator state. The control
   binary sensor represents ownership, not fresh connectivity or physical flame.
4. Light state feedback uses normal `make_call().perform()` calls, potentially
   reaching the write path. Its incoming normalized float and stored integer
   level comparisons are inconsistent. Test all seven levels, OFF with retained
   nonzero settings, boot and reacquisition; separate publish from command paths.
5. Actuator setters are not gated on confirmed ownership and a fresh complete
   baseline. Many call `ensure_on()` even for zero/false requests. Whole-register
   writes from stale cache and unintended mode transitions need regression tests.
6. Response matching is not transaction-specific. SET echo is not confirmation
   of accepted state or burner ignition. Test delayed, malformed and mismatched
   responses, and ensure stale echoes cannot re-enable command acceptance.
7. The upstream reports active-remote arbitration limitations and stale state
   during RF control. James requires normal remote use. Do not resolve this by
   unpairing the remote; determine actual IFC behavior first.

The supplied upstream simulator is useful but does not enforce all real IFC
ownership behavior. Simulator success would not prove remote coexistence.
No full OEM thermostat or burner-feedback parity has been established.

## Future implementation contract

- Separate disarmed startup, ownership, fresh baseline and accepted commands.
- No actuator restoration or automatic control acquisition on boot/reconnect.
- No queued actuation across link loss or ownership changes; bound memory usage.
- Mark stale state unavailable/unknown without inventing an OFF observation.
- Explicit user actions only after fresh state and ownership checks; no HA
  startup/reconnect automations. These software measures are not safety interlocks.
- Preserve a verified independent manual shutdown path and original remote.
- Pin reviewed source and ESPHome version, retain license/attribution if vendored,
  add protocol tests and independent review before appliance connection.

If UART cannot satisfy the remote requirement, evaluate a separate RF prototype
such as [HACS-Proflame2](https://github.com/jeffgregx2/HACS-Proflame2), with its own
hardware and acknowledgement limits. RF is not an automatic reliability upgrade.
