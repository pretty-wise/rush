/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "../common/punch_message.h"
#include "punch_socket.h"
#include "../include/client.h"

namespace Rush {
namespace Punch {

static const s32 kResolveRetryCount = 9;
static const u32 kKeepAliveTimeMs = 5000;

struct ResolveCommon {
  PunchSocket socket;
  PunchOnResolve resolved_cb;
  void *data;
  Base::Url punch_url;
  Base::Socket::Address punch_addr;
};

struct ResolveOperation {
  enum State { kBindingRequest, kReportResult, kKeepAlive } state;
  u32 retry_time;
  s32 retry_count;
  Msg::BindingRequest message;
};

void CreateResolveOp(ResolveOperation *data, RequestId req_id,
                     PunchSocket socket, const Base::Url &private_address);

u32 UpdateResolveOp(RequestId id, ResolveOperation *data,
                    const ResolveCommon &config);

void OnBindingResponse(RequestId id, ResolveOperation *data,
                       const Base::Url &private_url,
                       const Base::Url &public_url,
                       const ResolveCommon &config);

} // namespace Punch
} // namespace Rush
