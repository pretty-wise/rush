/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once
#include "base/core/types.h"
#include "rush/punch/server.h"
#include "../common/punch_message.h"
#include "base/network/url.h"
#include "base/network/socket.h"

#include <vector>

namespace Rush {
namespace Punch {

static const s32 kPunchCommandTimeoutMs = 10 * 1000;
static const s32 kPunchCommandRetryMs = 1000;

struct PunchOperation {
  // kRequest means a_url is the originator.
  enum Type { kRequest, kExternal } type;
  Base::Socket::Address a_public;
  Base::Socket::Address a_private;
  Base::Socket::Address b_public;
  Base::Socket::Address b_private;
  s32 punch_resend;
  s32 timeout;
  Rush::Punch::RequestId request_id;
  Rush::Punch::RequestId connect_id;
};

class PunchServer {
public:
  PunchServer();
  ~PunchServer();

  bool Open(const Base::Url &url, u16 *port);
  void Update(u32 dt);

private:
  void UpdatePunchOperations(u32 dt);

private:
  Base::Socket::Handle m_socket;
  Base::Url m_url;
  std::vector<PunchOperation> m_punch_ops;
  Rush::Punch::RequestId m_generator;
};

} // namespace Punch
} // namespace Rush
