# Renogy Battery Monitor to Home Assistant (XIAO ESP32-S3)

A Seeed Studio XIAO ESP32-S3 mounted inside the display housing of the Renogy 500 A Battery Monitor with Shunt (RBM500) in the Casita travel trailer. It reads the display's serial output and publishes state of charge, voltage, current, power, remaining amp-hours and time remaining to Home Assistant. It also serves a small local web page so a phone on the trailer's Wi-Fi can check the battery without Home Assistant.

The RBM500 has no Bluetooth or data port, and Renogy sells no add-on for it. Inside, its display is a Baiway TF03H V35 board. Baiway's TF03 meters can be built with an optional TTL serial output; on this unit the isolator module for that output (`MK1`) is not fitted, but the board's serial transmit line is exposed on a row of holes beside the empty footprint. This project taps that line directly and powers the XIAO from the display's own battery feed through a small buck regulator, so the whole build fits inside the display housing with no extra cables.

![System overview](images/system-overview.svg)

> [!IMPORTANT]
> **Status: design documented, not yet built.** The ESPHome configuration validates and compiles (ESPHome 2026.9.1), but no hardware has been assembled. The design assumes this display's firmware transmits frames on the `Tx` hole even though the isolator module is missing. [Step 1](#step-1-confirm-the-display-transmits-bench-test) confirms that with a $5 USB-serial adapter before you solder anything permanent. If no frames appear, stop: this approach will not work on your unit.

## How It Works

1. The 500 A shunt sends analog sense signals (`Rs+`, `Rs-`) and battery voltage (`B+`, `B-`) to the display over the shielded cable. That cable carries no data.
2. The display's microcontroller computes the readings and, about once per second, sends a 16-byte frame out of its serial transmit pin: 9600 baud, 8 data bits, no parity, 1 stop bit.
3. A 12 V to 5 V buck regulator, fed from the backs of the display's `B+` and `B-` connector pins, powers the XIAO.
4. The display's `Tx` line connects to the XIAO's `D7` (GPIO44), through a 1 kohm resistor if the display uses 3.3 V logic or a voltage divider if it uses 5 V logic.
5. ESPHome decodes each frame with the open-source [`tf03k_shunt` component](https://github.com/edillmann/esphome-tf03k-smart-shunt) and publishes the values over Wi-Fi.

### Why no optocoupler is needed

The display's ground is the battery negative on the **battery side** of the shunt. The XIAO here is powered only from the display's `B+` and `B-`, so it shares that ground and has no other wired connection to anything; its only link to the outside is Wi-Fi.

That changes if the XIAO is ever also connected to the trailer's 12 V system or its negative, for example by powering it from a USB socket. The XIAO's ground would then be on the **load side** of the shunt, and the data wire would become a second path around the shunt: readings would be wrong, and if the main negative cable came loose the trailer's load current would try to flow through that thin wire. If you ever power it that way, isolate the data line with an optocoupler.

### Frame format

| Bytes | Content | Units |
| --- | --- | --- |
| 1 | Header `0xA5` | |
| 2 | State of charge | % |
| 3-4 | Battery voltage | 10 mV |
| 5-8 | Remaining capacity | mAh |
| 9-12 | Current, signed (negative = discharging) | mA |
| 13-15 | Time remaining | seconds |
| 16 | Checksum: 8-bit sum of bytes 1-15 | |

All multi-byte values are big-endian. Source: Baiway's TF03K communication specification, as reproduced in the component repository.

### Known limitations

- **The display only transmits while it is awake.** Renogy's manual says the monitor enters a low-power sleep and turns off its backlight when battery current is low, and Baiway's specification says frames are sent only while the meter is working (backlight on). While the display sleeps, the `Monitor Online` entity turns off after 60 seconds and Home Assistant keeps the last values. Pressing any button wakes the display for about 10 seconds. With the trailer's 12 V fridge cycling, the display will usually be awake, but expect gaps when the trailer is idle.
- **The shunt does not see the XIAO's draw.** The XIAO takes power from the battery side, ahead of the shunt, so its consumption (roughly 0.5 W, about 1 Ah per day at 12.8 V) is not counted. The displayed state of charge slowly reads high by that amount. Set **Full V** in the display's user settings so the monitor resets to 100% at every full charge.
- **It is always on.** `B+` comes straight from the battery, so the XIAO runs even when the trailer's battery disconnect switch is off: about 1 Ah per day in storage.
- **Measurement offset risk.** The XIAO's current, including Wi-Fi transmit spikes, flows through the thin conductors of the 20 ft shunt cable. The shunt signal is tiny (75 mV at 500 A, so 0.15 mV per amp). If the display measures `Rs+`/`Rs-` against its own ground rather than differentially, voltage drop on the cable's `B-` conductor could add a current offset or noise. The separate `Rs-` and `B-` pins suggest a differential measurement, but this is unverified. [Step 5](#step-5-check-the-current-reading-for-offset) tests for it.
- **Remote reading away from home needs a network path.** See [Reading the battery away from home](#reading-the-battery-away-from-home).

## Parts

| Component | Quantity | Notes | Purchase link |
| --- | ---: | --- | --- |
| Seeed Studio XIAO ESP32-S3 | 1 | Already owned. 21 x 17.5 mm. Uses an external U.FL antenna (included with the board) | [Seeed](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) |
| 12 V to 5 V buck regulator | 1 | Fixed 5 V output, at least 500 mA, input rating of at least 16 V to cover a charging LiFePO4 bank (14.6 V) with margin. Example: Pololu D24V5F5, 5 V 500 mA, about 10 x 13 mm | [Pololu](https://www.pololu.com/product/2843) |
| C1: electrolytic capacitor | 1 | 470 uF, 25 V or higher, low-ESR. Smooths the XIAO's Wi-Fi current spikes on the shunt cable | |
| Data resistor(s) | 1-2 | **Option A** (`V+` about 3.3 V): one 1 kohm. **Option B** (`V+` about 5 V): one 10 kohm and one 20 kohm. 1/4 W or 0805 | |
| Thin stranded hookup wire | About 0.5 m | 26-28 AWG silicone wire; red, black and one signal color | |
| Inline fuse holder and 1 A fuse | 1 | On the RBM500's `B+` wire, close to the battery, if it is not already fused | |
| Kapton tape, heat-shrink, hot glue | As needed | Insulation and mounting inside the housing | |
| USB-to-TTL serial adapter | 1 | Bench test only. 3.3 V logic (CP2102, CH340 or FTDI) | |

Measure the free depth behind the display board before buying. The XIAO and regulator together need roughly 25 x 35 x 8 mm, plus the capacitor.

## Tools and Supplies

- Fine-tip soldering iron, solder, flux and solder wick or a desoldering pump
- Multimeter with DC voltage, DC current (mA) and continuity
- Small screwdrivers
- Laptop running on its battery for the bench test (not plugged into a charger)

## Where to Connect on the Display Board

![RBM500 display board connection points](images/display-board-header.svg)

With the back cover removed and the white shunt plug on the left edge:

| Point | Use |
| --- | --- |
| `B+` pin of the shunt connector | Battery positive. Solder to the back of the pin on the board. Feeds the regulator `VIN` |
| `B-` pin of the shunt connector | Battery negative (battery side of the shunt). Solder to the back of the pin. Feeds the regulator `GND` |
| `Tx` hole (6-hole row beside `MK1`) | Microcontroller serial transmit. Goes to XIAO `D7` through the data resistor(s) |
| `V+` hole | Board logic supply. **Measure only**, to choose Option A or B |
| `G` hole | Board ground. Meter reference for measurements and the bench test |
| `Out`, `Rx`, `TEN` holes | Not used |

Do not touch:

- **`Rs-` and `Rs+`** on the shunt connector. These carry the millivolt shunt signal.
- **The slot labels `Out-`, `Out+`, `VCC`, `GND`, `TXD`, `RXD`.** These are the outer side of the missing isolator module.
- **`MK2`** (pads `G`, `CS`, `S`, `T`, `R`, `V`). An unpopulated footprint for an unknown add-on module.
- **`PROG`.** Factory programming pads for the microcontroller.

Do not power the XIAO from `V+`. It is the display's internal logic supply and cannot deliver the 300-500 mA bursts the XIAO draws when Wi-Fi transmits.

Photograph the board before modifying it and save the photo as `photos/rbm500-board-back.jpg`.

## Wiring

![Wiring inside the display housing](images/wiring-schematic.svg)

| From | To | Notes |
| --- | --- | --- |
| Display `B+` pin (back) | Regulator `VIN` and C1 `+` | |
| Display `B-` pin (back) | Regulator `GND` and C1 `-` | Observe C1's polarity stripe |
| Regulator `5V OUT` | XIAO `5V` | |
| Regulator `GND` | XIAO `GND` | |
| Display `Tx` | Option A: 1 kohm, then XIAO `D7` | `V+` measured about 3.3 V |
| Display `Tx` | Option B: 10 kohm, then XIAO `D7`; 20 kohm from `D7` to XIAO `GND` | `V+` measured about 5 V. Keeps `D7` at about 3.3 V |

Build only one data option. In Option B, the divider gives 5 V x 20 / (10 + 20) = 3.33 V at `D7`.

### XIAO ESP32-S3 pins

![XIAO ESP32-S3 pins used](images/xiao-esp32s3-pins.svg)

Only `5V`, `GND` and `D7` (GPIO44) are wired. Logging stays on the native USB port (`USB_SERIAL_JTAG`), so the UART pins are free.

> [!WARNING]
> Never plug USB into the XIAO while the shunt cable is connected to the display. The regulator's 5 V would meet the USB 5 V, and the XIAO's ground would be tied to whatever the USB host is connected to. Flash over USB once on the bench, then use wireless updates.

## Build

### Step 1: Confirm the display transmits (bench test)

Do this before buying parts or modifying the display permanently.

1. Unplug the shunt cable from the display to power it down. Remove the back cover.
2. Clear the solder from the `Tx`, `G` and `V+` holes with wick or a pump, or plan to tack wires onto the top of the pads.
3. Solder a short temporary wire to each of `Tx`, `G` and `V+`. Insulate the ends.
4. Reconnect the shunt cable so the display powers up. Measure DC voltage from `G` to `V+` and write it down. About 3.3 V means Option A; about 5 V means Option B.
5. Set the USB-to-TTL adapter to 3.3 V logic. If `V+` is 5 V, check that the adapter's RX input is 5 V-tolerant, or put the Option B divider in line for the test.
6. Unplug the laptop from its charger.
7. Connect only adapter `GND` to display `G` and adapter `RX` to display `Tx`. Leave the adapter's `TX` and power pins unconnected.
8. Open a serial terminal in hex mode at 9600 baud, 8N1. Press a display button to wake it.
9. You should see a 16-byte frame starting with `A5` about once per second. Check one frame: bytes 3-4 as a number, divided by 100, should equal the voltage on the display.
10. Disconnect the adapter and unplug the shunt cable. Remove the temporary `G` and `V+` wires; keep `Tx`.

If no frames appear, wake the display again and confirm the voltage on `Tx` toggles. If there is still nothing, the firmware does not transmit without the isolator module. Stop here and use a shunt with built-in Bluetooth instead.

### Step 2: Fuse and prepare

1. Check the RBM500's `B+` wire between the battery and the shunt board. If it has no fuse, add an inline 1 A fuse as close to the battery positive as practical.
2. With the shunt cable unplugged, plan the layout inside the housing: regulator and C1 near the shunt connector, XIAO with its antenna toward the side of the housing that faces the trailer interior.
3. Flash the XIAO over USB on the bench now (see [Flash the XIAO](#2-flash-the-xiao-over-usb-first-time-only)), before it is wired into the display.

### Step 3: Wire power

1. Solder a red wire to the back of the `B+` connector pin and a black wire to the back of the `B-` pin. Keep the joints small and do not bridge to the neighboring `Rs` pins.
2. Solder C1 across the regulator's `VIN` and `GND`, positive lead to `VIN`.
3. Connect the red wire to `VIN` and the black wire to `GND`.
4. Before connecting the XIAO: plug in the shunt cable and measure the regulator output. It should read 4.9-5.1 V. Unplug the shunt cable again.
5. Wire regulator `5V OUT` to XIAO `5V` and regulator `GND` to XIAO `GND`.

### Step 4: Wire data

1. Build Option A or Option B from the [Wiring](#wiring) table between the `Tx` wire and XIAO `D7`. For Option B, the 20 kohm resistor goes from `D7` to XIAO `GND`.
2. Cover every resistor lead and joint with heat-shrink.
3. Attach the U.FL antenna to the XIAO and stick the flexible antenna to the inside of the plastic housing, away from the display board's copper.
4. Secure the XIAO and regulator with Kapton tape or a dab of hot glue so nothing can short against the display board, and close the housing.

### Step 5: Check the current reading for offset

1. With the shunt cable unplugged from the display, temporarily disconnect the regulator's red `VIN` wire (or leave it unsoldered until this test).
2. Turn off every DC load you can so battery current is near zero. Plug in the shunt cable and note the current shown on the display after it settles.
3. Unplug the shunt cable, connect the regulator, plug the cable back in, and let the XIAO boot and join Wi-Fi.
4. Compare the displayed current. It should not change, because the XIAO's power is drawn ahead of the shunt.
5. If the reading shifts by more than about 0.1 A or becomes noticeably noisy, try in order: lower `wifi: output_power` further (for example `11dB`); add a second 470 uF capacitor at the regulator input; or run a separate pair of power wires from the battery side for the regulator instead of using the shunt cable.

## ESPHome Configuration

The configuration is [`esphome/casita-battery-monitor.yaml`](esphome/casita-battery-monitor.yaml). It pulls the `tf03k_shunt` component from [edillmann/esphome-tf03k-smart-shunt](https://github.com/edillmann/esphome-tf03k-smart-shunt) (Apache-2.0), pinned to commit `f248cd1f3e2ffa26eef3c8892f57fae07a0c58f9`, the 2026-09-08 "fix for esphome 2026.8" commit. Pinning prevents an upstream change from altering the firmware unexpectedly.

| Setting | Value |
| --- | --- |
| Board | `seeed_xiao_esp32s3`, ESP-IDF framework |
| UART | RX on GPIO44 (`D7`), 9600 baud, 8N1, internal pull-up enabled |
| Wi-Fi transmit power | 15 dB (default is 20 dB), to reduce current spikes on the shunt cable |
| Publish interval | 5 s (`refresh_interval` substitution) |
| Offline timeout | 60 s without a valid frame |
| Daily energy counters | `restore: false`, so they reset on reboot but do not wear the flash |
| Logger | USB, level `INFO` |
| Local web page | Port 80, password-protected (`web_username`, `web_password` secrets) |

Validation on bee2 with ESPHome 2026.9.1 on 2026-10-08: `esphome config` passed and `esphome compile` succeeded (RAM 30.0%, flash 47.4%). This proves only that the configuration builds. It has not run on hardware.

### Secrets

Copy [`esphome/secrets.example.yaml`](esphome/secrets.example.yaml) into ESPHome's `secrets.yaml` and replace every placeholder. Generate the API key with `openssl rand -base64 32`. ESPHome 2026.9 rejects the all-zeros placeholder key used in older examples.

## Home Assistant Setup

### 1. Add the configuration to ESPHome Device Builder

1. In Home Assistant, open **ESPHome Builder**.
2. Open **Secrets** and add the keys from `secrets.example.yaml` with real values.
3. Select **+ New Device**, choose **Continue**, name it `casita-battery-monitor`, and skip the board-specific setup.
4. Open the new device's **Edit** view, replace its contents with `casita-battery-monitor.yaml`, and **Save**.
5. Select **Validate**. It downloads the pinned component from GitHub, so Home Assistant needs internet access.

### 2. Flash the XIAO over USB (first time only)

Do this on the bench, before the XIAO is wired to the display.

1. Connect the XIAO to your laptop with a USB-C data cable.
2. In Device Builder choose **Install**, then **Plug into this computer**. Use Chrome or Edge.
3. If the port does not appear, hold the XIAO's **BOOT** button, tap **RESET**, release **BOOT**, and try again.
4. Wait for the install to finish, then open **Logs** to confirm it joins Wi-Fi.
5. Unplug USB. All later updates install wirelessly with **Install** then **Wirelessly**.

### 3. Adopt the device in Home Assistant

1. Once the XIAO is installed in the display and powered, go to **Settings > Devices & services**. Home Assistant should show a discovered **ESPHome** device named **Casita Battery Monitor**.
2. Select **Configure** and paste the `api_encryption_key` from your secrets when asked.
3. Assign it to an area, such as "Casita".

If it is not discovered, select **Add Integration > ESPHome** and enter `casita-battery-monitor.local` or the device's IP address.

### 4. Check the entities

| Entity name | Likely entity ID | Unit |
| --- | --- | --- |
| State of Charge | `sensor.casita_battery_monitor_state_of_charge` | % |
| Voltage | `sensor.casita_battery_monitor_voltage` | V |
| Current | `sensor.casita_battery_monitor_current` | A (negative = discharging) |
| Power | `sensor.casita_battery_monitor_power` | W |
| Remaining Capacity | `sensor.casita_battery_monitor_remaining_capacity` | Ah |
| Time Remaining | `sensor.casita_battery_monitor_time_remaining` | duration |
| Daily Charged Energy | `sensor.casita_battery_monitor_daily_charged_energy` | kWh |
| Daily Discharged Energy | `sensor.casita_battery_monitor_daily_discharged_energy` | kWh |
| Monitor Online | `binary_sensor.casita_battery_monitor_monitor_online` | on/off |
| Wi-Fi Signal, Uptime, Restart | diagnostic and config entities | |

Home Assistant generates entity IDs from the device and entity names. Confirm the actual IDs on the device page and adjust the examples below if they differ.

Wake the display and confirm that **Monitor Online** turns on and that state of charge, voltage and current match the display. If a value looks wrong, set `logger: level: DEBUG`, reinstall wirelessly, and watch the logs: the component prints every parsed frame and any checksum failures.

### 5. Dashboard card

```yaml
type: vertical-stack
cards:
  - type: gauge
    entity: sensor.casita_battery_monitor_state_of_charge
    name: Casita Battery
    min: 0
    max: 100
    severity:
      green: 50
      yellow: 25
      red: 0
  - type: entities
    entities:
      - entity: sensor.casita_battery_monitor_voltage
      - entity: sensor.casita_battery_monitor_current
      - entity: sensor.casita_battery_monitor_power
      - entity: sensor.casita_battery_monitor_remaining_capacity
      - entity: sensor.casita_battery_monitor_time_remaining
      - entity: binary_sensor.casita_battery_monitor_monitor_online
```

### 6. Low-battery alert (optional)

Replace `notify.mobile_app_your_phone` with your phone's notify service.

```yaml
alias: Casita battery low
triggers:
  - trigger: numeric_state
    entity_id: sensor.casita_battery_monitor_state_of_charge
    below: 25
    for: "00:05:00"
actions:
  - action: notify.mobile_app_your_phone
    data:
      title: Casita battery low
      message: >-
        Battery at {{ states('sensor.casita_battery_monitor_state_of_charge') }}%
        ({{ states('sensor.casita_battery_monitor_voltage') }} V).
mode: single
```

### 7. Local web page

On the trailer's Wi-Fi, open `http://casita-battery-monitor.local` (or the device's IP address) and log in with `web_username` and `web_password`. This works without Home Assistant.

## Reading the Battery Away From Home

ESPHome's Home Assistant connection is opened **by Home Assistant** to the device on port 6053. That works when the trailer is parked on the home Wi-Fi. When the trailer is elsewhere, the home Home Assistant cannot reach a device behind a campground or Starlink connection.

Options, not yet built or tested:

1. **Travel router with a VPN subnet route.** A router in the trailer, such as a GL.iNet running Tailscale, advertises the trailer's network as a subnet route, and the Home Assistant host accepts that route. Home Assistant then reaches the XIAO as if it were local. Give the XIAO a DHCP reservation so its address does not change.
2. **MQTT.** Add ESPHome's `mqtt:` component and publish to a broker reachable from both places. This works through any internet connection but needs a broker exposed securely.
3. **Local only.** Use the local web page at the campsite and let Home Assistant catch up when the trailer returns home.

## Testing Checklist

1. Bench test shows valid `A5` frames whose voltage matches the display.
2. Regulator output measures 4.9-5.1 V before the XIAO is connected.
3. With everything installed, the XIAO joins Wi-Fi and **Monitor Online** turns on within a few seconds of waking the display.
4. Voltage, current and state of charge in Home Assistant match the display within rounding.
5. The current-offset check in Step 5 shows no meaningful change.
6. Turn on a known load, such as a light, and confirm current and power change in the right direction (negative while discharging).
7. Let the display sleep. **Monitor Online** turns off after about 60 seconds and turns back on after a button press.
8. Logs show no repeated checksum warnings.
9. After an hour, the housing and regulator are no more than slightly warm.

## Troubleshooting

| Symptom | Likely cause | Fix |
| --- | --- | --- |
| No data, `Monitor Online` stays off | Display asleep, wrong hole, or divider built for the wrong voltage | Press a display button; recheck `Tx` wiring and the Option A/B choice against the measured `V+` |
| XIAO resets or drops Wi-Fi | Supply sags during Wi-Fi bursts | Check the regulator rating and C1; measure 5 V while transmitting |
| Weak Wi-Fi signal | Antenna placement or reduced transmit power | Move the antenna to the housing wall facing the trailer interior; raise `output_power` toward 17-20 dB if Step 5 allows |
| Displayed current shifted after installing | Voltage drop on the shunt cable's `B-` conductor | See Step 5 |
| Checksum warnings in the logs | Marginal signal edges | Use the 1 kohm option only at 3.3 V; shorten the data wire |
| Values frozen in Home Assistant | Display sleeping at low current | Expected. See Known limitations |
| State of charge drifts high over days | XIAO draw is not metered | Set **Full V** in the display's settings so it resyncs at full charge |
| Device not discovered | mDNS blocked between networks | Add the ESPHome integration manually by IP |

## Safety Notes

- Always unplug the shunt cable from the display before soldering inside it. The display is powered from the battery through `B+`, which should be fused close to the battery.
- Do not bridge the `B+`/`B-` solder joints to the neighboring `Rs` pins.
- Never connect the XIAO, regulator or display ground to the trailer's 12 V negative or to a USB device while installed. In this design, the shunt cable is the XIAO's only wired connection.
- Insulate every joint. A short from `B+` inside the housing is limited only by the `B+` fuse.
- Opening the display almost certainly voids Renogy's warranty on it.

## Photos

Add photos to `photos/` as the build progresses:

- `rbm500-board-back.jpg`: the display board before modification
- `inside-housing.jpg`: the XIAO, regulator and wiring inside the housing
- `installed.jpg`: the finished display installed in the trailer

## References

- [Renogy RBM500 product page](https://www.renogy.com/products/500a-battery-monitor-with-shunt) and [G3 manual](https://cdn.shopify.com/s/files/1/0631/0137/0483/files/RBM500-G3-Manual_26f37388-12d7-442a-99e6-77ee6e79f8bf.pdf)
- [edillmann/esphome-tf03k-smart-shunt](https://github.com/edillmann/esphome-tf03k-smart-shunt): ESPHome component and TF03K protocol notes
- [patrickwasp/tf03k](https://github.com/patrickwasp/tf03k): protocol diagram and notes that most TF03K units ship without the serial option
- [Seeed Studio XIAO ESP32-S3 wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

## Revisions

| Date | Change |
| --- | --- |
| 2026-10-08 | Initial design: XIAO inside the display housing, powered from the display's `B+`/`B-` through a buck regulator, with Home Assistant setup (not yet built) |
