/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "transmission_log.h"
#include "bandwidth_limiter.h"

namespace Rush {

static const u16 kExtra = 128;

TransmissionLog::TransmissionLog(u16 mtu)
    : m_handle(Base::kInvalidHandle), m_mtu(mtu) {
  m_write_buffer = malloc(mtu + kExtra);
}

TransmissionLog::~TransmissionLog() {
  if(m_handle != Base::kInvalidHandle) {
    Base::Close(m_handle);
    m_handle = Base::kInvalidHandle;
  }

  free(m_write_buffer);
  m_write_buffer = nullptr;
}

bool TransmissionLog::Open(const char *filepath) {
  if(m_handle != Base::kInvalidHandle) {
    Base::Close(m_handle);
    m_handle = Base::kInvalidHandle;
  }
  if(!filepath) {
    return true;
  }

  m_handle =
      Base::Open(filepath, Base::OM_Create | Base::OM_Write | Base::OM_Trunc);
  return m_handle != Base::kInvalidHandle;
}

static const int kUpstream = 0;
static const int kDownstream = 1;

struct EntryHeader {
  int type; //
  EndpointId id;
  rush_sequence_t seq;
  rush_time_t time;
  u32 send_interval;
  u32 rtt;
  u16 length;
};

bool TransmissionLog::LogUpstream(EndpointId id, rush_sequence_t seq,
                                  rush_time_t time, const void *data,
                                  u16 data_size, u32 send_interval, u32 rtt) {
  return Log(kUpstream, id, seq, time, data, data_size, send_interval, rtt);
}

bool TransmissionLog::LogDownstream(EndpointId id, rush_sequence_t seq,
                                    rush_time_t time, const void *data,
                                    u16 data_size) {
  return Log(kDownstream, id, seq, time, data, data_size, 0, 0);
}

bool TransmissionLog::Log(int type, EndpointId id, rush_sequence_t seq,
                          rush_time_t time, const void *data, u16 nbytes,
                          u32 send_interval, u32 rtt) {
  if(m_handle == Base::kInvalidHandle) {
    return true;
  }

  if(nbytes + sizeof(EntryHeader) > m_mtu + kExtra) {
    return false;
  }

  EntryHeader header = {type, id, seq, time, send_interval, rtt, nbytes};
  memcpy(m_write_buffer, reinterpret_cast<void *>(&header), sizeof(header));
  memcpy((u8 *)m_write_buffer + sizeof(header), data, nbytes);

  size_t to_write = sizeof(header) + nbytes;
  size_t written = Base::Write(m_handle, m_write_buffer, to_write);
  return written == to_write;
}

} // namespace Rush
