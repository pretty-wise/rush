/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/macro.h"
#include "rush/rush.h"
#include "base/io/base_file.h"

#include "packet.h"
#include "endpoint.h"

namespace Rush {

struct ThroughputRegulation;

class TransmissionLog {
public:
  TransmissionLog(u16 mtu);
  ~TransmissionLog();

  bool Open(const char *filepath);

  bool LogUpstream(EndpointId id, rush_sequence_t seq, rush_time_t time,
                   const void *data, u16 data_size, u32 send_interval, u32 rtt);
  bool LogDownstream(EndpointId id, rush_sequence_t seq, rush_time_t time,
                     const void *data, u16 data_size);

private:
  bool Log(int type, EndpointId id, rush_sequence_t seq, rush_time_t time,
           const void *data, u16 nbytes, u32 send_interval, u32 rtt);

  Base::FileHandle m_handle;
  const u16 m_mtu;
  void *m_write_buffer;
};

} // namespace Rush
