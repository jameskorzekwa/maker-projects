# Fireplace knob: behaviour and state machines

Reference for the firmware in `esphome/fireplace-knob/`. Read this before
changing anything in `common.yaml` or `hw-m5dial.yaml`; the last section lists
the rules that were learned the hard way.

The firmware is four cooperating state machines, each owned by a few globals:

| Machine | Globals | Where |
| --- | --- | --- |
| Screen | `page`, `last_activity_ms`, `sleep_since_ms` | `common.yaml` (`go`, `tap`, `knob_turn`, 250 ms interval) |
| Button | `hold_hinted`, `hold_count`, `press_from_sleep`, `boot_guard`, `guard_ms` | `common.yaml` (`button_down`, `button_up`, `long_press_timer`) |
| Data / HA link | `got_*`, `loading_shown`, `stale_ok`, `conn_timed_out`, `disconnected_since_ms`, `pending_toggle`, `*_pending` | `common.yaml` (`render`, HA mirrors, `send_*`) |
| Power (Dial) | `sleep_state`, `idle_sleeping`, `power_off_tried`, `on_usb`, `battery_charging`, `battery_absent` | `hw-m5dial.yaml` (`board_idle_sleep`, `board_resume`, `board_power_off`, 200 ms interval, INA219 `on_value`) |

Timings are substitutions in `common.yaml` unless noted.

## 1. Screen machine

```
page: 0 Sleep, 1 Main, 2 Set Point, 3 Settings (arc), 4 Power, 5 Mode,
      6 Fan, 7 Light, 8 Flame, 9 Brightness
```

```
            tap              tap               tap on icon
  SLEEP ---------> MAIN -----------> SETTINGS ---------------> EDIT (4..9)
   ^  ^            ^ ^                 ^   |                     |
   |  | turn       | | tap / 10 s      |   +---- tap on Back ----+--> MAIN
   |  v            | v                 +------------ tap -----------+
   | SET POINT <--turn-- MAIN
   |
   +---- 10 s without input from ANY page != 0 (sleep_after_s)
```

| From | Turn | Tap | Timer |
| --- | --- | --- | --- |
| Sleep (0) | Set Point (2); the first click already changes the value | Main (1) (`press_from_sleep`: the tap only wakes) | - |
| Main (1) | Set Point (2) | Settings (3), highlight on Back | 10 s -> Sleep |
| Set Point (2) | +/- set temperature, or flame height in Manual | Main | 10 s (`setpoint_timeout_s`) -> Main; 10 s -> Sleep |
| Settings (3) | Move the highlight (no wrap) | Open the highlighted edit page; Back -> Main | 10 s -> Sleep |
| Power (4) / Mode (5) | Choose | Apply if changed, back to Settings | 10 s -> Sleep **discards** the edit |
| Fan (6) / Light (7) / Flame (8) | Change; sent ~1 s after the knob stops | Settings | 10 s -> Sleep |
| Brightness (9) | +/- 5 %, previewed live | Step 0 (Awake) -> step 1 (Sleep level) -> Settings | 10 s -> Sleep |

Rules:

- `go(to)` is the only way to change `page`. It seeds the edit buffers
  (`power_edit`, `mode_edit`, `bright_step`), applies the backlight, renders and
  loads the LVGL screen. Pages 2..9 share one LVGL screen (`page_value`).
- Every input calls `wake` first, which resets `last_activity_ms`. The idle
  timers in the 250 ms interval are all measured from `last_activity_ms`.
- A hold in progress never times out (`if (knob_button.state) return;`).
- The Settings arc lists Flame only in Manual while burning. `settings_rows()`
  in `knob_helpers.h` is the single source of the row list; `settings_sel` is
  clamped to it on every render.
- Set Point never lights the fireplace: in Manual while off the flame height is
  only stored (`flame_saved`) and sent 3 s after the next Manual turn-on.
- Backlight (`apply_backlight`): Sleep brightness on page 0 (and on page 9 step
  1 as a preview), Awake brightness elsewhere, off while `display_off` or
  `idle_sleeping`.

## 2. Button machine

Two inputs come from the hardware package: `button_down` and `button_up`
(GPIO42 through a 15 ms debounce). Everything else is derived from timing.

```
                       release before hint        => TAP  (section 1)
press --200 ms--> hint "Turning ON in 3" --1 s--> "in 2" --1 s--> "in 1" --1 s--> TOGGLE
                       release after hint, before TOGGLE  => ABORT (nothing, stay on page)
                       release after TOGGLE               => nothing
```

| Global | Meaning |
| --- | --- |
| `hold_hinted` | The hint was shown for this press: the release is not a tap (aborted or completed hold) |
| `hold_count` | Countdown while held (`hold_count` substitution = 3); the fireplace toggles when it reaches 0 |
| `press_from_sleep` | The press started on Sleep: a tap only wakes to Main, it does not open Settings |
| `boot_guard` | Ignore the button until its first release (the press that powered the board on or woke it from light sleep). Auto-clears 1 s after `guard_ms` if the button is seen released |

- `long_press_timer` (mode `restart`) runs the whole hold. `button_up` stops it.
  It bails out immediately while `loading_shown` or `boot_guard`.
- The hint moves Sleep to Main (so the countdown is readable at full
  brightness).
- There is no "stuck button" guard any more: a button held down by an object
  toggles the fireplace once after ~3.2 s, then nothing until released.
- Power in Settings (page 4) is the only other path that calls
  `toggle_fireplace`.

## 3. Data and HA-link machine

### Loading gate

`render` shows nothing but the loading screen until **all seven** values have
arrived from HA (`got_fp`, `got_light`, `got_preset`, `got_setpoint`,
`got_room`, `got_flame`, `got_fan`). "unavailable"/NaN counts as arrived.

```
boot --> LOADING (spinner, "Connecting..." / "Loading fireplace data...")
           |  all got_* true            --> MAIN  (loading_shown = false)
           |  connect_timeout_s (30 s)  --> LOADING with red error text, keeps waiting
```

- The `got_*` flags are **never cleared** by a disconnect. Once a full set has
  been shown, values stay on screen through any HA outage and are refreshed
  silently when HA is back. Only a cold boot shows the spinner.
- `stale_ok` is set by `board_idle_sleep` when the set was complete, and by
  the refresh window; `board_resume` re-asserts the flags from it so a wake
  from light sleep goes straight to Main. `api.on_client_connected` clears it.

### Link status as drawn

| Condition | Status row / Main info | Values |
| --- | --- | --- |
| connected | mode or OFF | white |
| disconnected < 5 s (`quiet`) | unchanged | white |
| disconnected 5 s .. 30 s (`connecting`) | "Connecting" + spinner | grey |
| disconnected > 30 s (`conn_timed_out`) | "No HA link" | grey |
| connected, fireplace entity not on/off | "Fireplace offline" | - |

`disconnected_since_ms` is reset on boot, on every API disconnect and in
`board_resume` (so a wake never flashes "Connecting").

### Commands

- Fan, Light, Flame, Set Point: `send_*` scripts (mode `restart`) debounce
  the knob (300-900 ms), call HA, then hold a `*_pending` flag for 4-5 s so
  the HA echo cannot overwrite the knob's value mid-way.
- Mode: sent on tap (`send_mode`), `mode_pending` for 5 s; the knob otherwise
  follows the fireplace's real preset (`ha_preset`), ignoring "Off".
- `toggle_fireplace`: off -> `switch.turn_off`; on -> `climate.set_preset_mode`
  with the selected mode, and in Manual `number.set_value` flame 3 s later.
  If HA is disconnected it sets `pending_toggle` and shows "Waiting for HA...";
  `api.on_client_connected` fires it 1.5 s after the link is back.

## 4. Power machine (M5Stack Dial)

```
                      input (tap/turn), USB current, HA refresh
   +------------------------------------------------------------------+
   |                                                                  |
   v        30 s idle on battery (power_off_after_s)                  |
 AWAKE -------------------------------------------> LIGHT SLEEP (sleep_state 1)
 sleep_state 0                                       4 s slices, Wi-Fi off, screen dark
   ^                                                  |        ^
   | board_resume(reason)                             | 60 s   | HA connected + all got_*, or 15 s
   |                                                  v        |
   |                                                REFRESH (sleep_state 2)
   |                                                awake, dark, Wi-Fi on
   |
   |   10 min of slices (light_sleep_minutes) --> POWER-OFF (latch released)
   |                                                 |
   +---- "power-off attempt" (we are still running = USB) <-+
```

| State | `sleep_state` | `idle_sleeping` | What runs |
| --- | --- | --- | --- |
| Awake | 0 | false | everything; the 200 ms interval returns immediately |
| Light sleep | 1 | true | each 200 ms tick: arm GPIO42/40/41 as wake pins, `esp_light_sleep_start` (4 s timer), then one INA219 read |
| Refresh | 2 | true | Wi-Fi on, HA reconnects and pushes values; screen stays off |
| Power-off | - | - | `board_power_off`: INA219 to sleep, `gpio_hold_dis`, HOLD low 500 ms. On battery the board is gone here |

Entering sleep (`board_idle_sleep`, called from the 250 ms interval once per
idle period via `power_off_tried`): record `stale_ok`, set the flags, Wi-Fi off,
backlight off.

Leaving sleep (`board_resume(reason)`), in this order: `sleep_state = 0`,
`idle_sleeping = false`, write `last_sleep_info`, reset the idle clock, Wi-Fi
on, `disconnected_since_ms = now`, re-assert `got_*` from `stale_ok`, `go(1)`.
Reasons: `input`, `usb`, `sleep refused` (>5 failed `esp_light_sleep_start`),
`power-off attempt`. `Last Sleep` on the HA device page shows the last one.

Wake sources while in light sleep:

| Source | Detected by | Effect |
| --- | --- | --- |
| Button press | GPIO42 wake (level) | `boot_guard` armed (the press is not a tap), `board_resume("input")` |
| Knob turn | GPIO40/41 wake | `board_resume("input")`; the ESPHome encoder then handles the click normally |
| USB plugged in | INA219 read after the slice: `battery_charging` | `board_resume("usb")` |
| Input during a refresh window | `wake` sees `idle_sleeping` | same as a button/knob wake |

USB rules:

- On USB (`on_usb`) the idle sleep never starts; the Sleep screen stays on at
  the Sleep brightness forever (`display_off_enabled: "false"` on the Dial, so
  the CrowPanel-style Display Off does not apply).
- Unplugging USB restarts the idle clock (`last_activity_ms`), so the 30 s /
  10 min countdowns start from the unplug.
- A power-off attempt on USB is harmless: `board_power_off` returns, the latch
  is re-asserted and the INA219 re-initialised, and the board resumes.

Boot paths:

| Cause | Reset Reason (HA) | Notes |
| --- | --- | --- |
| Long press on battery | power-on | The button powers the board until HOLD is high (priority 2000 `on_boot`). `boot_guard` ignores that press |
| USB plugged in while off | power-on | Hardware boots the board |
| OTA | "Reboot request from esphome.ota" | `safe_mode` marks the boot good after 60 s; a reset before that rolls back to the previous image |
| Crash | e.g. "interrupt watchdog" | The `debug` component prints the previous crash's backtrace on the next boot (serial only). Decode with `xtensa-esp32s3-elf-addr2line -pfiaC -e firmware.elf <addrs>` |

## 5. Battery monitor (INA219 in the + lead, 1 s reads)

Sign convention in firmware: `batt_i` > 0 = into the cell (charging).

| Flag | Set when | Cleared when |
| --- | --- | --- |
| `battery_absent` | `|i| > 500 mA` (shunt input floating), or `|i| < 5 mA` with > 50 mV spread over 6 s, or no INA219 reading for 3 s (`monitor_lost`) | `|i| > 5 mA`, or a steady voltage |
| `battery_charging` | present and `i > +10 mA` | otherwise |
| `on_usb` | absent, or `i > -20 mA` (the Dial always draws >= 60 mA awake, so a cell that is not supplying it means external power) | otherwise |

Percentage: coulomb counter (`soc_mah`, `battery_mah` = 2000) persisted per
whole percent, anchored to full at >= 4.18 V with the charge current tapering
below 15 mA, and to empty at <= 3.30 V discharging. "Reset Battery Gauge"
re-seeds it from the resting voltage. Main shows icon + % (hidden when absent);
Sleep shows a bolt only while charging.

## 6. Rules that keep this working

Each of these fixed a real regression. Keep them.

1. **Restore edge interrupts after every light-sleep slice.**
   `gpio_wakeup_enable(pin, LEVEL)` rewrites the pin's interrupt type and
   `gpio_wakeup_disable()` does not put it back. ESPHome's button and encoder
   ISRs (level-3 GPIO ISR service) then re-fire for as long as the pin sits at
   that level; a held button (> 300 ms) trips the interrupt watchdog and the
   Dial reboots. The slice ends with `gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE)`.
2. **`board_resume` sets `sleep_state = 0` and `idle_sleeping = false` before
   anything else**, so the 200 ms interval cannot start another slice and
   `apply_backlight` is allowed to turn the screen on.
3. **No blocking calls in lambdas** (no `delay()`, no busy waits). The 200 ms
   sleep interval is the one exception, by design: `esp_light_sleep_start`
   blocks for up to 4 s and feeds the watchdog on return.
4. **The wake press must not act.** Whatever path wakes the board on a press
   (cold boot, light-sleep GPIO wake, refresh-window input) arms `boot_guard`
   with `guard_ms`; `button_up` clears it, the 250 ms interval clears it 1 s
   after release if the sensor missed the press. Never arm it on a `usb` wake.
5. **Power latch order on boot**: drive GPIO46 high, `gpio_hold_dis`, high
   again, `gpio_hold_en`, at priority 2000, before any bus setup. The pad must
   never pass through its pulled-down default or a reboot on battery powers
   off.
6. **Display Off stays disabled on the Dial** (`display_off_enabled: "false"`
   in `hw-m5dial.yaml`). The Dial has its own sleep; on USB the dim Sleep screen
   is the intended resting state.
7. **Never clear the `got_*` flags on a disconnect.** The loading screen is for
   cold boot only; a wake from light sleep must show Main with the retained
   values and refresh silently.
8. **Keep the Dial on USB for 75-100 s after every OTA.** A reset inside the
   60 s `safe_mode` window rolls the firmware back. Verify with
   `dblogs.py ... | grep "compiled on"`.
9. **Only `toggle_fireplace` and Power (page 4) light the fireplace.** Set
   Point, Flame while off, and knob turns never do.
10. **A fireplace command while HA is disconnected is queued** (`pending_toggle`),
    not dropped; the toast says so.
11. **Crash diagnostics**: the serial log is useless for battery-side crashes
    (no USB). The `debug` component's stored crash record on the next boot is
    the evidence; keep `debug:` and the `Reset Reason` / `Last Sleep` sensors.
