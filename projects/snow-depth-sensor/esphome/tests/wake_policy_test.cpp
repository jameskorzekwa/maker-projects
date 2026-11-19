#include "../wake_policy.h"

#include <cassert>
#include <iostream>
#include <limits>

using snow::WakePolicy;

static WakePolicy measured(uint32_t base = 0) {
  WakePolicy p;
  p.start(base);
  p.battery_published(base + 1250, true);
  p.distance_published(base + 3800);
  return p;
}

int main() {
  // Uninitialized automation cannot sleep or accept pre-power UART bytes.
  WakePolicy idle;
  assert(!idle.should_sleep(60000, false));
  assert(!idle.accept_distance(60000, 1000));

  WakePolicy p;
  p.start(100);
  assert(!p.accept_distance(3099, 1000));
  assert(p.accept_distance(3100, 1000));
  for (int invalid : {0, 299, 300, 5000, 9999})
    assert(!p.accept_distance(3100, invalid));
  assert(p.accept_distance(3100, 301));
  assert(p.accept_distance(3100, 4999));
  p.distance_published(3800);
  assert(!p.accept_distance(3900, 1000));  // Completed sensor batch stays off.

  // Fast connection: both filtered batches AND transmit grace are required.
  p = measured();
  p.observe_connection(200, true);
  assert(!p.should_sleep(5199, false));  // connected grace
  assert(!p.should_sleep(5200, false));  // telemetry grace not yet complete
  assert(p.should_sleep(5300, false));

  // API connection alone and raw/invalid ADC results never suffice.
  p.start(0);
  p.observe_connection(200, true);
  p.battery_published(1250, false);
  p.distance_published(3800);
  assert(!p.battery_ready());
  assert(!p.should_sleep(10000, false));
  p.battery_published(10000, true);
  assert(!p.should_sleep(11499, false));
  assert(p.should_sleep(11500, false));
  p.battery_published(11500, true);  // Repeated callback cannot extend grace.
  assert(p.should_sleep(11500, false));
  p.start(0);
  p.observe_connection(200, true);
  p.battery_published(1250, true);
  assert(!p.should_sleep(10000, false));  // missing distance
  assert(!p.should_sleep(29999, false));
  assert(p.should_sleep(30000, false));

  // Offline deadline even without either measurement; late API can't extend it.
  p.start(0);
  assert(!p.should_sleep(29999, false));
  assert(p.should_sleep(30000, false));
  p.observe_connection(29900, true);
  assert(!p.should_sleep(29999, false));
  assert(p.should_sleep(30000, false));

  // Reconnect restarts subscription grace. Disconnection blocks early sleep.
  p = measured();
  p.observe_connection(200, true);
  p.observe_connection(4000, false);
  assert(!p.should_sleep(6000, false));
  p.observe_connection(6000, true);
  assert(!p.should_sleep(10999, false));
  assert(p.should_sleep(11000, false));

  // Persisted maintenance and in-progress OTA override even the hard deadline.
  p = measured();
  p.observe_connection(200, true);
  assert(!p.should_sleep(60000, true));
  p.ota_begin();
  assert(!p.should_sleep(60000, false));
  p.ota_finished(60000);  // Same recovery on success/error; permit reboot/retry.
  assert(!p.should_sleep(64999, false));
  assert(p.should_sleep(65000, false));
  assert(!p.should_sleep(65000, true));
  p.ota_begin();
  assert(!p.should_sleep(90000, false));

  // Unsigned elapsed arithmetic survives millis() rollover during maintenance.
  const uint32_t base = std::numeric_limits<uint32_t>::max() - 2000;
  p = measured(base);
  p.observe_connection(base + 200, true);
  assert(!p.should_sleep(base + 5299, false));
  assert(p.should_sleep(base + 5300, false));
  p.start(base);
  assert(p.accept_distance(base + 3000, 1000));
  assert(!p.should_sleep(base + 29999, false));
  assert(p.should_sleep(base + 30000, false));

  std::cout << "wake policy regressions passed\n";
}
