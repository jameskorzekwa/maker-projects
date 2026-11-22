"""Regression checks for safety-critical ESPHome integration wiring."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
YAML = (ROOT / "esphome/proflame-bench.yaml").read_text()


class TemperatureWiring(unittest.TestCase):
    def test_source_and_units_share_entity(self):
        self.assertEqual(
            YAML.count("entity_id: sensor.basement_basement_presence_sensor_temperature"),
            2,
        )
        self.assertIn("attribute: unit_of_measurement", YAML)

    def test_live_subscription_not_just_logger(self):
        block = YAML.split("  - id: update_room_temp\n")[1].split(
            "  - id: tstat_eval\n"
        )[0]
        self.assertIn("is_connected_with_state_subscription()", block)
        self.assertIn("if (!connected) id(basement_temp_seen) = false;", block)
        self.assertIn("proflame_temperature::usable(", block)
        self.assertNotIn("id(remote_temp_f)", block)
        self.assertNotIn("main_floor_temperature", YAML)

    def test_no_temperature_still_turns_off(self):
        block = YAML.split("  - id: tstat_eval\n")[1].split(
            "  # Start the ESP32 thermostat"
        )[0]
        self.assertIn("if (!have_temp) {\n            want = false;", block)
        self.assertIn("(!want && !have_temp)", block)
        self.assertIn("else id(apply_cmd).execute(-1, 0,", block)

    def test_reboot_does_not_restore_thermostat(self):
        self.assertIn(
            "{id: tstat_mode, type: int, restore_value: false, initial_value: '0'}",
            YAML,
        )


if __name__ == "__main__":
    unittest.main()
