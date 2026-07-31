#include "sd_card_test.h"

#include "esphome/core/log.h"

#include "driver/spi_master.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <unistd.h>

namespace esphome::sd_card_test {

static const char *const TAG = "sd_card_test";
static constexpr char MOUNT_POINT[] = "/sdcard";
// Keep the 8.3 name format: ESP-IDF's default FATFS configuration disables long file names.
static constexpr char TEST_PATH[] = "/sdcard/SDTEST.TXT";
static constexpr char TEST_CONTENT[] = "rotary-phone sd card test\n";

void SDCardTest::setup() {
  ESP_LOGI(TAG, "Initializing microSD over SPI without formatting");
  this->status_ = "initializing";

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.max_freq_khz = 8000;  // Faster transfers shorten each write's interference window.
  const auto spi_host = static_cast<spi_host_device_t>(host.slot);

  spi_bus_config_t bus_config{};
  bus_config.mosi_io_num = this->mosi_pin_;
  bus_config.miso_io_num = this->miso_pin_;
  bus_config.sclk_io_num = this->clk_pin_;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = 4096;

  esp_err_t error = spi_bus_initialize(spi_host, &bus_config, SPI_DMA_CH_AUTO);
  if (error != ESP_OK) {
    this->status_ = "SPI bus initialization failed";
    this->last_error_ = error;
    ESP_LOGE(TAG, "Could not initialize the microSD SPI bus: %s", esp_err_to_name(error));
    this->mark_failed();
    return;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = static_cast<gpio_num_t>(this->cs_pin_);
  slot_config.host_id = spi_host;

  esp_vfs_fat_sdmmc_mount_config_t mount_config{};
  mount_config.format_if_mount_failed = false;
  mount_config.max_files = 5;
  mount_config.allocation_unit_size = 16 * 1024;

  sdmmc_card_t *card = nullptr;
  error = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
  if (error != ESP_OK) {
    this->status_ = "FAT filesystem mount failed";
    this->last_error_ = error;
    ESP_LOGE(TAG, "Could not mount the existing FAT filesystem: %s", esp_err_to_name(error));
    spi_bus_free(spi_host);
    this->mark_failed();
    return;
  }

  const uint64_t capacity_bytes = static_cast<uint64_t>(card->csd.capacity) * card->csd.sector_size;
  ESP_LOGI(TAG, "Mounted card %.5s with capacity %.1f GB at %s", card->cid.name,
           static_cast<double>(capacity_bytes) / 1000000000.0, MOUNT_POINT);

  FILE *file = fopen(TEST_PATH, "wb");
  if (file == nullptr) {
    this->status_ = "test file open for writing failed";
    ESP_LOGE(TAG, "Could not open %s for writing", TEST_PATH);
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(spi_host);
    this->mark_failed();
    return;
  }

  const size_t expected_size = sizeof(TEST_CONTENT) - 1;
  bool write_ok = fwrite(TEST_CONTENT, 1, expected_size, file) == expected_size;
  write_ok = write_ok && fflush(file) == 0;
  write_ok = write_ok && fsync(fileno(file)) == 0;
  write_ok = fclose(file) == 0 && write_ok;
  if (!write_ok) {
    this->status_ = "test file write or fsync failed";
    ESP_LOGE(TAG, "Failed while writing or flushing %s", TEST_PATH);
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(spi_host);
    this->mark_failed();
    return;
  }

  file = fopen(TEST_PATH, "rb");
  if (file == nullptr) {
    this->status_ = "test file reopen failed";
    ESP_LOGE(TAG, "Could not reopen %s for verification", TEST_PATH);
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(spi_host);
    this->mark_failed();
    return;
  }

  char readback[sizeof(TEST_CONTENT)]{};
  const size_t bytes_read = fread(readback, 1, expected_size, file);
  const bool has_extra_data = fgetc(file) != EOF;
  const bool close_ok = fclose(file) == 0;
  if (bytes_read != expected_size || has_extra_data || !close_ok ||
      memcmp(readback, TEST_CONTENT, expected_size) != 0) {
    this->status_ = "test file readback mismatch";
    ESP_LOGE(TAG, "Readback verification failed for %s", TEST_PATH);
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(spi_host);
    this->mark_failed();
    return;
  }

  this->mounted_ = true;
  this->status_ = "PASS";
  ESP_LOGI(TAG, "MICROSD TEST PASS: write, fsync, and byte-for-byte readback succeeded");
}

void SDCardTest::dump_config() {
  ESP_LOGCONFIG(TAG, "microSD SPI test:");
  ESP_LOGCONFIG(TAG, "  CLK: GPIO%u", this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO: GPIO%u", this->miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI: GPIO%u", this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  CS: GPIO%u", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(this->mounted_));
  ESP_LOGCONFIG(TAG, "  Status: %s", this->status_);
  if (this->last_error_ != ESP_OK)
    ESP_LOGCONFIG(TAG, "  ESP-IDF error: %s (0x%03" PRIX32 ")", esp_err_to_name(this->last_error_),
                  static_cast<uint32_t>(this->last_error_));
}

}  // namespace esphome::sd_card_test
