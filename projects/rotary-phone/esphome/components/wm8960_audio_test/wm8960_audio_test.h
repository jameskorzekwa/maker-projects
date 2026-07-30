#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

#include "driver/i2s_std.h"

namespace esphome::wm8960_audio_test {

class WM8960AudioTest : public Component, public i2c::I2CDevice {
 public:
  void set_bclk_pin(uint8_t pin) { this->bclk_pin_ = pin; }
  void set_lrclk_pin(uint8_t pin) { this->lrclk_pin_ = pin; }
  void set_adc_pin(uint8_t pin) { this->adc_pin_ = pin; }
  void set_dac_pin(uint8_t pin) { this->dac_pin_ = pin; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  static void task_entry_(void *parameter);
  void run_test_();
  bool initialize_i2s_();
  bool initialize_codec_();
  bool write_register_(uint8_t reg, uint16_t value);
  bool play_tone_();
  bool record_audio_(int16_t *recording, size_t sample_count);
  bool play_audio_(const int16_t *recording, size_t sample_count);
  void shutdown_();

  uint8_t bclk_pin_{};
  uint8_t lrclk_pin_{};
  uint8_t adc_pin_{};
  uint8_t dac_pin_{};
  i2s_chan_handle_t tx_handle_{nullptr};
  i2s_chan_handle_t rx_handle_{nullptr};
};

}  // namespace esphome::wm8960_audio_test
