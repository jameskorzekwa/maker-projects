# Low-power investigation — 2026-11-19

## Scope and authorization

James initially approved preparation/testing only, then explicitly authorized
deployment ("well deploy it"). **Merge remains unauthorized.** The reviewed
firmware from PR #50 head `ddd25d9` was deployed on 2026-11-19. Repository
publication and code review are distinct from the live evidence below.

## Evidence

- Live `/config/esphome/snow-depth-sensor.yaml` was inspected read-only.
  It keeps the ESP32 and sensor awake for 60 seconds, then sleeps for
  15 minutes. The API wait does not terminate the wake early.
- The sensor free-runs throughout the wake, with a five-frame median.
  The battery ADC takes readings every five seconds and publishes every five
  readings, delaying the initial filtered voltage roughly 20–25 seconds.
- Wi-Fi already uses `fast_connect` and `power_save_mode: HIGH`; logging
  is WARN with serial logging disabled. OTA mode was off when inspected.
- HA reported ESPHome 2026.11.0 and approximately 3.978 V on the latest wake.
  Seven-day history was too sparse to infer discharge rate or reliable
  wake/connection timing. A held historical voltage is not a fresh measurement.
- James confirmed two replacement 3,100 mAh cold-tolerant batteries.
  Do not apply the old cell's percentage curve to this pack, or assume its
  model, parallel wiring, usable capacity, or safe charging temperature.
- Historical documentation contains inconsistent power and battery estimates.
  There is no directly measured current baseline established in this task.

## Candidate behavior and expected benefit

Take a settled batch of valid distance readings and fresh battery voltage,
switch the sensor off when acquisition completes, and sleep after the API has
had a bounded opportunity to deliver telemetry and accept maintenance commands.
Bound unsuccessful wakes too; missing Wi-Fi or invalid ranges must not leave
the device awake indefinitely. Explicit OTA mode/upload activity takes
precedence over early sleep.

The candidate uses a host-tested `wake_policy.h` shared with the YAML:

- Three seconds of sensor settling, then five valid frames with a median.
- A five-sample battery median at 250 ms intervals.
- Early sleep requires both filtered readings, an API **state subscriber**,
  five seconds of continuous subscribed connection, and 1.5 seconds since
  the last measurement publication.
- A 30-second deadline from the boot automation bounds unsuccessful wakes.
  Setup time and shutdown teardown are additional. The deadline can truncate
  connection/transmission grace on a late connection.
- Explicit maintenance mode or an active upload overrides the deadline.
  Upload completion/error grants five seconds for reboot or retry.
- Maintenance preferences flush immediately rather than waiting a minute.
- Battery percentage retains its entity but becomes unknown (`NaN`); voltage
  remains available. Existing HA templates may retain an old percentage and
  require separate review before their display is trusted.

The target is roughly 5–10 seconds on a good connection, **not a measured or
guaranteed boot-to-sleep duration**. Compared with 60 seconds, that would
remove 83–92% of each wake's time. It is not an 83–92% reduction in total
battery consumption: boot energy, conversion losses, and sleep current remain.
Fifteen minutes is the sleep duration, so start-to-start cadence also includes
the variable wake duration.

An API connection and a transmission grace period are not application-level
acknowledgement that HA stored a reading. Confirm actual delivery on hardware.
The shorter wake also reduces the opportunity to observe a brief charge-DONE
pulse; an off charge-complete latch does not prove the pack is not full.

## Verification gates

Before any separately authorized deployment:

- Validate and compile with ESPHome 2026.11.0 using **dummy secrets only**.
  Do not flash that test artifact; a production build needs the existing
  private device credentials.
- Run regression tests for settling, valid/invalid/no frames, battery readiness,
  API delay/disconnect, normal/timeout sleep, and OTA maintenance/upload guards.
- Independently review the shutdown paths, persistent OTA behavior, and
  preservation of existing distance/voltage entity identities.

Native test instructions are in [`esphome/tests/README.md`](../esphome/tests/README.md).
The scoped GitHub workflow runs those sanitizer-backed tests and compiles the
firmware with dummy credentials. Neither produces a deployable production
artifact, and neither measures actual power consumption.

After deployment approval, live acceptance remains separate:

- Verify repeated wake/report/sleep cycles and HA receipt of both readings.
- Confirm sensor power turns off after acquisition and during sleep.
- Exercise disconnected-network and missing-target paths; verify bounded wakes.
- Verify OTA hold, upload, reboot, and return to sleeping operation.
- Compare battery-side sleep and active current, including boot and Wi-Fi
  association peaks. Measure the existing hardware before prescribing changes.
- Observe comparable multi-day voltage/charging behavior, accounting for sun,
  temperature, mounting, and battery configuration. Do not infer capacity from
  the retired cell's calibration.

## Preparation results

- Native C++ policy tests passed with AddressSanitizer and UndefinedBehaviorSanitizer.
- Full ESPHome **2026.11.0 / ESP-IDF 5.5.6**, `esp32dev`, compilation passed
  locally using public dummy credentials. Image size: 778,019 bytes;
  static DRAM: 45,428 bytes. This is compile evidence, not live-device evidence.
- The compile caught and resolved ESPHome's custom-header/global-declaration
  ordering: the policy instance is now a header-local accessor used from
  generated lambda bodies.
- Independent review found no blocking defects; final-head PR CI passed.
  These local/CI test results are separate from the live evidence below.

## Authorized deployment evidence

- Updated only `/config/esphome/snow-depth-sensor.yaml` and added its required
  `wake_policy.h`, preserving the existing device credentials. Production
  compilation in Device Builder 2026.11.0 succeeded.
- At **21:48 UTC**, maintenance mode was enabled during a wake, encrypted OTA
  to `192.168.0.22` succeeded, and the device rebooted. Native API DeviceInfo
  at 21:48:23.850 UTC reported compilation time
  **2026-11-19 14:34:24 -0700**, matching the new production build
  (configuration hash `0x94f85196`).
- Maintenance mode was turned **off**, confirmed by HA at 21:48:28 UTC.
- Fresh battery voltage **3.928 V** arrived in HA at 21:48:23.747 UTC.
  The real percentage entity reports unknown as intended; a separate
  template still holds its old percentage, which must not be trusted.
- The real distance entity `sensor.snow_depth_sensor_a02yyuw_distance` is
  **unknown**, as it already was before deployment. The legacy front-yard
  distance template still holds **37.0079 inches from 18:25 UTC**, and
  `sensor.snow_depth` holds zero. These are not fresh distance/snow readings.
- The next automatic wake was observed at **22:03:31.767 UTC**, again
  identifying the new build. HA received fresh **3.948 V** at 22:03:31.631 UTC.
  The native API cleanly disconnected at **22:03:59.908 UTC**, after
  **28.17 seconds connected**, consistent with the 30-second missing-range
  deadline plus boot/connection timing. The observer did not subscribe to
  states, so it did not satisfy the firmware's HA-subscriber readiness gate.
  Maintenance remained off; the real distance remained unknown.
- This establishes an automatic wake/report/clean-sleep cycle, **not** a
  successful 5–10-second ranging cycle. The configured fallback wake budget
  is half the old 60 seconds, but a 50% whole-battery-life improvement is
  not established. Distance acquisition needs a separate diagnosis.
- No direct current/capacity test or multi-day battery-life comparison has
  been performed. Missing-range operation may use the 30-second fallback.

## Hardware boundary

If sleep drain is actually 4–6 mA, it alone costs 96–144 mAh/day. That is a
conditional calculation, not a measurement. The load switch is downstream of
the converter and cannot eliminate converter standby drain. The divider
consumes about 21 µA at 4.2 V and is not the first optimization target.

No new components were selected or purchased. Any hardware redesign requires
its own investigation and component workflow. Never remove the MCU's supply
and expect its internal deep-sleep timer to wake an unpowered processor.
"Cold tolerant" batteries still require verification of **charging**, not
just discharging, limits.
