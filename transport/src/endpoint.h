/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "endpoint_log.h"
#include "congestion.h"
#include "base/core/types.h"
#include "ack_info.h"
#include "rtt.h"
#include "punch/include/client.h"

typedef u32 EndpointId;

struct rush_endpoint {
  EndpointId id;
  Base::Socket::Address addr;
  Rush::AckInfo ack;
  Rush::RttInfo *rtt;
  Rush::CongestionInfo *congestion;
  enum State { st_INVALID, st_CONNECTING, st_CONNECTED } state;
  Rush::PacketHistory *upstream_log;
  Rush::PacketHistory *downstream_log;
  punch_op punch_id;
};

namespace Rush {

bool IsConnected(endpoint_t endpoint);

bool CanAddEndpoint(rush_t ctx);
endpoint_t FindEndpoint(rush_t ctx, const Base::Socket::Address &address);
endpoint_t Validate(rush_t ctx, endpoint_t endpoint);
rush_time_t GetAverageSendInterval(rush_t ctx);
rush_time_t GetAverageRTT(rush_t ctx);

// Change a state of an endpoint from connecting to connected.
// @param ctx Rush context.
// @param punch_id Id of connect operation that succeeded.
// @param address Destination address to be updated.
endpoint_t ConnectEndpoint(rush_t ctx, punch_op punch_id,
                           const Base::Socket::Address &address);

// Remove endpoint because of operation failure.
// @param ctx Rush context.
// @param id Id of connect operation that failed.
bool RemoveEndpoint(rush_t ctx, punch_op id);

// Add endpoint in connecting state.
// @param ctx Rush context.
// @param punch_id Id of connect operation for endpoint.
endpoint_t AddEndpoint(rush_t ctx, punch_op punch_id);

// Add endpoint in connected state.
endpoint_t AddEndpoint(rush_t ctx, const Base::Socket::Address &address);
void RemoveEndpoint(rush_t ctx, endpoint_t endpoint);
void CheckTimeouts(rush_t ctx);

} // namespace Rush
