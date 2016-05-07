/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "rush/rush.h"

namespace Rush {

class RttInfo {
private:
  static const u32 kSampleCount = 100;

  rush_sequence_t m_id[kSampleCount];
  rush_time_t m_send_time[kSampleCount];
  u32 m_current;

  rush_time_t m_average;

public:
  RttInfo(rush_time_t desired_rtt);

  rush_time_t GetAverage() const;
  void PacketSent(rush_sequence_t id, rush_time_t now);
  rush_time_t PacketReceived(rush_sequence_t id, rush_time_t now);
};

} // namespace Rush
