/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "istream.h"

namespace Rush {
namespace Stream {

class DataStream : public IStream {
private:
public:
  typedef void (*DataReceived)(const void *data, size_t nbytes, void *udata);

  bool Send(rush_sequence_t const void *data, size_t nbytes);

public: // IStream
  size_t Pack(rush_sequence_t packet_id, void *data, size_t nbytes);
  void Unpack(const void *data, size_t nbytes);
  void Delivered(rush_sequence_t packet_id);
  void Lost(rush_sequence_t packet_id);
};

} // namespace Stream
} // namespace Rush
