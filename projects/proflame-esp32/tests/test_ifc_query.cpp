#include "../esphome/ifc_query.h"
#include <cassert>
#include <cstring>
#include <initializer_list>

using namespace proflame_query;

static uint8_t xor5(const uint8_t *f) {
  uint8_t c = 0;
  for (int i = 0; i < 5; ++i) c ^= f[i];
  return c;
}

int main() {
  // Request frames: GET only, valid checksums, expected registers.
  for (const uint8_t *req : {GET_CONTROL, GET_STATE1, GET_STATE2}) {
    assert(req[0] == 0x5E);
    assert(xor5(req) == req[5]);
  }
  assert(GET_CONTROL[4] == 0x16 && GET_STATE1[4] == 0x17 && GET_STATE2[4] == 0x18);

  // Live reply observed from James's fireplace on 2026-11-07.
  const uint8_t live[] = {0xAE, 0xC1, 0xA0, 0x00, 0x16, 0xD9};
  assert(classify(live, 6) == Result::UART_CONTROL);
  const uint8_t ext[] = {0xAE, 0, 0, 0, 0x16, 0xB8};
  assert(classify(ext, 6) == Result::EXTERNAL_CONTROL);
  const uint8_t odd[] = {0xAE, 1, 0, 0, 0x16, 0xB9};
  assert(classify(odd, 6) == Result::UNKNOWN_VALUE);

  // Documented example: AE 00 61 00 17 D8 = mode ON, night light 6.
  const uint8_t s1[] = {0xAE, 0x00, 0x61, 0x00, 0x17, 0xD8};
  Reply r{};
  assert(parse_get_reply(s1, 6, r) && r.reg == REG_STATE1);
  State1 a = decode_state1(r);
  assert(a.mode == 1 && a.light == 6 && !a.continuous_pilot);
  assert(std::strcmp(mode_name(a.mode), "On") == 0);

  // State 1 with continuous pilot, mode OFF, light 0.
  uint8_t s1p[] = {0xAE, 0x00, 0x80, 0x00, 0x17, 0};
  s1p[5] = xor5(s1p);
  assert(parse_get_reply(s1p, 6, r));
  a = decode_state1(r);
  assert(a.mode == 0 && a.light == 0 && a.continuous_pilot);

  // Documented example: AE 00 00 00 18 B6 = everything off.
  const uint8_t s2off[] = {0xAE, 0x00, 0x00, 0x00, 0x18, 0xB6};
  assert(parse_get_reply(s2off, 6, r) && r.reg == REG_STATE2);
  State2 b = decode_state2(r);
  assert(b.flame == 0 && b.fan == 0 && !b.aux && !b.split_flow);

  // Flame 3, AUX on, fan 5, split flow on: 1 101 1 011 = 0xDB.
  uint8_t s2[] = {0xAE, 0x00, 0xDB, 0x00, 0x18, 0};
  s2[5] = xor5(s2);
  assert(parse_get_reply(s2, 6, r));
  b = decode_state2(r);
  assert(b.flame == 3 && b.aux && b.fan == 5 && b.split_flow);

  // Rejections: wrong length, bad checksum, SET echo, unknown register, null.
  assert(!parse_get_reply(nullptr, 0, r));
  for (size_t n = 0; n < 6; ++n) assert(!parse_get_reply(s1, n, r));
  for (size_t pos = 0; pos < 6; ++pos) {
    uint8_t d[6];
    std::memcpy(d, s1, 6);
    d[pos] ^= 0x01;
    assert(!parse_get_reply(d, 6, r));
  }
  const uint8_t set_echo[] = {0xAD, 0x00, 0x01, 0x00, 0x17, 0xBB};
  assert(!parse_get_reply(set_echo, 6, r));
  uint8_t other[] = {0xAE, 0, 0, 0, 0x19, 0};
  other[5] = xor5(other);
  assert(!parse_get_reply(other, 6, r));
  const uint8_t seven[] = {0xAE, 0x00, 0x61, 0x00, 0x17, 0xD8, 0x00};
  assert(!parse_get_reply(seven, 7, r));
  assert(classify(s1, 6) == Result::INVALID);  // state reply is not an ownership reply
  return 0;
}
