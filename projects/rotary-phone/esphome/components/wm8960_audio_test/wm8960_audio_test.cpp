#include "wm8960_audio_test.h"

#include "esphome/components/network/util.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "driver/gpio.h"
#include "esp_wifi.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace esphome::wm8960_audio_test {

static const char *const TAG = "wm8960_audio_test";
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t FRAMES_PER_BLOCK = 256;
static constexpr size_t MAX_RECORD_SAMPLES = SAMPLE_RATE * 300;  // Five-minute message limit.
static constexpr size_t MIN_KEEP_SAMPLES = SAMPLE_RATE / 2;      // Discard sub-half-second messages.
static constexpr size_t SAMPLES_PER_SYNC = SAMPLE_RATE * 8;      // fsync stalls the card; keep them rare.
// 128 KB: four seconds of audio between microphone and card. Must stay a power of two because
// the ring indexes through RING_MASK. If this allocation ever fails, setup() says so and the
// component marks itself failed rather than recording garbage.
static constexpr size_t RING_CAPACITY = 65536;
static constexpr size_t RING_MASK = RING_CAPACITY - 1;
// One write per ~3.1 s instead of ~1.5 s. Each burst costs a fixed ~39 ms of card latency plus
// transfer time, so halving the write count removes half the disturbances outright and leaves
// a full second of ring headroom to keep capturing while the card is busy.
static constexpr size_t WRITE_CHUNK_SAMPLES = 49152;
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
  this->ring_ = static_cast<int16_t *>(malloc(RING_CAPACITY * sizeof(int16_t)));
  if (this->ring_ == nullptr) {
    ESP_LOGE(TAG, "Could not allocate the recording ring buffer");
    this->mark_failed();
    return;
  }
  ESP_LOGI(TAG, "Guestbook recorder armed; lift the handset to record a message");
}

std::string WM8960AudioTest::get_last_message_url() const {
  if (this->last_saved_file_.empty())
    return {};
  return "http://" + network::get_ip_addresses()[0].str() + "/messages/" + this->last_saved_file_;
}

bool WM8960AudioTest::http_busy_() const {
  return this->state_ == RecorderState::WAIT_BEFORE_BEEP || this->state_ == RecorderState::WAITING_TO_RECORD ||
         this->state_ == RecorderState::RECORDING;
}

void WM8960AudioTest::start_http_server_() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.stack_size = 8192;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&this->http_server_, &config) != ESP_OK) {
    ESP_LOGW(TAG, "Could not start the message download server");
    this->http_server_ = nullptr;
    return;
  }
  httpd_uri_t list_uri{};
  list_uri.uri = "/messages";
  list_uri.method = HTTP_GET;
  list_uri.handler = WM8960AudioTest::handle_message_list_;
  list_uri.user_ctx = this;
  httpd_register_uri_handler(this->http_server_, &list_uri);
  httpd_uri_t file_uri{};
  file_uri.uri = "/messages/*";
  file_uri.method = HTTP_GET;
  file_uri.handler = WM8960AudioTest::handle_message_file_;
  file_uri.user_ctx = this;
  httpd_register_uri_handler(this->http_server_, &file_uri);
  httpd_uri_t prompt_uri{};
  prompt_uri.uri = "/prompt";
  prompt_uri.method = HTTP_PUT;
  prompt_uri.handler = WM8960AudioTest::handle_prompt_upload_;
  prompt_uri.user_ctx = this;
  httpd_register_uri_handler(this->http_server_, &prompt_uri);
  httpd_uri_t index_uri{};
  index_uri.uri = "/";
  index_uri.method = HTTP_GET;
  index_uri.handler = WM8960AudioTest::handle_index_;
  index_uri.user_ctx = this;
  httpd_register_uri_handler(this->http_server_, &index_uri);
}

esp_err_t WM8960AudioTest::handle_index_(httpd_req_t *req) {
  auto *self = static_cast<WM8960AudioTest *>(req->user_ctx);
  httpd_resp_set_type(req, "text/html");
  std::string html =
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1'>"
      "<title>Rotary Phone Guestbook</title><style>"
      "body{font-family:sans-serif;max-width:640px;margin:2em auto;padding:0 1em;background:#faf7f2}"
      "h1{font-size:1.4em}li{margin:1.2em 0;list-style:none;background:#fff;border-radius:8px;"
      "padding:1em;box-shadow:0 1px 3px rgba(0,0,0,.15)}audio{width:100%;margin-top:.5em}"
      "small{color:#666}</style></head><body><h1>&#9742; Rotary Phone Guestbook</h1>";
  if (self->http_busy_()) {
    html += "<p>A message is being recorded right now; refresh in a moment.</p></body></html>";
    httpd_resp_send(req, html.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }
  std::vector<std::string> names;
  DIR *dir = opendir("/sdcard");
  if (dir != nullptr) {
    for (struct dirent *entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
      unsigned index = 0;
      int consumed = 0;
      sscanf(entry->d_name, "MSG%5u.WAV%n", &index, &consumed);
      if (consumed == 12)
        names.emplace_back(entry->d_name);
    }
    closedir(dir);
  }
  std::sort(names.rbegin(), names.rend());  // Newest first.
  char count_line[64];
  snprintf(count_line, sizeof(count_line), "<p><small>%u recorded message%s</small></p><ul>",
           static_cast<unsigned>(names.size()), names.size() == 1 ? "" : "s");
  html += count_line;
  for (const auto &name : names) {
    char path[48];
    snprintf(path, sizeof(path), "/sdcard/%s", name.c_str());
    struct stat file_stat {};
    const long seconds =
        stat(path, &file_stat) == 0 ? (file_stat.st_size - WAV_HEADER_BYTES) / (SAMPLE_RATE * 2) : 0;
    char item[256];
    snprintf(item, sizeof(item),
             "<li><b>%s</b> <small>%ld s</small>"
             "<audio controls preload='none' src='/messages/%s'></audio></li>",
             name.c_str(), seconds, name.c_str());
    html += item;
  }
  html += "</ul></body></html>";
  httpd_resp_send(req, html.c_str(), HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t WM8960AudioTest::handle_prompt_upload_(httpd_req_t *req) {
  auto *self = static_cast<WM8960AudioTest *>(req->user_ctx);
  if (self->http_busy_()) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_send(req, "recording in progress", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }
  if (req->content_len <= WAV_HEADER_BYTES || req->content_len > 4 * 1024 * 1024) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid prompt size");
    return ESP_OK;
  }
  FILE *file = fopen("/sdcard/PROMPT.TMP", "wb");
  if (file == nullptr) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "could not open file");
    return ESP_OK;
  }
  char buffer[4096];
  size_t remaining = req->content_len;
  while (remaining > 0) {
    const int received = httpd_req_recv(req, buffer, std::min(remaining, sizeof(buffer)));
    if (received <= 0 || fwrite(buffer, 1, received, file) != static_cast<size_t>(received)) {
      fclose(file);
      unlink("/sdcard/PROMPT.TMP");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "upload failed");
      return ESP_OK;
    }
    remaining -= received;
  }
  bool ok = fflush(file) == 0;
  ok = fsync(fileno(file)) == 0 && ok;
  ok = fclose(file) == 0 && ok;
  unlink("/sdcard/PROMPT.WAV");
  if (!ok || rename("/sdcard/PROMPT.TMP", "/sdcard/PROMPT.WAV") != 0) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "finalize failed");
    return ESP_OK;
  }
  ESP_LOGI(TAG, "Received a new greeting of %u bytes", static_cast<unsigned>(req->content_len));
  httpd_resp_send(req, "ok", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t WM8960AudioTest::handle_message_list_(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  std::string json = "[";
  DIR *dir = opendir("/sdcard");
  if (dir != nullptr) {
    for (struct dirent *entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
      unsigned index = 0;
      int consumed = 0;
      sscanf(entry->d_name, "MSG%5u.WAV%n", &index, &consumed);
      if (consumed != 12)
        continue;
      char path[48];
      snprintf(path, sizeof(path), "/sdcard/%s", entry->d_name);
      struct stat file_stat {};
      const long size = stat(path, &file_stat) == 0 ? file_stat.st_size : 0;
      char item[96];
      snprintf(item, sizeof(item), "%s{\"name\":\"%s\",\"size\":%ld}", json.size() > 1 ? "," : "",
               entry->d_name, size);
      json += item;
    }
    closedir(dir);
  }
  json += "]";
  httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t WM8960AudioTest::handle_message_file_(httpd_req_t *req) {
  auto *self = static_cast<WM8960AudioTest *>(req->user_ctx);
  if (self->http_busy_()) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_send(req, "recording in progress", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  }
  const char *name = strrchr(req->uri, '/');
  name = name == nullptr ? req->uri : name + 1;
  unsigned index = 0;
  int consumed = 0;
  sscanf(name, "MSG%5u.WAV%n", &index, &consumed);
  if (consumed != 12 || strlen(name) != 12) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no such message");
    return ESP_OK;
  }
  char path[48];
  snprintf(path, sizeof(path), "/sdcard/%s", name);
  FILE *file = fopen(path, "rb");
  if (file == nullptr) {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no such message");
    return ESP_OK;
  }
  httpd_resp_set_type(req, "audio/wav");
  char buffer[4096];
  size_t bytes_read = 0;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
      fclose(file);
      return ESP_FAIL;
    }
  }
  fclose(file);
  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
}

void WM8960AudioTest::loop() {
  // The listen socket needs the network stack, which starts after this component's setup.
  if (this->http_server_ == nullptr && network::is_connected() &&
      millis() - this->http_last_attempt_ >= 5000) {
    this->http_last_attempt_ = millis();
    this->start_http_server_();
  }
  this->update_hook_();
  switch (this->state_) {
    case RecorderState::IDLE:
    case RecorderState::WAITING_FOR_HANGUP:
      break;
    case RecorderState::WAIT_BEFORE_BEEP:
      if (millis() - this->state_started_at_ >= LIFT_TO_BEEP_DELAY_MS)
        this->begin_recorder_();
      break;
    case RecorderState::PLAYING_PROMPT:
      // Feed a few blocks per loop pass so the greeting cannot starve the task watchdog.
      for (int block = 0; block < 3 && this->state_ == RecorderState::PLAYING_PROMPT; block++) {
        if (!this->play_prompt_block_()) {
          this->finish_prompt_and_beep_();
          break;
        }
      }
      break;
    case RecorderState::WAITING_TO_RECORD:
      if (millis() - this->state_started_at_ >= this->record_start_delay_ms_) {
        std::array<int16_t, FRAMES_PER_BLOCK * 2> discarded_audio{};
        for (size_t block = 0; block < 32; block++) {
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
  // Wi-Fi transmit bursts spike the supply and couple into the microphone line;
  // minimum transmit power shrinks them roughly tenfold while staying connected.
  esp_wifi_set_max_tx_power(8);
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
    case RecorderState::PLAYING_PROMPT:
      if (this->prompt_file_ != nullptr) {
        fclose(this->prompt_file_);
        this->prompt_file_ = nullptr;
      }
      this->finalize_recording_(false);
      this->shutdown_();
      ESP_LOGI(TAG, "Handset replaced during the greeting; nothing saved");
      break;
    case RecorderState::WAITING_TO_RECORD:
      this->finalize_recording_(false);
      this->shutdown_();
      ESP_LOGI(TAG, "Handset replaced before recording started; nothing saved");
      break;
    case RecorderState::WAIT_BEFORE_BEEP:
      ESP_LOGI(TAG, "Handset replaced before the greeting; nothing saved");
      break;
    case RecorderState::WAITING_FOR_HANGUP:
      break;
  }
  this->state_ = RecorderState::IDLE;
  esp_wifi_set_max_tx_power(80);  // Restore full transmit power between messages.
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
  this->prompt_file_ = fopen("/sdcard/PROMPT.WAV", "rb");
  if (this->prompt_file_ != nullptr && fseek(this->prompt_file_, WAV_HEADER_BYTES, SEEK_SET) == 0) {
    ESP_LOGI(TAG, "Playing the greeting");
    this->state_ = RecorderState::PLAYING_PROMPT;
    return;
  }
  if (this->prompt_file_ != nullptr) {
    fclose(this->prompt_file_);
    this->prompt_file_ = nullptr;
  }
  this->finish_prompt_and_beep_();
}

bool WM8960AudioTest::play_prompt_block_() {
  if (this->prompt_file_ == nullptr)
    return false;
  std::array<int16_t, FRAMES_PER_BLOCK> mono{};
  const size_t frames = fread(mono.data(), sizeof(int16_t), FRAMES_PER_BLOCK, this->prompt_file_);
  if (frames == 0)
    return false;
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  for (size_t index = 0; index < frames; index++) {
    stereo[index * 2] = mono[index];
    stereo[index * 2 + 1] = mono[index];
  }
  size_t written = 0;
  if (i2s_channel_write(this->tx_handle_, stereo.data(), frames * 2 * sizeof(int16_t), &written, 1000) != ESP_OK)
    return false;
  return frames == FRAMES_PER_BLOCK;
}

void WM8960AudioTest::finish_prompt_and_beep_() {
  const bool prompt_played = this->prompt_file_ != nullptr;
  if (prompt_played) {
    fclose(this->prompt_file_);
    this->prompt_file_ = nullptr;
  }
  // The greeting file carries its own beep; the generated tone is only the no-greeting fallback.
  if (!prompt_played && !this->play_tone_()) {
    this->finalize_recording_(false);
    this->fail_cycle_("Could not play the beep");
    return;
  }
  // After a greeting, wait out the buffered playback tail so recording starts after the beep.
  this->record_start_delay_ms_ = prompt_played ? 500 : 250;
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

// Telephone-band Butterworth biquads at 16 kHz in Q13 fixed point: the ESP32-C6 has no FPU.
static constexpr int32_t FILTER_SHIFT = 13;
static constexpr int32_t HIGH_PASS_300HZ[5] = {7537, -15074, 7537, -15022, 6935};
static constexpr int32_t LOW_PASS_3800HZ[5] = {2214, 4428, 2214, -754, 1418};

// The tick survived the preamp, an isolated preamp supply, and a codec-analog ground reference.
// It is the card's 100-200 mA write surge coupling into the front end, which is a hardware
// problem software cannot remove. What software does know is exactly when each write happens,
// so mute that window instead of letting the tick through.
static constexpr bool DUCK_ENABLED = true;

// Margin covers surge leading and trailing the blocking call. DUCK_EDGE_MS is only consulted
// when DUCK_FULL_WINDOW is false, and is kept so the cheaper option can be re-measured if the
// coupling is ever fixed in hardware.
static constexpr uint32_t DUCK_MARGIN_MS = 10;
static constexpr uint32_t DUCK_EDGE_MS = 20;
// Edge-only muting was retried at correct gain, once the clipping that could have masked the
// verdict was gone, and the tick returned while the mutes stayed audible: worse on both counts.
// The disturbance genuinely spans the whole burst, so mute all of it. This is the software
// ceiling; the ~120 ms gap every 3 s it costs is the price until the coupling is fixed in
// hardware.
static constexpr bool DUCK_FULL_WINDOW = true;

// Gain 0..32 for one duck window with 2 ms linear fades at both edges.
static int32_t duck_window_gain(size_t sample, size_t from, size_t to) {
  if (to <= from || sample < from || sample >= to)
    return 32;
  const size_t into = sample - from;
  const size_t left = to - sample;
  int32_t gain = 0;
  if (into < 32)
    gain = 32 - static_cast<int32_t>(into);
  if (left <= 32)
    gain = std::max(gain, 32 - static_cast<int32_t>(left));
  return gain;
}

static int32_t run_biquad(const int32_t (&coeff)[5], int32_t (&state)[4], int32_t input) {
  const int64_t accumulator = static_cast<int64_t>(coeff[0]) * input + static_cast<int64_t>(coeff[1]) * state[0] +
                              static_cast<int64_t>(coeff[2]) * state[1] - static_cast<int64_t>(coeff[3]) * state[2] -
                              static_cast<int64_t>(coeff[4]) * state[3];
  const auto output = static_cast<int32_t>(accumulator >> FILTER_SHIFT);
  state[1] = state[0];
  state[0] = input;
  state[3] = state[2];
  state[2] = output;
  return output;
}

bool WM8960AudioTest::record_audio_block_() {
  // Drain every buffered microphone block first so a slow card write cannot drop audio.
  std::array<int16_t, FRAMES_PER_BLOCK * 2> stereo{};
  for (int burst = 0; burst < 32; burst++) {
    size_t bytes_read = 0;
    const esp_err_t error =
        i2s_channel_read(this->rx_handle_, stereo.data(), sizeof(stereo), &bytes_read, burst == 0 ? 50 : 0);
    if (error != ESP_OK && error != ESP_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "I2S microphone read failed: %s", esp_err_to_name(error));
      return false;
    }
    if (bytes_read == 0)
      break;
    // Partial reads carry real audio: discarding them cuts a click into the recording.
    const size_t frames_read = bytes_read / (2 * sizeof(int16_t));
    const size_t frames_to_keep = std::min(frames_read, MAX_RECORD_SAMPLES - this->captured_samples_);
    for (size_t index = 0; index < frames_to_keep; index++) {
      const int16_t slot_1 = stereo[index * 2 + 1];
      const int32_t band_limited = run_biquad(
          LOW_PASS_3800HZ, this->band_pass_state_[1],
          run_biquad(HIGH_PASS_300HZ, this->band_pass_state_[0], slot_1));
      // No digital makeup gain: the MAX4466 sets the level in analog, ahead of the ADC, so the
      // codec sees a real signal instead of quantization noise multiplied after the fact.
      int32_t amplified = band_limited;
      if (DUCK_ENABLED) {
        // Mute the window the last card write occupied, located by continuous sample index so
        // the audio drained after the write still lines up with when it happened.
        const size_t global_index = this->captured_samples_ + index;
        const int32_t duck_gain =
            std::min(duck_window_gain(global_index, this->duck_a_from_, this->duck_a_to_),
                     duck_window_gain(global_index, this->duck_b_from_, this->duck_b_to_));
        if (duck_gain < 32)
          amplified = amplified * duck_gain / 32;
      }
      const int16_t sample = static_cast<int16_t>(std::clamp<int32_t>(amplified, -32767, 32767));
      if (this->ring_count_ == RING_CAPACITY) {
        // Sacrifice the oldest audio rather than corrupting the stream; log once per message.
        this->ring_tail_ = (this->ring_tail_ + 1) & RING_MASK;
        this->ring_count_--;
        if (!this->ring_overflowed_) {
          this->ring_overflowed_ = true;
          ESP_LOGW(TAG, "Recording ring overflowed; the card is stalling badly");
        }
      }
      this->ring_[this->ring_head_] = sample;
      this->ring_head_ = (this->ring_head_ + 1) & RING_MASK;
      this->ring_count_++;
      this->raw_peak_ = std::max(this->raw_peak_, std::abs(static_cast<int32_t>(slot_1)));
      this->filtered_peak_ = std::max(this->filtered_peak_, std::abs(static_cast<int32_t>(sample)));
      this->raw_sum_squares_ += static_cast<int64_t>(slot_1) * slot_1;
    }
    this->captured_samples_ += frames_to_keep;
    if (bytes_read < sizeof(stereo))
      break;  // Everything currently buffered has been drained.
  }

  // One large chunk per pass: rare bursts, each with two matching micro-mutes in the audio.
  if (this->ring_count_ >= WRITE_CHUNK_SAMPLES) {
    const size_t base = this->captured_samples_;
    const uint32_t write_started_ms = millis();
    if (!this->write_ring_to_file_(WRITE_CHUNK_SAMPLES))
      return false;
    if (this->samples_since_sync_ >= SAMPLES_PER_SYNC) {
      this->samples_since_sync_ = 0;
      if (fflush(this->file_) != 0 || fsync(fileno(this->file_)) != 0) {
        ESP_LOGE(TAG, "microSD flush failed at sample %u", this->samples_written_to_file_);
        return false;
      }
    }
    const uint32_t write_ms = millis() - write_started_ms;
    const size_t span = static_cast<size_t>(write_ms + DUCK_MARGIN_MS) * (SAMPLE_RATE / 1000);
    const size_t edge = DUCK_EDGE_MS * (SAMPLE_RATE / 1000);
    if (DUCK_FULL_WINDOW) {
      this->duck_a_from_ = base;
      this->duck_a_to_ = base + span;
      this->duck_b_from_ = 0;  // One window covers everything; the second pair stays disarmed.
      this->duck_b_to_ = 0;
    } else {
      this->duck_a_from_ = base;                                 // Current steps up here.
      this->duck_a_to_ = base + std::min(edge, span);
      this->duck_b_to_ = base + span;                            // And back down here.
      this->duck_b_from_ = span > edge ? base + span - edge : base;
    }
    ESP_LOGD(TAG, "card write took %" PRIu32 " ms, muting %s of a %.0f ms window", write_ms,
             DUCK_FULL_WINDOW ? "all" : "2x20 ms", 1000.0f * span / SAMPLE_RATE);
  }
  return true;
}

bool WM8960AudioTest::write_ring_to_file_(size_t max_samples) {
  size_t to_write = std::min(this->ring_count_, max_samples);
  while (to_write > 0) {
    const size_t linear = std::min(to_write, RING_CAPACITY - this->ring_tail_);
    if (fwrite(&this->ring_[this->ring_tail_], sizeof(int16_t), linear, this->file_) != linear) {
      ESP_LOGE(TAG, "microSD write failed at sample %u", this->samples_written_to_file_);
      return false;
    }
    this->ring_tail_ = (this->ring_tail_ + linear) & RING_MASK;
    this->ring_count_ -= linear;
    this->samples_written_to_file_ += linear;
    this->samples_since_sync_ += linear;
    to_write -= linear;
  }
  return true;
}

bool WM8960AudioTest::finalize_recording_(bool keep_file) {
  if (this->file_ == nullptr)
    return true;
  bool ok = this->write_ring_to_file_(this->ring_count_);  // Drain everything still buffered.
  const bool keep = keep_file && this->samples_written_to_file_ >= MIN_KEEP_SAMPLES;
  const uint32_t data_bytes = static_cast<uint32_t>(this->samples_written_to_file_) * 2;
  ok = this->write_wav_header_(data_bytes) && ok;
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
  const float seconds = static_cast<float>(this->samples_written_to_file_) / SAMPLE_RATE;
  const float rms = std::sqrt(static_cast<float>(this->raw_sum_squares_) /
                              std::max<size_t>(this->captured_samples_, 1));
  ESP_LOGI(TAG, "SAVED %s: %.1f s, raw peak %" PRIi32 ", RMS %.1f, filtered peak %" PRIi32,
           this->final_path_, seconds, this->raw_peak_, rms, this->filtered_peak_);
  this->last_saved_file_ = this->final_path_ + 8;  // Strip the "/sdcard/" prefix.
  this->saved_message_count_++;
  this->next_message_index_++;
  return true;
}

bool WM8960AudioTest::initialize_i2s_() {
  i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  channel_config.dma_desc_num = 24;  // Roughly 384 ms of hardware buffering to ride out card stalls.
  channel_config.dma_frame_num = FRAMES_PER_BLOCK;
  // Without this, a transmit underflow endlessly replays the last buffered audio instead of going silent.
  channel_config.auto_clear = true;

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
      // INVOL counts 0.75 dB steps from -17.25 dB at 0, so 23 is 0 dB. Recordings railed at full
      // scale with RMS up to 25000, so take 15 dB out here (INVOL 3) rather than wait on the
      // preamp trimmer. The trimmer is still the better place to fix this: attenuating after a
      // too-hot preamp wastes the headroom instead of never overdriving the input at all.
      {0x01, 0x103, 0},   // Right input PGA -15 dB.
      {0x20, 0x138, 0},   // Left input boost path.
      {0x21, 0x108, 0},   // Right input boost 0 dB: no codec gain belongs ahead of a preamp.
      {0x22, 0x150, 0},   // Left DAC to output mixer.
      {0x25, 0x150, 0},   // Right DAC to output mixer.
      // Sidetone (0x2E right-boost bypass) stays OFF: at +50 dB front-end gain it feeds back
      // through the handset acoustically. Revisit after the MAX4466 lowers the codec gain.
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
  this->samples_written_to_file_ = 0;
  this->samples_since_sync_ = 0;
  this->ring_head_ = 0;
  this->ring_tail_ = 0;
  this->ring_count_ = 0;
  this->ring_overflowed_ = false;
  this->duck_a_from_ = 0;
  this->duck_a_to_ = 0;
  this->duck_b_from_ = 0;
  this->duck_b_to_ = 0;
  this->raw_peak_ = 0;
  this->filtered_peak_ = 0;
  this->raw_sum_squares_ = 0;
  memset(this->band_pass_state_, 0, sizeof(this->band_pass_state_));
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
