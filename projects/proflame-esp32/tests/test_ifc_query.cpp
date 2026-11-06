#include "../esphome/ifc_query.h"
#include <cassert>

int main() {
  using namespace proflame_query;
  const uint8_t uart[] = {0xAE, 0xC1, 0xA0, 0, 0x16, 0xD9};
  const uint8_t other[] = {0xAE, 0, 0, 0, 0x16, 0xB8};
  const uint8_t unfamiliar[] = {0xAE, 1, 0, 0, 0x16, 0xB9};
  assert(classify(uart, 6) == Result::UART_CONTROL);
  assert(classify(other, 6) == Result::EXTERNAL_CONTROL);
  assert(classify(unfamiliar, 6) == Result::UNKNOWN_VALUE);
  assert(classify(nullptr, 0) == Result::INVALID);
  for (size_t size = 1; size < 6; ++size)
    assert(classify(uart, size) == Result::INVALID);
  uint8_t damaged[6];
  for (size_t pos = 0; pos < 6; ++pos) {
    for (size_t i = 0; i < 6; ++i) damaged[i] = uart[i];
    damaged[pos] ^= 1;
    assert(classify(damaged, 6) == Result::INVALID);
  }
  const uint8_t set_echo[] = {0xAD, 0xC1, 0xA0, 0, 0x16, 0xDA};
  const uint8_t other_register[] = {0xAE, 0, 0, 0, 0x17, 0xB9};
  assert(classify(set_echo, 6) == Result::INVALID);
  assert(classify(other_register, 6) == Result::INVALID);
  uint8_t checksum = 0;
  for (size_t i = 0; i < 5; ++i) checksum ^= GET_CONTROL[i];
  assert(GET_CONTROL[0] == 0x5E && GET_CONTROL[4] == 0x16);
  assert(checksum == GET_CONTROL[5]);
}
