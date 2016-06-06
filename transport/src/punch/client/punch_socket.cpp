/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "punch_socket.h"

#include "base/core/assert.h"
#include <string>
#include <string.h>

#include "../../packet.h"

namespace Rush {
namespace Punch {

PunchSocket::PunchSocket(Base::Socket::Handle sock) : m_socket(sock) {}

bool PunchSocket::SendToPeer(const Base::Socket::Address &to, const void *data,
                             u16 nbytes) const {
  const u16 kMaxPacketSize = 1024;
  char packet[kMaxPacketSize];
  u16 packet_size = sizeof(Rush::PacketHeader) + nbytes;
  BASE_ASSERT(sizeof(Rush::PacketHeader) + nbytes < kMaxPacketSize,
              "packet capacity exceeded %d/%d", packet_size, kMaxPacketSize);

  Rush::PacketHeader header = {
      Rush::kPunchProtocol,
  };

  memcpy(packet, static_cast<const void *>(&header),
         sizeof(Rush::PacketHeader));
  if(nbytes > 0) {
    memcpy(packet + sizeof(Rush::PacketHeader), data, nbytes);
  }

  int error;
  return Base::Socket::Udp::Send(m_socket, to, packet, packet_size, &error);
}

bool PunchSocket::Send(const Base::Socket::Address &to, const void *data,
                       u16 nbytes) const {
  int error;
  bool res = Base::Socket::Udp::Send(m_socket, to, data, nbytes, &error);
  return res;
}

} // namespace Punch
} // namespace Rush
