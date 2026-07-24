# MB7389 Snow Depth Sensor Guide

Wiring, ESPHome configuration, mounting, and calibration for the MaxBotix
MB7389-100 ultrasonic sensor on the ATOM Echo. Power for the system comes from
the MPPT solar manager — see the [Solar Power Board Guide](solar-power-board-guide.md).

Key facts:

- Range 300–5000 mm, 1 mm resolution, internal temperature compensation, 3/4-in NPS threaded housing.
- Pin 5 serial output is true TTL, idle high (verified by meter 2026-07-22) — standard non-inverted UART at 9600 8-N-1 into GPIO32 through a 10k/20k divider. **The divider's 20k must be a series pair if built from two 10k resistors — wired in parallel they crush logic highs to 1.67 V and the ESP32 hears nothing.**
- GPIO26 switches sensor power through a Pololu #2810 load switch; the sensor is fully off during deep sleep.
- The distance entity keeps the legacy name/ID `a02yyuw_distance` so Home Assistant history and helpers survived the sensor swap.
- Bare-ground baseline recalibration is pending the final outdoor mount.

## Sensor-subsystem hardware

- **Load switch** (selected 2026-07-21): Pololu Mini MOSFET Slide Switch with Reverse Voltage Protection, LV — [Pololu item #2810](https://www.pololu.com/product/2810), also sold by RobotShop. Verified against the requirements: high-side P-MOSFET switch, 1.8–16 V operating range, 3 A continuous (MB7389 peaks at ~98 mA), ON pin turns on above ~1 V so 3.3 V GPIO logic drives it directly, and it stays off when the ON pin is low or disconnected. **Keep the board's physical slide switch OFF permanently**: the ON pin only controls the MOSFET while the slide is off. A 100 kΩ ON-to-GND pulldown is still recommended so the GPIO26 line cannot float into the ~1 V turn-on threshold during deep sleep.
- One 10 kΩ resistor and one 20 kΩ resistor for the serial voltage divider.
- One 100 µF electrolytic capacitor at the sensor's power pins; a 0.1 µF ceramic beside it is also recommended.
- A weatherproof rear enclosure and cable gland (the MB7389-100 transducer housing is weather resistant, but its rear pins are exposed).
- A seven-position 0.1-inch connector/header or a soldered cable with strain relief.
- An external temperature probe is not required; leave MB7389 pin 1 unconnected.
- **Do not power the MB7389 directly from an ESP32 GPIO** — its 5 V peak current is approximately 98 mA.

## Complete wiring diagram

Power arrives from the MPPT board (always-on 3.3 V to the ATOM, 5 V to the
Pololu) — full power wiring in the [Solar Power Board Guide](solar-power-board-guide.md).

```text
SWITCHED SENSOR POWER
Pololu #2810 VOUT ──> MB7389 pin 6 (V+)        (only live while GPIO26 is high)
Pololu #2810 slide switch: set OFF and leave OFF

ENABLE LINE
ATOM GPIO26 ──┬──> Pololu #2810 ON pin         (direct wire, nothing in series)
            100 kΩ
              └──> GND                         (pulldown in parallel, soldered
                                                across the Pololu ON and GND holes)

SERIAL LINE (5 V to 3.3 V divider)
MB7389 pin 5 ──> 10 kΩ ──┬──> ATOM GPIO32
                       20 kΩ
                         └──> GND

CAPACITORS (at the sensor, directly across its power pins)
MB7389 pin 6 (V+) ───┬──────────┬───
                  100 µF     0.1 µF
                  (+ lead)   (either way)
                  (− lead)
MB7389 pin 7 (GND) ──┴──────────┴───

MB7389 pins 1, 2, 3, 4: leave unconnected
```

### MB7389 pin map for this build

| Pin | Function | Connection |
| --- | --- | --- |
| 1 | External temperature input | Leave unconnected |
| 2 | Pulse-width output | Leave unconnected |
| 3 | Analog output | Leave unconnected |
| 4 | Ranging start/stop | Leave unconnected (free-runs while powered, filtered output) |
| 5 | TTL serial output | GPIO32 through the 10 kΩ / 20 kΩ divider |
| 6 | V+ | Switched 5 V (Pololu VOUT) |
| 7 | GND | Shared system ground |

Confirm the printed pin labels or datasheet orientation before soldering; do not identify pins only by left/right position.

### What the capacitors do and where they physically go

Both capacitors connect **in parallel across the sensor's power input**: one lead to pin 6 (V+), the other to pin 7 (GND). They are not in the current path — they simply bridge the sensor's two power pins. Mount them as close to the sensor as practical (at its connector or pigtail), on the switched side of the Pololu. The 100 µF electrolytic is a local energy reservoir: the MB7389 draws ~98 mA pulses when the transducer fires, and the capacitor supplies those bursts locally so the supply doesn't dip through the wiring. It is polarized — stripe-marked negative lead to pin 7, positive to pin 6. The 0.1 µF ceramic absorbs fast high-frequency noise; no polarity. Because they sit on the switched output they charge/discharge each wake cycle — expected and harmless.

### Why the serial divider is required

The sensor is best powered from 5 V (especially for cold-weather performance), so its TTL serial output rises to ~5 V. The ATOM Echo GPIO is a 3.3 V input, not 5 V tolerant. The 10 kΩ series + 20 kΩ shunt reduce a 5 V high to ~3.33 V. Never connect pin 5 directly to GPIO32 with the sensor on 5 V.

### Why the load switch is required

The MB7389 draws ~3.1 mA even when idle, which would spoil the near-zero deep-sleep result achieved after removing the Echo speaker. GPIO26 enables the Pololu #2810 only while awake; the ON pin threshold is ~1 V and the switch turns off when the pin is low or disconnected, so deep sleep (GPIO high-impedance + the 100 kΩ pulldown) powers the sensor fully down. Cautions: the physical slide switch must stay OFF, and the on-board LED draws ~1 mA at 5 V while on (wake windows only).

## Mounting constraints

### Minimum range and maximum measurable snow

The MB7389 reports 300 mm to 5000 mm; anything closer than ~11.8 in reads as 300 mm. Therefore:

```text
maximum measurable snow depth ≈ bare-deck sensor height − 11.8 inches
```

To measure up to 24 in of snow the sensor must sit at least ~35.8 in above bare deck; **40–48 in of mounting height is the comfortable sweet spot**. A 12-inch test stand has essentially no usable range.

### Beam and wall clearance

Nominal 11° beam: at 40 in the beam radius is ~3.8 in — keep the sensor center at least 5 in from any vertical face, post, enclosure wall, or roof edge (more is better). Horn faces straight down and protrudes through the enclosure; never recessed in a tube or behind a lip. The MB7389 locks onto the strongest acoustic return, and 90° corners reflect strongly — keep rail-and-deck corners out of the beam.

## ESPHome configuration

The deployed configuration is
[`../esphome/snow-depth-sensor-mb7389-production.yaml`](../esphome/snow-depth-sensor-mb7389-production.yaml)
(copy `secrets.example.yaml` values into the private ESPHome `secrets.yaml`).
Implementation note: the frame parser lives in the UART debug hook, which
fires regardless of logger level, so it works with production WARN logging.
The key blocks:

### UART frame parser

```yaml
uart:
  id: uart_a
  rx_pin: GPIO32
  baud_rate: 9600
  data_bits: 8
  parity: NONE
  stop_bits: 1
  debug:
    direction: RX
    dummy_receiver: true
    debug_prefix: "[MB7389 UART] "
    after:
      delimiter: "\r"
    sequence:
      - lambda: |-
          if (bytes.size() != 6 || bytes[0] != 'R' || bytes[5] != '\r')
            return;

          for (int i = 1; i <= 4; i++) {
            if (bytes[i] < '0' || bytes[i] > '9')
              return;
          }

          const int mm = (bytes[1] - '0') * 1000
                       + (bytes[2] - '0') * 100
                       + (bytes[3] - '0') * 10
                       + (bytes[4] - '0');

          if (mm < 300 || mm >= 5000) {
            ESP_LOGW("maxbotix", "Rejected range: %d mm", mm);
            return;
          }

          id(a02yyuw_distance).publish_state((float) mm);
          ESP_LOGD("maxbotix", "Distance: %.2f in (%d mm)",
                   mm / 25.4f, mm);
```

The MB7389 sends `Rdddd` followed by carriage return (`dddd` = millimeters; `5000` = no target detected).

### Distance template sensor

```yaml
sensor:
  - platform: template
    name: "A02YYUW Distance"
    id: a02yyuw_distance
    unit_of_measurement: "mm"
    device_class: distance
    state_class: measurement
    accuracy_decimals: 0
    update_interval: never
    force_update: true
    filters:
      - median:
          window_size: 5
          send_every: 5
          send_first_at: 5

  # Battery/charge monitoring (VBAT ADC, CHRG/DONE) follows in the full config.
```

The legacy name and ID are intentionally preserved so the existing Home Assistant entity, history, and helpers survive.

### GPIO26 sensor-power enable

```yaml
output:
  - platform: gpio
    pin: GPIO26
    id: maxbotix_power_enable
```

At the start of `on_boot`, power the sensor and let it settle (the deployed production config guards `deep_sleep.prevent` behind the OTA-mode switch; see the config file for the exact sequence):

```yaml
esphome:
  name: snow-depth-sensor
  friendly_name: snow-depth-sensor
  on_boot:
    priority: -100
    then:
      - deep_sleep.prevent: deep_sleep_control
      - output.turn_on: maxbotix_power_enable
      - delay: 3s
      - wait_until:
          condition:
            api.connected:
          timeout: 15s
```

Keep the OTA `on_begin` safeguard and the persistent "Snow Sensor OTA Mode" switch.

## Deep sleep and update behavior

- Each wake: GPIO26 enables 5 V, the MB7389 free-runs, the parser collects frames. Wait ≥3 s before depending on a measurement; the five-sample median publishes well inside the 60 s window.
- On sleep, GPIO26 goes inactive and the Pololu ON pin falls through the pulldown — sensor fully off.
- For OTA of this deep-sleep device: enable the OTA Mode switch during a wake window first.

## Calibration and commissioning

1. Mount the sensor in its final position, facing straight down, deck bare.
2. Verify frames (e.g. `R1016`) and that reported inches match a tape measure.
3. Record the median bare-deck distance in millimeters (the stable sensor reading, not the physical tape measurement).
4. Update the Home Assistant snow-depth calculation baseline: `snow depth (in) = max(0, (bare_deck_mm − current_mm) / 25.4)`.
5. Test with flat targets at known heights; confirm the baseline returns when removed.
6. Restore production deep sleep and verify near-zero sleep current.

## Acceptance checklist

- [ ] GPIO32 never sees more than ~3.3 V.
- [ ] Sensor power is off during deep sleep (Pololu slide switch confirmed OFF).
- [ ] Five or more valid `Rdddd` frames every wake cycle.
- [ ] Bare-deck distance repeatable within tolerance.
- [ ] Nearest wall/post/edge outside the beam with margin.
- [ ] Max expected snow level stays ≥11.8 in below the sensor.
- [ ] Rear pins, joints, and cable entry protected from precipitation and condensation.
- [ ] OTA mode keeps the device awake through a complete update.

## Sources

- [MaxBotix MB7389 product page](https://maxbotix.com/products/mb7389)
- [HRXL-MaxSonar-WR datasheet](https://maxbotix.com/pages/hrxl-maxsonar-wr-datasheet)
- [Pololu Mini MOSFET Slide Switch LV, item #2810](https://www.pololu.com/product/2810)
- ESPHome UART and deep-sleep documentation
