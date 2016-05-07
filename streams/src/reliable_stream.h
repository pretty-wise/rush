/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "istream.h"

namespace Rush {
namespace Stream {

class ReliableStream : public IStream {
private:
public:
  enum Type {
    kUnreliable, // no delivery guarantee
    kReliable,   // delivery guaranteed
    kOrdered     // delivery and ordering guaranteed
  };
  typedef void (*DataReceived)(const void *data, size_t nbytes, void *udata);

  bool Send(const void *data, size_t nbytes, Type type);

public: // IStream
  size_t Pack(rush_sequence_t packet_id, void *data, size_t nbytes);
  void Unpack(const void *data, size_t nbytes);
  void Delivered(rush_sequence_t packet_id);
  void Lost(rush_sequence_t packet_id);
};

} // namespace Stream
} // namespace Rush
