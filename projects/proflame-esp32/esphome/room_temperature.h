#pragma once
#include <cmath>
#include <cstdint>
#include <string>

namespace proflame_temperature {
// Fail closed after 30 minutes without a HA temperature state delivery.
constexpr uint32_t MAX_AGE_MS = 30u * 60u * 1000u;
inline float fahrenheit(float value, const std::string &unit) {
  if (!std::isfinite(value)) return NAN;
  float f = NAN;
  if (unit == "°F") f = value;
  if (unit == "°C") f = value * 9.0f / 5.0f + 32.0f;
  return std::isfinite(f) && f > 20.0f && f < 120.0f ? f : NAN;
}
inline float usable(float value, const std::string &unit, bool connected,
                    bool seen, uint32_t now, uint32_t received) {
  if (!connected || !seen || uint32_t(now - received) >= MAX_AGE_MS) return NAN;
  return fahrenheit(value, unit);
}
}  // namespace proflame_temperature
