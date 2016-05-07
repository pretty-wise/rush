/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "rtt.h"
#include "base/core/assert.h"

namespace Rush {

RttInfo::RttInfo(rush_time_t desired_rtt) : m_average(desired_rtt) {
  m_current = 0;
  for(u32 i = 0; i < kSampleCount; ++i) {
    m_id[i] = -1;
  }
}

rush_time_t RttInfo::GetAverage() const { return m_average; }

void RttInfo::PacketSent(rush_sequence_t id, rush_time_t now) {
  m_id[m_current] = id;
  m_send_time[m_current] = now;
  m_current = (m_current + 1) % kSampleCount;
}

rush_time_t RttInfo::PacketReceived(rush_sequence_t id, rush_time_t now) {
  rush_time_t rtt = 0;

  bool found_rtt = false;
  {
    for(u32 i = 0; i < kSampleCount; ++i) {
      if(m_id[i] == id) {
        rtt = now - m_send_time[i];
        found_rtt = true;
        break;
      }
    }
  }
  if(found_rtt) {
    BASE_ASSERT(rtt >= 0.f, "rtt value for %d at %f is %f", id, now, rtt);
    m_average += (rtt - m_average) * 0.05f; // kSmoothFactor;
  }
  return rtt;
}
} // namespace Rush
