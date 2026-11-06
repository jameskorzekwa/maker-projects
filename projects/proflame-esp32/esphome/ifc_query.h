#pragma once

#include <cstddef>
#include <cstdint>

namespace proflame_query {
// GET ownership register 0x16. Never SET ownership or actuator state.
inline constexpr uint8_t GET_CONTROL[] = {0x5E, 0, 0, 0, 0x16, 0x48};
enum class Result { INVALID, UART_CONTROL, EXTERNAL_CONTROL, UNKNOWN_VALUE };

inline Result classify(const uint8_t *data, size_t size) {
  if (size != 6 || data[0] != 0xAE || data[4] != 0x16) return Result::INVALID;
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; ++i) checksum ^= data[i];
  if (checksum != data[5]) return Result::INVALID;
  if (data[1] == 0xC1 && data[2] == 0xA0 && data[3] == 0)
    return Result::UART_CONTROL;
  if (data[1] == 0 && data[2] == 0 && data[3] == 0)
    return Result::EXTERNAL_CONTROL;
  return Result::UNKNOWN_VALUE;
}
}  // namespace proflame_query
