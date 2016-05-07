/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "congestion.h"
#include "congestion_buckets.h"

namespace Rush {

CongestionInfo::CongestionInfo(u16 _m_mtu, rush_time_t _m_min_interval,
                               rush_time_t _m_max_interval,
                               rush_time_t _cur_interval, rush_time_t now)
    : m_mtu(_m_mtu), m_min_interval(_m_min_interval),
      m_max_interval(_m_max_interval), m_current_interval(_cur_interval),
      m_last_send(now - m_current_interval), m_last_recv(now),
      m_last_delivered(now), m_current_lost_packet(0) {
  for(u16 i = 0; i < kLostPacketStoreSize; ++i) {
    m_packet_lost_time[i] = 0;
  }
  m_control = new BucketCongestion(*this, &m_current_interval, now);
}

CongestionInfo::~CongestionInfo() { delete m_control; }

void CongestionInfo::TickSend(rush_time_t now, rush_sequence_t id) {
  m_last_send = now;
  m_control->TickSend(now, id);
}

void CongestionInfo::TickRecv(rush_time_t now) { m_last_recv = now; }

void CongestionInfo::TickDelivered(rush_time_t now, rush_sequence_t id,
                                   rush_time_t rtt) {
  m_last_delivered = now;
  m_control->TickDelivered(now, id, rtt);
}
void CongestionInfo::TickLost(rush_time_t now, rush_sequence_t id) {
  m_packet_lost_time[m_current_lost_packet] = now;
  m_current_lost_packet = (m_current_lost_packet + 1) % kLostPacketStoreSize;
  m_control->TickLost(now, id);
}

bool CongestionInfo::ShouldSendPacket(rush_time_t now) {
  return TimeSinceLastSend(now) >= SendInterval();
}

rush_time_t CongestionInfo::SendInterval() const { return m_current_interval; }
rush_time_t CongestionInfo::TimeSinceLastSend(rush_time_t now) const {
  return now - m_last_send;
}
rush_time_t CongestionInfo::TimeSinceLastRecv(rush_time_t now) const {
  return now - m_last_recv;
}

u16 CongestionInfo::NumLostPackets(rush_time_t from, rush_time_t to) {
  u16 total = 0;
  for(u16 i = 0; i < kLostPacketStoreSize; ++i) {
    if(m_packet_lost_time[i] >= from && m_packet_lost_time[i] <= to &&
       m_packet_lost_time[i] != 0) {
      ++total;
    }
  }
  return total;
}

} // namespace Rush
