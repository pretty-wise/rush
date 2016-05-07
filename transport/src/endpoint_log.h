/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/macro.h"
#include "base/math/math.h"
#include "rush/rush.h"

#include "packet.h"

namespace Rush {

class PacketHistory {
private:
  u16 mtu;
  u16 history_size;
  u16 current;
  rush_sequence_t *id;
  s8 *packet_data;
  u16 *packet_size;
  rush_time_t *packet_time;

public:
  PacketHistory(u16 _mtu, u16 size);
  ~PacketHistory();
  void Log(rush_sequence_t id, rush_time_t time, const void *data, u16 data_size);
  u32 GetDataReceived(rush_time_t *start, rush_time_t *end);
};

////////////////////////////////////////////////////////////////////////////////

inline PacketHistory::PacketHistory(u16 _mtu, u16 size)
    : mtu(_mtu), history_size(size), current(0) {
  id = new rush_sequence_t[size];
  packet_data = new s8[size * mtu];
  packet_size = new u16[size];
  packet_time = new rush_time_t[size];
  memset(id, 0, sizeof(rush_sequence_t) * size);
  memset(packet_data, 0, size * mtu);
  memset(packet_size, 0, sizeof(u16) * size);
  memset(packet_time, 0, sizeof(rush_time_t) * size);
}

inline PacketHistory::~PacketHistory() {
  delete[] id;
  delete[] packet_data;
  delete[] packet_size;
  delete[] packet_time;
}

inline void PacketHistory::Log(rush_sequence_t id, rush_time_t time, const void *data,
                               u16 data_size) {
  this->id[current] = id;
  this->packet_time[current] = time;
  memcpy(&this->packet_data[current * mtu], data, data_size);
  this->packet_size[current] = data_size;
  this->current = (this->current + 1) % this->history_size;
}

inline u32 PacketHistory::GetDataReceived(rush_time_t *start, rush_time_t *end) {
  rush_time_t lower_bound = packet_time[0];
  rush_time_t upper_bound = packet_time[0];
  u32 total = 0;
  for(u16 i = 0; i < history_size; ++i) {
    rush_time_t t = packet_time[i];
    if(t < lower_bound) {
      lower_bound = t;
    }
    if(t > upper_bound) {
      upper_bound = t;
    }
    if(t >= *start && t <= *end) {
      total += packet_size[i];
    }
  }
  if(lower_bound > *start) {
    *start = lower_bound;
  }
  if(upper_bound < *end) {
    *end = upper_bound;
  }
  return total;
}

/*
struct BandwidthStats {
  u32 total_bytes_sent;
  u32 total_bytes_received;
  rush_time_t connection_start;

  rush_time_t ConnectionTime(rush_time_t now) const { return now - connection_start; }
  u32 DownstreamPerSecond(rush_time_t now) const {
    return total_bytes_received / ConnectionTime(now);
  }
  u32 UpstreamPerSecond(rush_time_t now) const {
    return total_bytes_sent / ConnectionTime(now);
  }
};*/

} // namespace Rush
