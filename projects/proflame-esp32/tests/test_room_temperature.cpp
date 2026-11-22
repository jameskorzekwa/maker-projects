#include "../esphome/room_temperature.h"
#include <cassert>
#include <limits>

int main() {
  using namespace proflame_temperature;
  assert(std::abs(fahrenheit(62.258f, "°F") - 62.258f) < 0.001f);
  assert(std::abs(fahrenheit(20.0f, "°C") - 68.0f) < 0.001f);
  for (auto unit : {"", "K", "unknown", "unavailable"})
    assert(std::isnan(fahrenheit(62.0f, unit)));
  for (float value : {NAN, INFINITY, -INFINITY, 20.0f, 120.0f, -40.0f})
    assert(std::isnan(fahrenheit(value, "°F")));
  assert(std::isnan(usable(62, "°F", false, true, 100, 0)));
  assert(std::isnan(usable(62, "°F", true, false, 100, 0)));
  assert(usable(62, "°F", true, true, MAX_AGE_MS - 1, 0) == 62);
  assert(std::isnan(usable(62, "°F", true, true, MAX_AGE_MS, 0)));
  assert(usable(62, "°F", true, true, 10, UINT32_MAX - 10) == 62);
}
