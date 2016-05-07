/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "packet.h"

#include "base/core/macro.h"

namespace Rush {

struct BandwidthLimiter {
  float capacity;
  float available;
  float fill_rate; // tokens per millisecond
  rush_time_t timestamp;

  BandwidthLimiter();
  BandwidthLimiter(u32 bytes_per_second, rush_time_t now);

  void SetCapacity(u32 bytes_per_second, rush_time_t now);
  bool Consume(u32 bytes, rush_time_t now);
  float Recalculate(rush_time_t now) const;
};

} // namespace Rush
