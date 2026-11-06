# Commissioning and rollback checklist

**Not executable yet:** hardware identification and control-firmware fixes in
[README.md](README.md) must be completed first. Current firmware is USB-only.
Do not interpret unchecked tasks as tests already performed.

## Before IFC connection

- [ ] Identify fireplace, IFC revision, remote and dongle; retain photos/manuals.
- [ ] Confirm X0A pin orientation, signal voltage and grounding for this appliance.
- [ ] Verify completed unpowered harness continuity and insulation; independently
      check connector orientation. No 5 V or ESP power lead goes to the IFC.
- [ ] Confirm approved mounting location, cable temperature ratings and strain relief.
- [ ] Keep existing dongle/configuration and paired remote for rollback.
- [ ] Fix and test all upstream blockers; pin and independently review control firmware.
- [ ] Mock tests: boot, loss/recovery, silence, queue bounds/expiry, malformed/late
      replies, ownership changes, retained actuator levels and zero-valued commands.
- [ ] Captures show no actuation or control acquisition without explicit arming;
      no actuation is replayed after reconnect or reboot.
- [ ] Confirm independent appliance shutdown procedure and working CO/smoke alarms.

## Supervised appliance trial, after the above

Follow manufacturer shutdown/isolation instructions before touching the harness.
Have a qualified technician handle uncertain appliance-side work. One UART master
only. Keep clear of hot components and do not modify gas/ignition circuits.

- [ ] First connection is supervised, with fireplace in its documented safe state.
- [ ] Verify fresh state and ownership without assuming the burner is off from HA.
- [ ] Verify physical response to each explicitly requested action, including OFF,
      separately from HA's displayed state. Record unsupported features.
- [ ] Test handheld operation both before and after ESP control; record ownership
      changes and state freshness. Reject a solution that requires unpairing it.
- [ ] Test ESP restart and network/HA loss first with the appliance off. Any later
      powered-operation failure tests require supervision and the shutdown method
      immediately available. Do not assume a network failure turns off the burner.
- [ ] Verify no unsolicited ignition, control takeover or stale command replay.
- [ ] If behavior differs from expectations, stop; document and restore OEM setup.
- [ ] Record monitored burn-in duration, conditions, resets/errors and limitations;
      do not declare unattended readiness solely from a short successful test.

## Rollback

1. Use the appliance's documented independent shutdown method; verify its actual
   state. Do not depend on the experimental ESPHome connection for shutdown.
2. Follow manufacturer power-isolation instructions, including backup supplies
   if applicable. Never unplug/replug the IFC harness live.
3. Remove DIY harness and power, restore original OEM dongle connection.
4. Restore power per manufacturer instructions and verify remote operation.
5. Verify original HA integration reconnects; keep automations disabled until
   state and controls are confirmed. Record rollback and any unresolved fault.
