#include "wm8960_audio_test.h"

#include "esphome/core/log.h"

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace esphome::wm8960_audio_test {

static const char *const TAG = "wm8960_audio_test";
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t RECORD_SECONDS = 4;
static constexpr size_t RECORD_SAMPLES = SAMPLE_RATE * RECORD_SECONDS;
static constexpr size_t FRAMES_PER_BLOCK = 256;
static constexpr float PI = 3.14159265358979323846f;

void WM8960AudioTest::setup() {
  // Reset immediately so a previous interrupted test cannot leave the speaker amplifiers enabled.
  if (!this->write_register_(0x0F, 0x000)) {
    ESP_LOGE(TAG, "Could not reset the WM8960 codec");
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Stock audio test will start in 5 seconds");
  if (xTaskCreate(WM8960AudioTest::task_entry_, "wm8960_test", 6144, this, 1, nullptr) != pdPASS) {
    ESP_LOGE(TAG, "Could not create the audio test task");
    this->mark_failed();
  }
}

void WM8960AudioTest::dump_config() {
  ESP_LOGCONFIG(TAG, "WM8960 stock audio test:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Sample rate: %" PRIu32 " Hz", SAMPLE_RATE);
  ESP_LOGCONFIG(TAG, "  BCLK: GPIO%u", this->bclk_pin_);
  ESP_LOGCONFIG(TAG, "  LRCLK: GPIO%u", this->lrclk_pin_);
  ESP_LOGCONFIG(TAG, "  ADC data: GPIO%u", this->adc_pin_);
  ESP_LOGCONFIG(TAG, "  DAC data: GPIO%u", this->dac_pin_);
}

void WM8960AudioTest::task_entry_(void *parameter) {
  auto *test = static_cast<WM8960AudioTest *>(parameter);
  vTaskDelay(pdMS_TO_TICKS(5000));
  test->run_test_();
  vTaskDelete(nullptr);
}

void WM8960AudioTest::run_test_() {
  auto *recording = static_cast<int16_t *>(heap_caps_malloc(RECORD_SAMPLES * sizeof(int16_t), MALLOC_CAP_8BIT));
  if (recording == nullptr) {
    ESP_LOGE(TAG, "Could not allocate %u bytes for the recording", RECORD_SAMPLES * sizeof(int16_t));
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Initializing the ESP32 I2S peripheral");
  if (!this->initialize_i2s_()) {
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Initializing the WM8960 codec at low speaker volume");
  if (!this->initialize_codec_()) {
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Playing the start beep");
  if (!this->play_tone_()) {
    ESP_LOGE(TAG, "Could not play the start beep");
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }

  if (!this->write_register_(0x05, 0x008)) {
    ESP_LOGE(TAG, "Could not mute the speakers before recording");
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(250));
  ESP_LOGI(TAG, "RECORDING NOW: speak toward either silver onboard microphone for 4 seconds");
  if (!this->record_audio_(recording, RECORD_SAMPLES)) {
    ESP_LOGE(TAG, "Could not capture microphone audio");
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Recording complete; playing it through both test speakers");
  if (!this->write_register_(0x05, 0x000)) {
    ESP_LOGE(TAG, "Could not unmute the speakers for playback");
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }
  if (!this->play_audio_(recording, RECORD_SAMPLES)) {
    ESP_LOGE(TAG, "Could not play the captured audio");
    this->shutdown_();
    heap_caps_free(recording);
    this->mark_failed();
    return;
  }

  heap_caps_free(recording);
  this->shutdown_();
  ESP_LOGI(TAG, "AUDIO TEST COMPLETE: press Reset to repeat the test");
}

bool WM8960AudioTest::initialize_i2s_() {
  i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  channel_config.dma_desc_num = 6;
  channel_config.dma_frame_num = FRAMES_PER_BLOCK;

  esp_err_t error = i2s_new_channel(&channel_config, &this->tx_handle_, &this->rx_handle_);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(error));
    return false;
  }

  i2s_std_config_t standard_config{};
  standard_config.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE);
  standard_config.slot_cfg =
      I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  standard_config.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  standard_config.gpio_cfg.bclk = static_cast<gpio_num_t>(this->bclk_pin_);
  standard_config.gpio_cfg.ws = static_cast<gpio_num_t>(this->lrclk_pin_);
  standard_config.gpio_cfg.dout = static_cast<gpio_num_t>(this->dac_pin_);
  standard_config.gpio_cfg.din = static_cast<gpio_num_t>(this->adc_pin_);
  standard_config.gpio_cfg.invert_flags.mclk_inv = false;
  standard_config.gpio_cfg.invert_flags.bclk_inv = false;
  standard_config.gpio_cfg.invert_flags.ws_inv = false;

  error = i2s_channel_init_std_mode(this->tx_handle_, &standard_config);
  if (error == ESP_OK)
    error = i2s_channel_init_std_mode(this->rx_handle_, &standard_config);
  if (error == ESP_OK)
    error = i2s_channel_enable(this->tx_handle_);
  if (error == ESP_OK)
    error = i2s_channel_enable(this->rx_handle_);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "I2S initialization failed: %s", esp_err_to_name(error));
    return false;
  }

  std::array<int16_t, FRAMES_PER_BLOCK * 2> silence{};
  size_t written = 0;
  error = i2s_channel_write(this->tx_handle_, silence.data(), sizeof(silence), &written, 1000);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Could not start I2S clocks: %s", esp_err_to_name(error));
    return false;
  }
  return true;
}

bool WM8960AudioTest::initialize_codec_() {
  struct RegisterValue {
    uint8_t reg;
    uint16_t value;
    uint16_t delay_ms;
  };

  static constexpr RegisterValue INITIALIZATION[] = {
      {0x0F, 0x000, 0},  // Reset.
      {0x1C, 0x09C, 0},  // VMID soft start and anti-pop.
      {0x19, 0x080, 100},
      {0x19, 0x0C0, 0},  // VREF on.
      {0x1C, 0x008, 0},
      {0x34, 0x038, 0},  // 24 MHz MCLK PLL: N=8, K=0x3126E9.
      {0x35, 0x031, 0},
      {0x36, 0x026, 0},
      {0x37, 0x0E9, 0},
      {0x1A, 0x001, 250},  // PLL on.
      {0x04, 0x0DD, 0},   // PLL source, SYSCLK /2, ADC and DAC /3 for 16 kHz.
      {0x07, 0x002, 0},   // Peripheral mode, Philips I2S, 16-bit samples.
      {0x06, 0x008, 0},   // Gradual DAC soft-unmute.
      {0x00, 0x027, 0},   // Left input PGA +12 dB.
      {0x01, 0x127, 0},   // Right input PGA +12 dB; update both.
      {0x20, 0x138, 0},   // Left input boost path.
      {0x21, 0x138, 0},   // Right input boost path.
      {0x22, 0x150, 0},   // Left DAC to output mixer.
      {0x25, 0x150, 0},   // Right DAC to output mixer.
      {0x28, 0x06D, 0},   // Left speaker PGA -12 dB.
      {0x29, 0x16D, 0},   // Right speaker PGA -12 dB; update both.
      {0x2F, 0x03C, 0},   // Input PGAs and output mixers on.
      {0x19, 0x0FC, 0},   // VMID, VREF, inputs and ADCs on.
      {0x1A, 0x199, 0},   // DACs, speaker PGAs and PLL on.
      {0x31, 0x0F7, 0},   // Both class-D outputs on.
      {0x05, 0x000, 0},   // DAC unmuted.
  };

  for (const auto &entry : INITIALIZATION) {
    if (!this->write_register_(entry.reg, entry.value)) {
      ESP_LOGE(TAG, "WM8960 register 0x%02X write failed", entry.reg);
      return false;
    }
    if (entry.delay_ms != 0)
      vTaskDelay(pdMS_TO_TICKS(entry.delay_ms));
  }
  return true;
}

bool WM8960AudioTest::write_register_(uint8_t reg, uint16_t value) {
  const uint8_t data[2] = {
      static_cast<uint8_t>((reg << 1U) | ((value >> 8U) & 0x01U)),
      static_cast<uint8_t>(value & 0xFFU),
  };
  return this->write(data, sizeof(data)) == i2c::ERROR_OK;
}

bool WM8960AudioTest::play_tone_() {
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  constexpr size_t frames = SAMPLE_RATE / 3;
  size_t completed = 0;

  while (completed < frames) {
    const size_t block_frames = std::min(FRAMES_PER_BLOCK, frames - completed);
    for (size_t index = 0; index < block_frames; index++) {
      const float phase = 2.0f * PI * 880.0f * static_cast<float>(completed + index) / SAMPLE_RATE;
      const int16_t sample = static_cast<int16_t>(std::sin(phase) * 1000.0f);
      stereo[index * 2] = sample;
      stereo[index * 2 + 1] = sample;
    }
    size_t written = 0;
    if (i2s_channel_write(this->tx_handle_, stereo.data(), block_frames * 2 * sizeof(int16_t), &written, 1000) !=
        ESP_OK) {
      return false;
    }
    completed += block_frames;
  }

  stereo.fill(0);
  size_t written = 0;
  if (i2s_channel_write(this->tx_handle_, stereo.data(), sizeof(stereo), &written, 1000) != ESP_OK)
    return false;
  return true;
}

bool WM8960AudioTest::record_audio_(int16_t *recording, size_t sample_count) {
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  size_t captured = 0;
  int32_t peak = 0;

  while (captured < sample_count) {
    size_t bytes_read = 0;
    const esp_err_t error = i2s_channel_read(this->rx_handle_, stereo.data(), sizeof(stereo), &bytes_read, 1000);
    if (error != ESP_OK) {
      ESP_LOGE(TAG, "I2S microphone read failed: %s", esp_err_to_name(error));
      return false;
    }
    const size_t frames_read = bytes_read / (2 * sizeof(int16_t));
    const size_t frames_to_copy = std::min(frames_read, sample_count - captured);
    for (size_t index = 0; index < frames_to_copy; index++) {
      const int16_t sample = stereo[index * 2];
      recording[captured + index] = sample;
      peak = std::max(peak, std::abs(static_cast<int32_t>(sample)));
    }
    captured += frames_to_copy;
  }
  ESP_LOGI(TAG, "Captured microphone peak: %" PRIi32 " of 32767", peak);
  return true;
}

bool WM8960AudioTest::play_audio_(const int16_t *recording, size_t sample_count) {
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  size_t played = 0;

  while (played < sample_count) {
    const size_t block_frames = std::min(FRAMES_PER_BLOCK, sample_count - played);
    for (size_t index = 0; index < block_frames; index++) {
      const int16_t sample = recording[played + index] / 2;
      stereo[index * 2] = sample;
      stereo[index * 2 + 1] = sample;
    }
    size_t written = 0;
    const esp_err_t error =
        i2s_channel_write(this->tx_handle_, stereo.data(), block_frames * 2 * sizeof(int16_t), &written, 1000);
    if (error != ESP_OK) {
      ESP_LOGE(TAG, "I2S speaker write failed: %s", esp_err_to_name(error));
      return false;
    }
    played += block_frames;
  }

  std::array<int16_t, FRAMES_PER_BLOCK * 2> silence{};
  size_t written = 0;
  i2s_channel_write(this->tx_handle_, silence.data(), sizeof(silence), &written, 1000);
  vTaskDelay(pdMS_TO_TICKS(100));
  return true;
}

void WM8960AudioTest::shutdown_() {
  if (this->tx_handle_ != nullptr) {
    this->write_register_(0x05, 0x008);
    vTaskDelay(pdMS_TO_TICKS(35));
    this->write_register_(0x31, 0x037);
    this->write_register_(0x1A, 0x001);
    this->write_register_(0x2F, 0x000);
    this->write_register_(0x19, 0x0C0);
    this->write_register_(0x04, 0x0DC);
    this->write_register_(0x1A, 0x000);
    this->write_register_(0x1C, 0x09C);
    this->write_register_(0x19, 0x000);

    i2s_channel_disable(this->rx_handle_);
    i2s_channel_disable(this->tx_handle_);
    i2s_del_channel(this->rx_handle_);
    i2s_del_channel(this->tx_handle_);
    this->rx_handle_ = nullptr;
    this->tx_handle_ = nullptr;
  }
}

}  // namespace esphome::wm8960_audio_test
