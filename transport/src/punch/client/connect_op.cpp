/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "connect_op.h"

#include "../common/punch_defs.h"
#include "../../packet.h"
#include "base/core/assert.h"
#include <cstring> // memcpy

namespace Rush {
namespace Punch {

namespace {
template <class T> u32 GetNextRetryTime(T *data) {
  /** Timeout:
          retransmit time doubles until it reaches 1.6sec.
          retransmit max retries 9 times.
          at 9.5sec the operation fails.
   */
  if(data->retry_time < 1600) {
    data->retry_time *= 2;
  }
  ++data->retry_count;
  if(data->retry_count >= 9) {
    return 0;
  }
  return data->retry_time;
}

u32 TimeoutToNextState(RequestId id, ConnectOperation *data,
                       const ConnectCommon &config) {
  data->retry_count = 0;
  data->retry_time = 100;
  switch(data->state) {
  case ConnectOperation::kProbing: {
    data->state = ConnectOperation::kReport;
    // todo(kstasik): store what to report.
  } break;
  case ConnectOperation::kReport: {
    BASE_DEBUG(kPunchLog, "punch id(%d) operation ended", id);
    return 0;
  }
  default:
    break;
  }
  return data->retry_time;
}

} // unnamed namespace

void CreateConnectRequest(ConnectRequest *data, RequestId req_id,
                          const Base::Url &a_private, const Base::Url &a_public,
                          const Base::Url &b_private,
                          const Base::Url &b_public) {
  data->retry_time = 100;
  data->retry_count = 0;
  data->a_private = a_private;
  data->a_public = a_public;
  data->b_private = b_private;
  data->b_public = b_public;
}

u32 UpdateConnectRequest(RequestId id, ConnectRequest *data,
                         const ConnectCommon &config) {
  Msg::ConnectRequest message(MessageType::kPunchThroughStart, id,
                              data->a_private, data->a_public, data->b_private,
                              data->b_public);
  message.hton();
  bool res = config.socket.Send(config.address, (const void *)&message,
                                sizeof(message));
  BASE_DEBUG(
      kPunchLog,
      "connect out(kPunchThroughStart) id(%d) request #%d, %dms. status '%s'",
      id, data->retry_count, data->retry_time, res ? "ok" : "fail");

  u32 retry_in = GetNextRetryTime(data);
  if(retry_in == 0) {
    // todo(kstasik): failure
  }
  return retry_in;
}

void CreateConnectOp(ConnectOperation *data, RequestId connect_id,
                     RequestId request_id, const Base::Url &priv_addr,
                     const Base::Url &pub_addr, bool is_outgoing) {
  data->state = ConnectOperation::kProbing;
  data->retry_time = 100;
  data->retry_count = 0;
  data->type =
      is_outgoing ? ConnectOperation::kEstablish : ConnectOperation::kConnect;
  data->request_id = request_id;
  data->punch_id = connect_id;
  data->private_url = priv_addr;
  data->public_url = pub_addr;
  data->connected = false;

  Base::Socket::Address::CreateUDP(data->private_url, &data->private_addr);
  Base::Socket::Address::CreateUDP(data->public_url, &data->public_addr);
}

u32 UpdateConnectOp(RequestId id, ConnectOperation *data,
                    const ConnectCommon &config) {
  switch(data->state) {
  case ConnectOperation::kProbing: {
    Msg::ConnectProbe message = {MessageType::kPunchThroughProbeRequest,
                                 Msg::ConnectProbe::kPrivate, id};
    message.hton();

    bool nat_device_present = data->private_url == data->public_url;
    bool private_sent = false;
    if(!nat_device_present) {
      private_sent = config.socket.SendToPeer(
          data->private_addr, static_cast<void *>(&message), sizeof(message));
    }
    /*bool public_sent = */ config.socket.SendToPeer(
        data->public_addr, static_cast<void *>(&message), sizeof(message));
    BASE_DEBUG(kPunchLog, "connect out(kPunchThroughProbeRequest) %s:%s "
                          "connect(%d) #%d, %dms.",
               PRINTF_URL(data->private_url), id, data->retry_count,
               data->retry_time);
    BASE_DEBUG(kPunchLog, "connect out(kPunchThroughProbeRequest) %s:%s "
                          "connect(%d), #%d, %dms.",
               PRINTF_URL(data->public_url), id, data->retry_count,
               data->retry_time);
  } break;
  case ConnectOperation::kReport: {
    BASE_DEBUG(kPunchLog, "connect id(%d) report #%d, %dms.", id,
               data->retry_count, data->retry_time);
    // todo(kstasik): report result to punch.
  } break;
  }
  u32 retry_in = GetNextRetryTime(data);
  if(retry_in == 0) {
    retry_in = TimeoutToNextState(id, data, config);
  }

  return retry_in;
}

void OnProbeRequest(RequestId id, ConnectOperation *data,
                    const Base::Socket::Address &from,
                    const Msg::ConnectProbe::Type type,
                    const ConnectCommon &config) {
  BASE_DEBUG(kPunchLog,
             "connect in(kPunchThroughProbeRequest) connect(%d) from %s", id,
             Base::Socket::Print(from));

  Msg::ConnectProbe message = {MessageType::kPunchThroughProbeResponse, type,
                               id};
  message.hton();

  if(from != data->private_addr && from != data->public_addr) {
    BASE_ERROR(kPunchLog,
               "received request from unknown ip address, discarding");
    return;
  }

  /*bool sent = */ config.socket.SendToPeer(from, static_cast<void *>(&message),
                                            sizeof(message));

  BASE_DEBUG(kPunchLog,
             "connect out(kPunchThroughProbeResponse) connect(%d) to %s", id,
             Base::Socket::Print(from));
}

void OnProbeResponse(RequestId id, ConnectOperation *data,
                     const Base::Socket::Address &from,
                     const Msg::ConnectProbe::Type type,
                     const ConnectCommon &config) {
  BASE_DEBUG(kPunchLog,
             "connect in(kPunchThroughProbeResponse) connect(%d) from %s", id,
             Base::Socket::Print(from));
  if(data->connected) {
    return; // already connected
  }
  data->connected = true;
  data->retry_count = 0;
  data->retry_time = 100;
  data->state = ConnectOperation::kReport;
  Base::Url url(from);
  if(data->type == ConnectOperation::kEstablish) {
    BASE_ASSERT(data->request_id != 0);
    BASE_INFO(kPunchLog, "connection established with %s connect(%d)",
              Base::Socket::Print(from), id);
    config.established_cb(kPunchSuccess, data->request_id, url.GetHostname(),
                          url.GetPort(), config.data);
  } else {
    BASE_INFO(kPunchLog, "connected with %s connect(%d)",
              Base::Socket::Print(from), id);
    config.connected_cb(url.GetHostname(), url.GetPort(), config.data);
  }
}

} // namespace Punch
} // namespace Rush
