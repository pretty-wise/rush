/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"

namespace Rush {
namespace Stream {

class IStream {
public:
  // Pack the data to a buffer of id packet_id.
  // @param packet_id Id of the packet sent to destintion.
  // @param data Buffer to pack the data to
  // @param nbytes Size of the data buffer.
  // @return Number of bytes packed.
  virtual size_t Pack(rush_sequence_t packet_id, void *data, size_t nbytes) = 0;

  // Unpack the data received.
  // @param data Data received
  // @param nbytes Number of bytes received.
  virtual void Unpack(const void *data, size_t nbytes) = 0;

  // Notification of packet_id being successfully delivered.
  // @param packet_id Id of the packet delivered.
  virtual void Delivered(rush_sequence_t packet_id) = 0;

  // Notification of packet_id being lost.
  // @param packet_id Id of the lost packet.
  virtual void Lost(rush_sequence_t packet_id) = 0;
};

} // namespace Stream
} // namespace Rush
