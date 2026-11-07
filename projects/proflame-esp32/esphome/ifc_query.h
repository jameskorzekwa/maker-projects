#pragma once

#include <cstddef>
#include <cstdint>

// Proflame 2 IFC protocol helpers: GET requests, reply parsing, the ownership
// request, and SET frame building for state registers 0x17/0x18 only.
namespace proflame_query {

inline constexpr uint8_t GET_CONTROL[] = {0x5E, 0, 0, 0, 0x16, 0x48};
inline constexpr uint8_t GET_STATE1[] = {0x5E, 0, 0, 0, 0x17, 0x49};
inline constexpr uint8_t GET_STATE2[] = {0x5E, 0, 0, 0, 0x18, 0x46};

inline constexpr uint8_t REG_CONTROL = 0x16;
inline constexpr uint8_t REG_STATE1 = 0x17;
inline constexpr uint8_t REG_STATE2 = 0x18;
// Undocumented registers identified on James's IFC (2026-11-07):
// 0x1E thermostat setpoint, whole degrees F (remote 71 -> 73 moved it 71 -> 73);
// 0x21 mirrors 0x1E; 0x1F room temperature candidate, tenths of degrees F
// (670 = 67.0 F while the remote displayed 66 F; still provisional).
inline constexpr uint8_t REG_SETPOINT = 0x1E;
inline constexpr uint8_t REG_ROOM_TEMP = 0x1F;
inline constexpr uint8_t REG_SETPOINT_MIRROR = 0x21;
inline constexpr uint8_t GET_SETPOINT[] = {0x5E, 0, 0, 0, 0x1E, 0x40};
inline constexpr uint8_t GET_ROOM_TEMP[] = {0x5E, 0, 0, 0, 0x1F, 0x41};
inline constexpr uint8_t GET_SETPOINT_MIRROR[] = {0x5E, 0, 0, 0, 0x21, 0x7F};
inline constexpr int SETPOINT_MIN_F = 41;  // 5 C, same limits as the old integration
inline constexpr int SETPOINT_MAX_F = 95;  // 35 C

// The ONLY write frame: ask the IFC for control (SET register 0x16). It does
// not change burner, flame, fan or light state. Sent only on explicit request.
inline constexpr uint8_t REQUEST_CONTROL[] = {0x5D, 0xC1, 0xA0, 0x00, 0x16, 0x2A};

enum class Result { INVALID, UART_CONTROL, EXTERNAL_CONTROL, UNKNOWN_VALUE };

// Response to REQUEST_CONTROL: AD C1 A0 00 16 DA granted, AD 00 00 00 16 BB denied.
inline Result classify_control_set_reply(const uint8_t *data, size_t size) {
  if (data == nullptr || size != 6 || data[0] != 0xAD || data[4] != 0x16) return Result::INVALID;
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; ++i) checksum ^= data[i];
  if (checksum != data[5]) return Result::INVALID;
  if (data[1] == 0xC1 && data[2] == 0xA0 && data[3] == 0) return Result::UART_CONTROL;
  if (data[1] == 0 && data[2] == 0 && data[3] == 0) return Result::EXTERNAL_CONTROL;
  return Result::UNKNOWN_VALUE;
}

struct Reply {
  uint8_t reg;
  uint8_t payload[3];
};

// Accepts exactly one six-byte GET response (0xAE) for a known register with a
// valid XOR checksum. Anything else (SET echoes, noise, partial frames) fails.
inline bool parse_get_reply(const uint8_t *data, size_t size, Reply &out) {
  if (data == nullptr || size != 6 || data[0] != 0xAE) return false;
  const uint8_t reg = data[4];
  if (reg != REG_CONTROL && reg != REG_STATE1 && reg != REG_STATE2 && reg != REG_SETPOINT &&
      reg != REG_ROOM_TEMP && reg != REG_SETPOINT_MIRROR)
    return false;
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

// ---- Command encoding (SET 0x5D on registers 0x17 / 0x18 only) ----
// Field setters change ONE field of the current raw register byte and keep
// every other bit (including undocumented bits and out-of-range values).
inline uint8_t clamp6(int v) { return v < 0 ? 0 : (v > 6 ? 6 : static_cast<uint8_t>(v)); }
inline uint8_t with_mode(uint8_t b, int mode) { return static_cast<uint8_t>((b & ~0x03) | (mode & 0x03)); }
inline uint8_t with_light(uint8_t b, int v) { return static_cast<uint8_t>((b & ~0x70) | (clamp6(v) << 4)); }
inline uint8_t with_pilot(uint8_t b, bool on) { return static_cast<uint8_t>(on ? (b | 0x80) : (b & 0x7F)); }
inline uint8_t with_flame(uint8_t b, int v) { return static_cast<uint8_t>((b & ~0x07) | clamp6(v)); }
inline uint8_t with_aux(uint8_t b, bool on) { return static_cast<uint8_t>(on ? (b | 0x08) : (b & ~0x08)); }
inline uint8_t with_fan(uint8_t b, int v) { return static_cast<uint8_t>((b & ~0x70) | (clamp6(v) << 4)); }
inline uint8_t with_split(uint8_t b, bool on) { return static_cast<uint8_t>(on ? (b | 0x80) : (b & 0x7F)); }

// Builds a SET frame for register 0x17 or 0x18. Returns false for any other
// register so ownership (0x16) and unknown registers can never be written here.
inline bool build_set(uint8_t reg, uint8_t value, uint8_t out[6]) {
  if (reg != REG_STATE1 && reg != REG_STATE2) return false;
  out[0] = 0x5D;
  out[1] = 0x00;
  out[2] = value;
  out[3] = 0x00;
  out[4] = reg;
  out[5] = out[0] ^ out[1] ^ out[2] ^ out[3] ^ out[4];
  return true;
}
// Thermostat setpoint SET frame (register 0x1E only). Refuses values outside
// 41-95 F instead of clamping, so a bad conversion can never be written.
inline bool build_setpoint(int fahrenheit, uint8_t out[6]) {
  if (fahrenheit < SETPOINT_MIN_F || fahrenheit > SETPOINT_MAX_F) return false;
  out[0] = 0x5D;
  out[1] = 0x00;
  out[2] = static_cast<uint8_t>(fahrenheit);
  out[3] = 0x00;
  out[4] = REG_SETPOINT;
  out[5] = out[0] ^ out[1] ^ out[2] ^ out[3] ^ out[4];
  return true;
}
inline int setpoint_f(const Reply &r) { return r.payload[1]; }
inline float room_temp_f(const Reply &r) { return ((r.payload[0] << 8) | r.payload[1]) / 10.0f; }

// SET echo for state registers (AD 00 vv 00 reg cs); used only for logging.
inline bool is_state_set_echo(const uint8_t *data, size_t size) {
  if (data == nullptr || size != 6 || data[0] != 0xAD) return false;
  if (data[4] != REG_STATE1 && data[4] != REG_STATE2 && data[4] != REG_SETPOINT) return false;
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; ++i) checksum ^= data[i];
  return checksum == data[5];
}

inline const char *mode_name(uint8_t mode) {
  switch (mode) {
    case 0: return "Off";
    case 1: return "Manual";
    case 2: return "Thermostat";
    case 3: return "Smart";
    default: return "Unknown";
  }
}

}  // namespace proflame_query
