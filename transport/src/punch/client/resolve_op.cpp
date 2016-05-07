/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "resolve_op.h"
#include "../include/client.h"
#include "../common/punch_defs.h"

namespace Rush {
namespace Punch {

namespace {

u32 GetNextRetryTime(ResolveOperation *data) {
  if(data->state == ResolveOperation::kKeepAlive) {
    return kKeepAliveTimeMs;
  }
  /** Timeout:
          retransmit time doubles until it reaches 1.6sec.
          retransmit max retries kResolveRetryCount times.
          at 9.5sec the operation fails.
   */
  if(data->retry_time < 1600) {
    data->retry_time *= 2;
  }
  ++data->retry_count;
  if(data->retry_count >= kResolveRetryCount) {
    return 0;
  }
  return data->retry_time;
}

u32 TimeoutToNextState(RequestId id, ResolveOperation *data,
                       const ResolveCommon &config) {
  switch(data->state) {
  case ResolveOperation::kBindingRequest: {
    data->retry_count = 0;
    data->retry_time = 100;
    data->state = ResolveOperation::kReportResult;
    BASE_WARN(kPunchLog, "resolve id(%d) failed", id);
    config.resolved_cb(kPunchFailure, 0, 0, config.data);
  } break;
  case ResolveOperation::kReportResult: {
    BASE_DEBUG(kPunchLog, "resolve id(%d) operation ended", id);
    data->retry_count = 0;
    data->retry_time = kKeepAliveTimeMs;
    data->state = ResolveOperation::kKeepAlive;
  } break;
  case ResolveOperation::kKeepAlive: {
    break;
  }
  }
  return data->retry_time;
}

} // unnamed namespace

void CreateResolveOp(ResolveOperation *data, RequestId req_id,
                     PunchSocket socket, const Base::Url &private_address) {
  data->state = ResolveOperation::kBindingRequest;
  data->retry_count = 0;
  data->retry_time = 100;
  data->message.type = MessageType::kMappedAddressRequest;
  data->message.id = req_id;
  data->message.private_address = private_address.GetAddress().GetRaw();
  data->message.private_port = private_address.GetPort();
}

u32 UpdateResolveOp(RequestId id, ResolveOperation *data,
                    const ResolveCommon &config) {
  switch(data->state) {
  case ResolveOperation::kBindingRequest: {
    Msg::BindingRequest message = data->message;
    message.hton();
    bool res = config.socket.Send(config.punch, (const void *)&message,
                                  sizeof(message));
    BASE_DEBUG(
        kPunchLog, "resolve out(kMappedAddressRequest) id(%d) binding request "
                   "#%d, %dms status: '%s'",
        id, data->retry_count, data->retry_time, res ? "sent" : "failed");
  } break;
  case ResolveOperation::kReportResult: {
    // todo(kstasik): report result
    BASE_DEBUG(kPunchLog, "resolve id(%d) report #%d, %dms", id,
               data->retry_count, data->retry_time);
  } break;
  case ResolveOperation::kKeepAlive: {
    BASE_DEBUG(kPunchLog, "out(kKeepAlive) id(%d)", id);
    Msg::KeepAlive message = {MessageType::kMappedAddressKeepAlive};
    config.socket.Send(config.punch, (const void *)&message, sizeof(message));
  } break;
  }
  u32 retry_time = GetNextRetryTime(data);
  if(retry_time == 0) {
    retry_time = TimeoutToNextState(id, data, config);
  }
  return retry_time;
}

void OnBindingResponse(RequestId id, ResolveOperation *data,
                       const Base::Url &private_url,
                       const Base::Url &public_url,
                       const ResolveCommon &config) {
  data->state = ResolveOperation::kReportResult;
  data->retry_count = 0;
  data->retry_time = 100;
  BASE_DEBUG(kPunchLog, "resolve id(%d) binding response received priv: "
                        "%d.%d.%d.%d:%d, pub: %d.%d.%d.%d:%d",
             id, PRINTF_URL(private_url), PRINTF_URL(public_url));
  config.resolved_cb(kPunchSuccess, public_url.GetAddress().GetRaw(),
                     public_url.GetPort(), config.data);
}

} // namespace Punch
} // namespace Rush
