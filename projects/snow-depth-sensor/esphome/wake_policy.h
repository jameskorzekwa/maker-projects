#pragma once

#include <cstdint>

namespace snow {

// Pure timing/state policy shared by production automation and host tests.
// API state subscription + grace is an opportunity to transmit, NOT a delivery ACK.
class WakePolicy {
 public:
  static constexpr uint32_t SETTLE_MS = 3000;
  static constexpr uint32_t CONNECTED_GRACE_MS = 5000;
  static constexpr uint32_t TELEMETRY_GRACE_MS = 1500;
  static constexpr uint32_t MAX_WAKE_MS = 30000;

  void start(uint32_t now) {
    *this = WakePolicy{};
    started_ = true;
    started_at_ = now;
  }
  bool accept_distance(uint32_t now, int mm) const {
    return started_ && !distance_ready_ &&
           elapsed(now, started_at_) >= SETTLE_MS && mm > 300 && mm < 5000;
  }
  void distance_published(uint32_t now) {
    distance_ready_ = true;
    last_telemetry_at_ = now;
  }
  void battery_published(uint32_t now, bool valid) {
    if (!valid || battery_ready_) return;
    battery_ready_ = true;
    last_telemetry_at_ = now;
  }
  bool battery_ready() const { return battery_ready_; }
  void observe_connection(uint32_t now, bool connected) {
    if (connected && !connected_) connected_at_ = now;
    connected_ = connected;
  }
  void ota_begin() { uploading_ = true; }
  void ota_finished(uint32_t now) {
    uploading_ = false;
    recovery_ = true;
    recovery_at_ = now;
  }
  bool should_sleep(uint32_t now, bool maintenance) const {
    if (!started_ || maintenance || uploading_) return false;
    if (recovery_ && elapsed(now, recovery_at_) < CONNECTED_GRACE_MS) return false;
    // The hard deadline wins over connected grace on a late connection.
    if (elapsed(now, started_at_) >= MAX_WAKE_MS) return true;
    if (connected_ && elapsed(now, connected_at_) < CONNECTED_GRACE_MS) return false;
    return connected_ && distance_ready_ && battery_ready_ &&
           elapsed(now, last_telemetry_at_) >= TELEMETRY_GRACE_MS;
  }

 private:
  static uint32_t elapsed(uint32_t now, uint32_t then) { return now - then; }
  bool started_{false};
  bool distance_ready_{false};
  bool battery_ready_{false};
  bool connected_{false};
  bool uploading_{false};
  bool recovery_{false};
  uint32_t started_at_{0};
  uint32_t connected_at_{0};
  uint32_t last_telemetry_at_{0};
  uint32_t recovery_at_{0};
};

// ESPHome emits global component declarations before custom includes. Keep the
// instance in this header and reference it only from generated lambda bodies.
inline WakePolicy &policy() {
  static WakePolicy instance;
  return instance;
}
}  // namespace snow
