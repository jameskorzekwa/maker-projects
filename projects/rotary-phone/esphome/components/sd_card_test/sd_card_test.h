#pragma once

#include "esphome/core/component.h"

#include <cstdint>

namespace esphome::sd_card_test {

class SDCardTest : public Component {
 public:
  void set_clk_pin(uint8_t pin) { this->clk_pin_ = pin; }
  void set_cs_pin(uint8_t pin) { this->cs_pin_ = pin; }
  void set_miso_pin(uint8_t pin) { this->miso_pin_ = pin; }
  void set_mosi_pin(uint8_t pin) { this->mosi_pin_ = pin; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  uint8_t clk_pin_{};
  uint8_t cs_pin_{};
  uint8_t miso_pin_{};
  uint8_t mosi_pin_{};
  bool mounted_{false};
  const char *status_{"not run"};
  int32_t last_error_{0};
};

}  // namespace esphome::sd_card_test
