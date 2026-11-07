#pragma once

#include <cstddef>
#include <cstdint>

// Read-only Proflame 2 IFC helpers. Only GET requests (0x5E) are defined here;
// there are deliberately no SET (0x5D) frames in this file.
namespace proflame_query {

inline constexpr uint8_t GET_CONTROL[] = {0x5E, 0, 0, 0, 0x16, 0x48};
inline constexpr uint8_t GET_STATE1[] = {0x5E, 0, 0, 0, 0x17, 0x49};
inline constexpr uint8_t GET_STATE2[] = {0x5E, 0, 0, 0, 0x18, 0x46};

inline constexpr uint8_t REG_CONTROL = 0x16;
inline constexpr uint8_t REG_STATE1 = 0x17;
inline constexpr uint8_t REG_STATE2 = 0x18;

enum class Result { INVALID, UART_CONTROL, EXTERNAL_CONTROL, UNKNOWN_VALUE };

struct Reply {
  uint8_t reg;
  uint8_t payload[3];
};

// Accepts exactly one six-byte GET response (0xAE) for a known register with a
// valid XOR checksum. Anything else (SET echoes, noise, partial frames) fails.
inline bool parse_get_reply(const uint8_t *data, size_t size, Reply &out) {
  if (data == nullptr || size != 6 || data[0] != 0xAE) return false;
  if (data[4] != REG_CONTROL && data[4] != REG_STATE1 && data[4] != REG_STATE2) return false;
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; ++i) checksum ^= data[i];
  if (checksum != data[5]) return false;
  out.reg = data[4];
  out.payload[0] = data[1];
  out.payload[1] = data[2];
  out.payload[2] = data[3];
  return true;
}

inline Result control_state(const Reply &r) {
  if (r.payload[0] == 0xC1 && r.payload[1] == 0xA0 && r.payload[2] == 0) return Result::UART_CONTROL;
  if (r.payload[0] == 0 && r.payload[1] == 0 && r.payload[2] == 0) return Result::EXTERNAL_CONTROL;
  return Result::UNKNOWN_VALUE;
}

// Backwards-compatible single-frame ownership classifier.
inline Result classify(const uint8_t *data, size_t size) {
  Reply r{};
  if (!parse_get_reply(data, size, r) || r.reg != REG_CONTROL) return Result::INVALID;
  return control_state(r);
}

// Register 0x17 / 0x18 bit layout per the documented Proflame 2 protocol,
// carried in the middle payload byte (e.g. AE 00 61 00 17 = mode ON, light 6).
// Observed on James's fireplace while physically OFF: state-1 byte 0x04. The
// documented modes 0-3 fit in bits 0-1, so mode is decoded from those bits and
// bits 2-3 are reported separately as undocumented flags (bit 2 set when off).
struct State1 {
  uint8_t mode;        // 0 OFF, 1 ON, 2 THERMOSTAT, 3 SMART (bits 0-1)
  uint8_t flags;       // bits 2-3, meaning undocumented
  uint8_t light;       // 0-6 (7 = out of documented range)
  bool continuous_pilot;
};
struct State2 {
  uint8_t flame;       // 0-6
  bool aux;
  uint8_t fan;         // 0-6
  bool split_flow;
};

inline State1 decode_state1(const Reply &r) {
  const uint8_t b = r.payload[1];
  return State1{static_cast<uint8_t>(b & 0x03), static_cast<uint8_t>((b >> 2) & 0x03),
                static_cast<uint8_t>((b >> 4) & 0x07), static_cast<bool>(b & 0x80)};
}
inline State2 decode_state2(const Reply &r) {
  const uint8_t b = r.payload[1];
  return State2{static_cast<uint8_t>(b & 0x07), static_cast<bool>(b & 0x08),
                static_cast<uint8_t>((b >> 4) & 0x07), static_cast<bool>(b & 0x80)};
}

inline const char *mode_name(uint8_t mode) {
  switch (mode) {
    case 0: return "Off";
    case 1: return "On";
    case 2: return "Thermostat";
    case 3: return "Smart";
    default: return "Unknown";
  }
}

}  // namespace proflame_query
