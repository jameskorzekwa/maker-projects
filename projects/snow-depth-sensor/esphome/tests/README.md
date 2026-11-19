# Short-wake regression checks

From the repository root, compile and run the production policy's native tests:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
  -fsanitize=address,undefined -g \
  projects/snow-depth-sensor/esphome/tests/wake_policy_test.cpp \
  -o /tmp/snow-wake-policy-test
/tmp/snow-wake-policy-test
git diff --check
```

For ESPHome validation/builds, use ESPHome 2026.11.0 and copy the production
YAML, `wake_policy.h`, and `secrets.example.yaml` into an isolated build
directory, naming the example `secrets.yaml` there. Run:

```sh
esphome config snow-depth-sensor-mb7389-production.yaml
esphome compile snow-depth-sensor-mb7389-production.yaml
```

The example's API key is a publicly known, nonzero dummy (32 bytes of `0x01`).
ESPHome rejects the former all-zero key as reserved. The dummy enables
reproducible validation; **never deploy firmware built with example secrets**.
Use securely supplied real secrets only for an owner-authorized deployment.

Native tests execute the same wake policy included by the firmware. They cover
settling/range acceptance, readiness and grace timing, missing measurements,
offline/late connections, reconnects, maintenance and OTA overrides, recovery,
and timer rollover. ESPHome validation/build checks integration separately;
neither establishes hardware power consumption, telemetry delivery, sensor
accuracy, or flash persistence on the actual device.
