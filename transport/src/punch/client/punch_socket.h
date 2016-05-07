/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/network/socket.h"
#include "base/network/url.h"

namespace Rush {
namespace Punch {

class PunchSocket {
public:
  PunchSocket() : m_socket(Base::Socket::InvalidHandle) {}
  PunchSocket(Base::Socket::Handle sock);

  bool SendToPeer(const Base::Url &to, const void *data, u16 nbytes) const;
  bool Send(const Base::Url &to, const void *data, u16 nbytes) const;

private:
  Base::Socket::Handle m_socket;
};

} // namespace Punch
} // namespace Rush
