/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "bandwidth_limiter.h"

namespace Rush {

BandwidthLimiter::BandwidthLimiter() { available = 0; }

BandwidthLimiter::BandwidthLimiter(u32 bytes_per_second, rush_time_t now) {
  available = 0;
  SetCapacity(bytes_per_second, now);
}

void BandwidthLimiter::SetCapacity(u32 bytes_per_second, rush_time_t now) {
  capacity = (float)bytes_per_second;
  if(available > capacity) {
    available = capacity;
  }
  fill_rate = (float)bytes_per_second * 0.001f;
  timestamp = now;
}

bool BandwidthLimiter::Consume(u32 bytes, rush_time_t now) {
  available = Recalculate(now);
  timestamp = now;
  if(available <= bytes) {
    return false;
  }
  available -= bytes;
  return true;
}

float BandwidthLimiter::Recalculate(rush_time_t now) const {
  u32 new_size = available;
  if(available < capacity) {
    float delta = fill_rate * (now - timestamp);
    new_size = available + delta;
    if(new_size > capacity) {
      new_size = capacity;
    }
  }
  return new_size;
}

} // namespace Rush
