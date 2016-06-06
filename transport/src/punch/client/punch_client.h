/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once
#include "base/core/types.h"
#include "../include/client.h"
#include "../common/punch_message.h"
#include "connect_op.h"
#include "resolve_op.h"
#include "timed_operations.h"
#include "punch_socket.h"

#include <vector>
#include <functional>

namespace Rush {
namespace Punch {

// todo(kstasik): move those to PunchConfig
static const u32 kMaxResolveOps = 10;
static const u32 kMaxConnectOps = 64;

bool IsValid(const PunchConfig &config);

class PunchClient {
public:
  PunchClient(const PunchConfig &config);
  ~PunchClient();

  punch_op ResolvePublicAddress(u32 timeout_ms);
  punch_op Connect(const Base::Url &a_private, const Base::Url &a_public,
                   const Base::Url &b_private, const Base::Url &b_public);
  void Update(u32 delta_ms);
  void Read(const void *data, streamsize nbytes,
            const Base::Socket::Address &from);
  void Abort(punch_op id);

private:
  punch_op GenerateId();

private:
  Base::Url m_private_addr;
  PunchSocket m_socket;
  ResolveCommon m_resolveData;
  ConnectCommon m_connectData;
  Base::Url m_publicUrl;
  punch_op m_generator;

  TimedOperations<ResolveOperation, punch_op, kMaxResolveOps> m_resolve;
  TimedOperations<ConnectRequest, punch_op, kMaxConnectOps> m_connect_request;
  TimedOperations<ConnectOperation, punch_op, kMaxConnectOps> m_connect_op;
};

} // namespace Punch
} // namespace Rush
