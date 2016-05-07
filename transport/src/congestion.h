/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once
#include "packet.h"
#include "base/core/macro.h"
#include "base/math/math.h"

namespace Rush {

class CongestionInfo;

/// Congestion control algorithm which responsibility is to modify
/// m_current_interval value based on connection status.
class ICongestionControl {
public:
  ICongestionControl(const CongestionInfo &info, rush_time_t *interval)
      : m_info(info), m_current_interval(interval) {}
  virtual ~ICongestionControl() {}

  virtual void TickSend(rush_time_t now, rush_sequence_t id) = 0;
  virtual void TickDelivered(rush_time_t now, rush_sequence_t id,
                             rush_time_t rtt) = 0;
  virtual void TickLost(rush_time_t now, rush_sequence_t id) = 0;

protected:
  const CongestionInfo &m_info;
  rush_time_t *m_current_interval;
};

class CongestionInfo {
private:
  // max number of tracked packets that were lost.
  static const u16 kLostPacketStoreSize = 1000;

  ICongestionControl *m_control;
  u16 m_mtu;
  rush_time_t m_min_interval;
  rush_time_t m_max_interval;
  rush_time_t m_current_interval;
  rush_time_t m_last_send;
  rush_time_t m_last_recv;
  rush_time_t m_last_delivered;
  rush_time_t m_packet_lost_time[kLostPacketStoreSize];
  u16 m_current_lost_packet;

public:
  CongestionInfo(u16 _mtu, rush_time_t _min_interval, rush_time_t _max_interval,
                 rush_time_t _cur_interval, rush_time_t now);
  ~CongestionInfo();

  void TickSend(rush_time_t now, rush_sequence_t id);
  void TickRecv(rush_time_t now);
  void TickDelivered(rush_time_t now, rush_sequence_t id, rush_time_t rtt);
  void TickLost(rush_time_t now, rush_sequence_t id);

  bool ShouldSendPacket(rush_time_t now);
  rush_time_t SendInterval() const;
  rush_time_t GetMinInterval() const { return m_min_interval; }
  rush_time_t GetMaxInterval() const { return m_max_interval; }
  u16 GetMtu() const { return m_mtu; }

  rush_time_t TimeSinceLastSend(rush_time_t now) const;
  rush_time_t TimeSinceLastRecv(rush_time_t now) const;

  u16 NumLostPackets(rush_time_t from, rush_time_t to);
};

} // namespace Rush
