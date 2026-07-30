#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/core/component.h"

#include "driver/i2s_std.h"
#include "esp_http_server.h"

#include <cstdio>
#include <string>

namespace esphome::wm8960_audio_test {

class WM8960AudioTest : public Component, public i2c::I2CDevice {
 public:
  void set_bclk_pin(uint8_t pin) { this->bclk_pin_ = pin; }
  void set_lrclk_pin(uint8_t pin) { this->lrclk_pin_ = pin; }
  void set_adc_pin(uint8_t pin) { this->adc_pin_ = pin; }
  void set_dac_pin(uint8_t pin) { this->dac_pin_ = pin; }
  void set_headphone_output(bool headphone_output) { this->headphone_output_ = headphone_output; }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  /// Full download URL of the most recently saved message, or empty before the first save.
  std::string get_last_message_url() const;

 protected:
  enum class RecorderState : uint8_t {
    IDLE,
    WAIT_BEFORE_BEEP,
    WAITING_TO_RECORD,
    RECORDING,
    WAITING_FOR_HANGUP,
  };

  void update_hook_();
  void start_cycle_();
  void stop_cycle_();
  void begin_recorder_();
  void fail_cycle_(const char *message);
  bool initialize_message_index_();
  bool open_message_file_();
  bool write_wav_header_(uint32_t data_bytes);
  bool record_audio_block_();
  bool finalize_recording_(bool keep_file);
  bool initialize_i2s_();
  bool initialize_codec_();
  bool write_register_(uint8_t reg, uint16_t value);
  bool play_tone_();
  void shutdown_();
  void reset_cycle_metrics_();
  void start_http_server_();
  bool http_busy_() const;
  static esp_err_t handle_message_list_(httpd_req_t *req);
  static esp_err_t handle_message_file_(httpd_req_t *req);

  uint8_t bclk_pin_{};
  uint8_t lrclk_pin_{};
  uint8_t adc_pin_{};
  uint8_t dac_pin_{};
  bool headphone_output_{false};
  i2s_chan_handle_t tx_handle_{nullptr};
  i2s_chan_handle_t rx_handle_{nullptr};
  FILE *file_{nullptr};
  char temp_path_[32]{};
  char final_path_[32]{};
  uint32_t next_message_index_{1};
  bool message_index_initialized_{false};
  uint32_t saved_message_count_{0};
  httpd_handle_t http_server_{nullptr};
  uint32_t http_last_attempt_{0};
  std::string last_saved_file_{};
  size_t captured_samples_{0};
  size_t blocks_since_sync_{0};
  int32_t raw_peak_{0};
  int32_t filtered_peak_{0};
  int64_t raw_sum_squares_{0};
  bool high_pass_initialized_{false};
  float high_pass_previous_input_{0.0f};
  float high_pass_previous_output_{0.0f};
  uint32_t state_started_at_{0};
  uint32_t hook_raw_changed_at_{0};
  bool hook_initialized_{false};
  bool hook_raw_lifted_{false};
  bool hook_stable_lifted_{false};
  RecorderState state_{RecorderState::IDLE};
};

}  // namespace esphome::wm8960_audio_test
