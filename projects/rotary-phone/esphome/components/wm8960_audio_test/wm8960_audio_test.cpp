#include "wm8960_audio_test.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "driver/gpio.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace esphome::wm8960_audio_test {

static const char *const TAG = "wm8960_audio_test";
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t FRAMES_PER_BLOCK = 256;
static constexpr size_t MAX_RECORD_SAMPLES = SAMPLE_RATE * 300;  // Five-minute message limit.
static constexpr size_t MIN_KEEP_SAMPLES = SAMPLE_RATE / 2;      // Discard sub-half-second messages.
static constexpr size_t BLOCKS_PER_SYNC = 62;                    // Roughly one fsync per second.
static constexpr uint32_t LIFT_TO_BEEP_DELAY_MS = 2000;          // Time to raise the handset to the ear.
static constexpr uint32_t WAV_HEADER_BYTES = 44;
static constexpr float PI = 3.14159265358979323846f;

void WM8960AudioTest::setup() {
  // Reset immediately so a previous interrupted cycle cannot leave the amplifiers enabled.
  if (!this->write_register_(0x0F, 0x000)) {
    ESP_LOGE(TAG, "Could not reset the WM8960 codec");
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Guestbook recorder armed; lift the handset to record a message");
}

void WM8960AudioTest::loop() {
  this->update_hook_();
  switch (this->state_) {
    case RecorderState::IDLE:
    case RecorderState::WAITING_FOR_HANGUP:
      break;
    case RecorderState::WAIT_BEFORE_BEEP:
      if (millis() - this->state_started_at_ >= LIFT_TO_BEEP_DELAY_MS)
        this->begin_recorder_();
      break;
    case RecorderState::WAITING_TO_RECORD:
      if (millis() - this->state_started_at_ >= 250) {
        std::array<int16_t, FRAMES_PER_BLOCK * 2> discarded_audio{};
        for (size_t block = 0; block < 8; block++) {
          size_t bytes_read = 0;
          if (i2s_channel_read(this->rx_handle_, discarded_audio.data(), sizeof(discarded_audio), &bytes_read, 0) !=
                  ESP_OK ||
              bytes_read == 0)
            break;
        }
        ESP_LOGI(TAG, "RECORDING %s: speak into the handset until it is hung up", this->final_path_);
        this->state_ = RecorderState::RECORDING;
      }
      break;
    case RecorderState::RECORDING:
      if (!this->record_audio_block_()) {
        this->finalize_recording_(false);
        this->fail_cycle_("Audio capture or card write failed");
      } else if (this->captured_samples_ >= MAX_RECORD_SAMPLES) {
        ESP_LOGI(TAG, "Reached the five-minute limit; finalizing the message");
        this->finalize_recording_(true);
        this->shutdown_();
        this->state_ = RecorderState::WAITING_FOR_HANGUP;
      }
      break;
  }
}

void WM8960AudioTest::update_hook_() {
  static constexpr gpio_num_t HOOK_GPIO = GPIO_NUM_17;
  static constexpr uint32_t HOOK_DEBOUNCE_MS = 150;
  const bool lifted = gpio_get_level(HOOK_GPIO) != 0;
  const uint32_t now = millis();

  if (!this->hook_initialized_) {
    this->hook_initialized_ = true;
    this->hook_raw_lifted_ = lifted;
    this->hook_raw_changed_at_ = now;
    return;
  }
  if (lifted != this->hook_raw_lifted_) {
    this->hook_raw_lifted_ = lifted;
    this->hook_raw_changed_at_ = now;
  }
  if (lifted == this->hook_stable_lifted_ || now - this->hook_raw_changed_at_ < HOOK_DEBOUNCE_MS)
    return;

  this->hook_stable_lifted_ = lifted;
  if (lifted)
    this->start_cycle_();
  else
    this->stop_cycle_();
}

void WM8960AudioTest::start_cycle_() {
  if (this->state_ != RecorderState::IDLE || this->is_failed())
    return;
  this->reset_cycle_metrics_();
  this->state_started_at_ = millis();
  this->state_ = RecorderState::WAIT_BEFORE_BEEP;
  ESP_LOGI(TAG, "Handset lifted; beep in %.1f seconds", LIFT_TO_BEEP_DELAY_MS / 1000.0f);
}

void WM8960AudioTest::stop_cycle_() {
  switch (this->state_) {
    case RecorderState::IDLE:
      return;
    case RecorderState::RECORDING:
      this->finalize_recording_(true);
      this->shutdown_();
      break;
    case RecorderState::WAITING_TO_RECORD:
      this->finalize_recording_(false);
      this->shutdown_();
      ESP_LOGI(TAG, "Handset replaced before recording started; nothing saved");
      break;
    case RecorderState::WAIT_BEFORE_BEEP:
      ESP_LOGI(TAG, "Handset replaced before the beep; nothing saved");
      break;
    case RecorderState::WAITING_FOR_HANGUP:
      break;
  }
  this->state_ = RecorderState::IDLE;
  ESP_LOGI(TAG, "Ready for the next message");
}

void WM8960AudioTest::begin_recorder_() {
  if (!this->message_index_initialized_ && !this->initialize_message_index_()) {
    this->fail_cycle_("Could not scan the card for existing messages");
    return;
  }
  if (!this->initialize_codec_()) {
    this->fail_cycle_("Could not initialize the WM8960 codec");
    return;
  }
  if (!this->initialize_i2s_()) {
    this->fail_cycle_("Could not initialize the ESP32 I2S peripheral");
    return;
  }
  if (!this->open_message_file_()) {
    this->shutdown_();
    this->fail_cycle_("Could not create the message file on the card");
    return;
  }
  if (!this->play_tone_()) {
    this->finalize_recording_(false);
    this->fail_cycle_("Could not play the beep");
    return;
  }
  this->state_started_at_ = millis();
  this->state_ = RecorderState::WAITING_TO_RECORD;
}

void WM8960AudioTest::fail_cycle_(const char *message) {
  ESP_LOGE(TAG, "%s", message);
  this->shutdown_();
  this->state_ = RecorderState::WAITING_FOR_HANGUP;
}

bool WM8960AudioTest::initialize_message_index_() {
  DIR *dir = opendir("/sdcard");
  if (dir == nullptr)
    return false;
  uint32_t highest = 0;
  for (struct dirent *entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
    unsigned index = 0;
    char suffix[8]{};
    if (sscanf(entry->d_name, "MSG%5u.%3s", &index, suffix) == 2 &&
        (strcmp(suffix, "WAV") == 0 || strcmp(suffix, "TMP") == 0)) {
      highest = std::max<uint32_t>(highest, index);
    }
  }
  closedir(dir);
  this->next_message_index_ = highest + 1;
  this->message_index_initialized_ = true;
  ESP_LOGI(TAG, "Next message number is %" PRIu32, this->next_message_index_);
  return true;
}

bool WM8960AudioTest::open_message_file_() {
  snprintf(this->temp_path_, sizeof(this->temp_path_), "/sdcard/MSG%05" PRIu32 ".TMP", this->next_message_index_);
  snprintf(this->final_path_, sizeof(this->final_path_), "/sdcard/MSG%05" PRIu32 ".WAV", this->next_message_index_);
  this->file_ = fopen(this->temp_path_, "wb");
  if (this->file_ == nullptr)
    return false;
  return this->write_wav_header_(0);
}

bool WM8960AudioTest::write_wav_header_(uint32_t data_bytes) {
  uint8_t header[WAV_HEADER_BYTES];
  const uint32_t riff_size = 36 + data_bytes;
  const uint32_t byte_rate = SAMPLE_RATE * 2;
  memcpy(header, "RIFF", 4);
  memcpy(header + 4, &riff_size, 4);
  memcpy(header + 8, "WAVEfmt ", 8);
  const uint32_t fmt_size = 16;
  memcpy(header + 16, &fmt_size, 4);
  const uint16_t pcm_format = 1, channels = 1, block_align = 2, bits_per_sample = 16;
  memcpy(header + 20, &pcm_format, 2);
  memcpy(header + 22, &channels, 2);
  memcpy(header + 24, &SAMPLE_RATE, 4);
  memcpy(header + 28, &byte_rate, 4);
  memcpy(header + 32, &block_align, 2);
  memcpy(header + 34, &bits_per_sample, 2);
  memcpy(header + 36, "data", 4);
  memcpy(header + 40, &data_bytes, 4);
  if (fseek(this->file_, 0, SEEK_SET) != 0)
    return false;
  return fwrite(header, 1, sizeof(header), this->file_) == sizeof(header);
}

bool WM8960AudioTest::record_audio_block_() {
  // A gentle 100 Hz high-pass rejects hum without thinning normal telephone speech.
  static constexpr float HIGH_PASS_ALPHA = 0.96221f;
  std::array<int16_t, FRAMES_PER_BLOCK * 2> silence{};
  size_t bytes_written = 0;
  esp_err_t error = i2s_channel_write(this->tx_handle_, silence.data(), sizeof(silence), &bytes_written, 1000);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "I2S silence write failed: %s", esp_err_to_name(error));
    return false;
  }

  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  size_t bytes_read = 0;
  error = i2s_channel_read(this->rx_handle_, stereo.data(), sizeof(stereo), &bytes_read, 1000);
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "I2S microphone read failed: %s", esp_err_to_name(error));
    return false;
  }

  std::array<int16_t, FRAMES_PER_BLOCK> mono{};
  const size_t frames_read = bytes_read / (2 * sizeof(int16_t));
  const size_t frames_to_keep = std::min(frames_read, MAX_RECORD_SAMPLES - this->captured_samples_);
  for (size_t index = 0; index < frames_to_keep; index++) {
    const int16_t slot_1 = stereo[index * 2 + 1];
    if (!this->high_pass_initialized_) {
      this->high_pass_previous_input_ = slot_1;
      this->high_pass_initialized_ = true;
    }
    const float filtered = HIGH_PASS_ALPHA *
                           (this->high_pass_previous_output_ + slot_1 - this->high_pass_previous_input_);
    this->high_pass_previous_input_ = slot_1;
    this->high_pass_previous_output_ = filtered;
    const int16_t sample = static_cast<int16_t>(std::clamp<int32_t>(std::lround(filtered), -32768, 32767));
    mono[index] = sample;
    this->raw_peak_ = std::max(this->raw_peak_, std::abs(static_cast<int32_t>(slot_1)));
    this->filtered_peak_ = std::max(this->filtered_peak_, std::abs(static_cast<int32_t>(sample)));
    this->raw_sum_squares_ += static_cast<int64_t>(slot_1) * slot_1;
  }
  if (frames_to_keep > 0 &&
      fwrite(mono.data(), sizeof(int16_t), frames_to_keep, this->file_) != frames_to_keep) {
    ESP_LOGE(TAG, "microSD write failed at sample %u", this->captured_samples_);
    return false;
  }
  this->captured_samples_ += frames_to_keep;
  if (++this->blocks_since_sync_ >= BLOCKS_PER_SYNC) {
    this->blocks_since_sync_ = 0;
    if (fflush(this->file_) != 0 || fsync(fileno(this->file_)) != 0) {
      ESP_LOGE(TAG, "microSD flush failed at sample %u", this->captured_samples_);
      return false;
    }
  }
  return true;
}

bool WM8960AudioTest::finalize_recording_(bool keep_file) {
  if (this->file_ == nullptr)
    return true;
  const bool keep = keep_file && this->captured_samples_ >= MIN_KEEP_SAMPLES;
  const uint32_t data_bytes = static_cast<uint32_t>(this->captured_samples_) * 2;
  bool ok = this->write_wav_header_(data_bytes);
  ok = fflush(this->file_) == 0 && ok;
  ok = fsync(fileno(this->file_)) == 0 && ok;
  ok = fclose(this->file_) == 0 && ok;
  this->file_ = nullptr;
  if (!ok) {
    ESP_LOGE(TAG, "Could not finalize %s cleanly", this->temp_path_);
    return false;
  }
  if (!keep) {
    unlink(this->temp_path_);
    if (keep_file)
      ESP_LOGW(TAG, "Message was shorter than half a second; discarded");
    return true;
  }
  if (rename(this->temp_path_, this->final_path_) != 0) {
    ESP_LOGE(TAG, "Could not rename %s to %s", this->temp_path_, this->final_path_);
    return false;
  }
  const float seconds = static_cast<float>(this->captured_samples_) / SAMPLE_RATE;
  const float rms = std::sqrt(static_cast<float>(this->raw_sum_squares_) /
                              std::max<size_t>(this->captured_samples_, 1));
  ESP_LOGI(TAG, "SAVED %s: %.1f s, raw peak %" PRIi32 ", RMS %.1f, filtered peak %" PRIi32,
           this->final_path_, seconds, this->raw_peak_, rms, this->filtered_peak_);
  this->saved_message_count_++;
  this->next_message_index_++;
  return true;
}

bool WM8960AudioTest::initialize_i2s_() {
  i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  channel_config.dma_desc_num = 8;
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
      {0x01, 0x137, 0},   // Right handset input PGA +24 dB; update both.
      {0x20, 0x138, 0},   // Left input boost path.
      {0x21, 0x138, 0},   // Right handset input boost +29 dB.
      {0x22, 0x150, 0},   // Left DAC to output mixer.
      {0x25, 0x150, 0},   // Right DAC to output mixer.
      {0x02, 0x073, 0},   // Left headphone PGA -6 dB.
      {0x03, 0x173, 0},   // Right headphone PGA -6 dB; update both.
      {0x28, 0x073, 0},   // Left speaker PGA -6 dB.
      {0x29, 0x173, 0},   // Right speaker PGA -6 dB; update both.
      {0x2F, 0x03C, 0},   // Input PGAs and output mixers on.
      {0x19, 0x0FE, 0},   // VMID, VREF, MICBIAS, inputs and ADCs on.
  };

  for (const auto &entry : INITIALIZATION) {
    if (!this->write_register_(entry.reg, entry.value)) {
      ESP_LOGE(TAG, "WM8960 register 0x%02X write failed", entry.reg);
      return false;
    }
    if (entry.delay_ms != 0)
      delay(entry.delay_ms);
  }
  if (!this->write_register_(0x1A, this->headphone_output_ ? 0x1E1 : 0x199))
    return false;
  if (!this->write_register_(0x31, this->headphone_output_ ? 0x037 : 0x0F7))
    return false;
  if (!this->write_register_(0x05, 0x000))
    return false;
  return true;
}

bool WM8960AudioTest::write_register_(uint8_t reg, uint16_t value) {
  const uint8_t data[2] = {
      static_cast<uint8_t>((reg << 1U) | ((value >> 8U) & 0x01U)),
      static_cast<uint8_t>(value & 0xFFU),
  };
  const i2c::ErrorCode error = this->write(data, sizeof(data));
  if (error != i2c::ERROR_OK)
    ESP_LOGE(TAG, "I2C write to register 0x%02X failed with error code %u", reg, static_cast<unsigned>(error));
  return error == i2c::ERROR_OK;
}

bool WM8960AudioTest::play_tone_() {
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  constexpr size_t frames = SAMPLE_RATE * 4 / 10;
  constexpr size_t fade_frames = SAMPLE_RATE / 100;
  size_t completed = 0;

  // Let the newly enabled DAC and output path settle before generating the tone.
  for (size_t block = 0; block < 7; block++) {
    size_t written = 0;
    if (i2s_channel_write(this->tx_handle_, stereo.data(), sizeof(stereo), &written, 1000) != ESP_OK)
      return false;
  }

  while (completed < frames) {
    const size_t block_frames = std::min(FRAMES_PER_BLOCK, frames - completed);
    for (size_t index = 0; index < block_frames; index++) {
      const size_t position = completed + index;
      const float phase = 2.0f * PI * 1000.0f * static_cast<float>(position) / SAMPLE_RATE;
      const float fade_in = std::min(1.0f, static_cast<float>(position) / fade_frames);
      const float fade_out = std::min(1.0f, static_cast<float>(frames - position) / fade_frames);
      const int16_t sample = static_cast<int16_t>(std::sin(phase) * 8000.0f * std::min(fade_in, fade_out));
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

void WM8960AudioTest::shutdown_() {
  if (this->tx_handle_ != nullptr) {
    if (this->rx_handle_ != nullptr)
      i2s_channel_disable(this->rx_handle_);
    i2s_channel_disable(this->tx_handle_);
    if (this->rx_handle_ != nullptr)
      i2s_del_channel(this->rx_handle_);
    i2s_del_channel(this->tx_handle_);
    this->rx_handle_ = nullptr;
    this->tx_handle_ = nullptr;
  }

  // The bench HAT lacks strong I2C pull-ups, so stop the fast audio clocks before control writes.
  this->write_register_(0x05, 0x008);
  delay(35);
  this->write_register_(0x31, 0x037);
  this->write_register_(0x1A, 0x001);
  this->write_register_(0x2F, 0x000);
  this->write_register_(0x19, 0x0C0);
  this->write_register_(0x04, 0x0DC);
  this->write_register_(0x1A, 0x000);
  this->write_register_(0x1C, 0x09C);
  this->write_register_(0x19, 0x000);
}

void WM8960AudioTest::reset_cycle_metrics_() {
  this->captured_samples_ = 0;
  this->blocks_since_sync_ = 0;
  this->raw_peak_ = 0;
  this->filtered_peak_ = 0;
  this->raw_sum_squares_ = 0;
  this->high_pass_initialized_ = false;
  this->high_pass_previous_input_ = 0.0f;
  this->high_pass_previous_output_ = 0.0f;
}

void WM8960AudioTest::dump_config() {
  ESP_LOGCONFIG(TAG, "WM8960 guestbook recorder:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Sample rate: %" PRIu32 " Hz", SAMPLE_RATE);
  ESP_LOGCONFIG(TAG, "  BCLK: GPIO%u", this->bclk_pin_);
  ESP_LOGCONFIG(TAG, "  LRCLK: GPIO%u", this->lrclk_pin_);
  ESP_LOGCONFIG(TAG, "  ADC data: GPIO%u", this->adc_pin_);
  ESP_LOGCONFIG(TAG, "  DAC data: GPIO%u", this->dac_pin_);
  ESP_LOGCONFIG(TAG, "  Output: %s", this->headphone_output_ ? "headphone jack" : "class-D speakers");
  ESP_LOGCONFIG(TAG, "  Lift-to-beep delay: %" PRIu32 " ms", LIFT_TO_BEEP_DELAY_MS);
  ESP_LOGCONFIG(TAG, "  Messages saved since boot: %" PRIu32, this->saved_message_count_);
}

}  // namespace esphome::wm8960_audio_test
