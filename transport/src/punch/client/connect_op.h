/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "../include/client.h"
#include "../common/punch_message.h"
#include "base/network/url.h"
#include "punch_socket.h"

/** The flow of the connect operation is as follows:
 * - A sends Msg::ConnectRequest to server S
 * - S sends Msg::StartConnect to A and B
 * - A and B send Msg::ConnectProbe to each other
*/

namespace Rush {
namespace Punch {

static const s32 kConnectRetryTimeMs = 500;
static const s32 kConnectRetryCount = 10;

// Data common to all connect operations.
struct ConnectCommon {
  PunchSocket socket;
  Base::Url punch_url;
  Base::Socket::Address address;
  PunchOnEstablished established_cb;
  PunchOnConnected connected_cb;
  void *data;
};

struct ConnectRequest {
  u32 retry_time;
  u32 retry_count;
  Base::Url connect_pub;
  Base::Url connect_priv;
  Base::Url a_private;
  Base::Url a_public;
  Base::Url b_private;
  Base::Url b_public;
};

struct ConnectOperation {
  enum State { kProbing, kReport } state;
  u32 retry_time;
  u32 retry_count;
  enum Type { kEstablish, kConnect } type;
  RequestId request_id;
  RequestId punch_id;
  Base::Url private_url;
  Base::Url public_url;
  Base::Socket::Address private_addr;
  Base::Socket::Address public_addr;
  bool connected;
};

void CreateConnectRequest(ConnectRequest *data, RequestId req_id,
                          const Base::Url &a_private, const Base::Url &a_public,
                          const Base::Url &b_private,
                          const Base::Url &b_public);

u32 UpdateConnectRequest(RequestId id, ConnectRequest *data,
                         const ConnectCommon &config);

void OnConnectResponse(RequestId id, ConnectRequest *data,
                       const Base::Url &priv_addr, const Base::Url &pub_addr);

void CreateConnectOp(ConnectOperation *data, RequestId connect_id,
                     RequestId request_id, const Base::Url &priv_addr,
                     const Base::Url &pub_addr, bool is_outgoing);

u32 UpdateConnectOp(RequestId id, ConnectOperation *data,
                    const ConnectCommon &config);

void OnProbeRequest(RequestId id, ConnectOperation *data,
                    const Base::Socket::Address &from,
                    const Msg::ConnectProbe::Type type,
                    const ConnectCommon &config);
void OnProbeResponse(RequestId id, ConnectOperation *data,
                     const Base::Socket::Address &from,
                     const Msg::ConnectProbe::Type type,
                     const ConnectCommon &config);

} // namespace Punch
} // namespace Rush
