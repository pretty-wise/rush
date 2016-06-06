/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "endpoint.h"
#include "rush_context.h"
#include "ack_info.h"

namespace Rush {

// number of logged upstream/downstream packets.
static const u16 kPacketLogSize = 500;

void OnDelivered(rush_sequence_t id, rush_time_t time, rush_t ctx,
                 endpoint_t endpoint) {
  rush_time_t rtt = endpoint->rtt->PacketReceived(id, time);
  endpoint->congestion->TickDelivered(time, id, rtt);
  if(ctx->callback.ack) {
    ctx->callback.ack(ctx, static_cast<endpoint_t>(endpoint), id,
                      Ack::kRushDelivered);
  }
}

void OnLost(rush_sequence_t id, rush_time_t time, rush_t ctx,
            endpoint_t endpoint) {
  //	BASE_LOG("discarding packet lost %p %d.\n", endpoint,
  // endpoint->addr.GetPort());
  endpoint->congestion->TickLost(time, id);
  if(ctx->callback.ack) {
    ctx->callback.ack(ctx, static_cast<endpoint_t>(endpoint), id,
                      Ack::kRushLost);
  }
}

bool IsConnected(endpoint_t endpoint) {
  return endpoint->state == rush_endpoint::st_CONNECTED;
}

bool CanAddEndpoint(rush_t ctx) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    if(ctx->endpoints[i].state == rush_endpoint::st_INVALID) {
      return true;
    }
  }
  return false;
}

endpoint_t FindEndpoint(rush_t ctx, const Base::Socket::Address &address) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    if(ctx->endpoints[i].state == rush_endpoint::st_CONNECTED &&
       ctx->endpoints[i].addr == address) {
      return &ctx->endpoints[i];
    }
  }
  return nullptr;
}

endpoint_t Validate(rush_t ctx, endpoint_t endpoint) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t handle = reinterpret_cast<endpoint_t>(&ctx->endpoints[i]);
    if(ctx->endpoints[i].state != rush_endpoint::st_INVALID &&
       handle == endpoint) {
      return &ctx->endpoints[i];
    }
  }
  return nullptr;
}

rush_time_t GetAverageSendInterval(rush_t ctx) {
  rush_time_t interval = 0;
  u32 endpoint_count = 0;
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state != rush_endpoint::st_INVALID) {
      interval += endpoint->congestion->SendInterval();
      ++endpoint_count;
    }
  }
  if(endpoint_count > 0) {
    return interval / endpoint_count;
  }
  return 0;
}

rush_time_t GetAverageRTT(rush_t ctx) {
  rush_time_t rtt = 0;
  u32 endpoint_count = 0;
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state != rush_endpoint::st_INVALID) {
      rtt += endpoint->rtt->GetAverage();
      ++endpoint_count;
    }
  }
  if(endpoint_count > 0) {
    return rtt / endpoint_count;
  }
  static const rush_time_t kDefaultRtt = 50;
  return kDefaultRtt;
}

void InitializeEndpoint(rush_t ctx, endpoint_t endpoint) {
  static EndpointId g_generator = 0;
  endpoint->id = ++g_generator;
  DeliveryCallbacks cbs = {&OnDelivered, &OnLost /*, &OnAkc*/};
  endpoint->addr = Base::Socket::Address();
  endpoint->punch_id = 0;
  endpoint->ack = AckInfo(cbs, ctx, endpoint);
  endpoint->rtt = new RttInfo(GetAverageRTT(ctx));
  rush_time_t min_interval = 33, max_interval = 500;
  rush_time_t cur_interval = GetAverageSendInterval(ctx);
  if(cur_interval == 0) {
    cur_interval = min_interval;
  }
  endpoint->congestion = new CongestionInfo(
      ctx->mtu, min_interval, max_interval, cur_interval, ctx->time);
  endpoint->upstream_log = new PacketHistory(ctx->mtu, kPacketLogSize);
  endpoint->downstream_log = new PacketHistory(ctx->mtu, kPacketLogSize);
}

void TerminateEndpoint(rush_t ctx, endpoint_t endpoint) {
  delete endpoint->rtt;
  delete endpoint->congestion;
  delete endpoint->upstream_log;
  delete endpoint->downstream_log;
  endpoint->addr = Base::Socket::Address();
  endpoint->punch_id = 0;
  endpoint->state = rush_endpoint::st_INVALID;
}

endpoint_t AddEndpoint(rush_t ctx, const Base::Socket::Address &address) {
  if(FindEndpoint(ctx, address) != nullptr) {
    return 0; // connection already exists!
  }
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    if(ctx->endpoints[i].state == rush_endpoint::st_INVALID) {
      endpoint_t endpoint = &ctx->endpoints[i];
      InitializeEndpoint(ctx, endpoint);
      ctx->num_endpoints++;
      endpoint->addr = address;
      endpoint->state = rush_endpoint::st_CONNECTED; // set at the end!
      return endpoint;
    }
  }
  return nullptr;
}

endpoint_t AddEndpoint(rush_t ctx, punch_op punch_id) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state == rush_endpoint::st_INVALID) {
      InitializeEndpoint(ctx, endpoint);
      ctx->num_endpoints++;
      endpoint->punch_id = punch_id;
      endpoint->state = rush_endpoint::st_CONNECTING; // set at the end!
      return endpoint;
    }
  }
  return nullptr;
}

endpoint_t ConnectEndpoint(rush_t ctx, punch_op punch_id,
                           const Base::Socket::Address &address) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state == rush_endpoint::st_CONNECTING &&
       endpoint->punch_id == punch_id) {
      endpoint->addr = address;
      endpoint->state = rush_endpoint::st_CONNECTED;
      return endpoint;
    }
  }
  return nullptr;
}

void RemoveEndpoint(rush_t ctx, endpoint_t endpoint) {
  TerminateEndpoint(ctx, endpoint);
  ctx->num_endpoints--;
}

bool RemoveEndpoint(rush_t ctx, punch_op id) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state == rush_endpoint::st_INVALID) {
      continue;
    }
    if(endpoint->punch_id != id) {
      continue;
    }
    RemoveEndpoint(ctx, endpoint);
    return true;
  }
  return false;
}

void CheckTimeouts(rush_t ctx) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state == rush_endpoint::st_INVALID) {
      continue;
    }
    if(endpoint->congestion->TimeSinceLastRecv(ctx->time) >
       kRushConnectionTimeout) {
      if(ctx->callback.connectivity) {
        ctx->callback.connectivity(ctx, reinterpret_cast<endpoint_t>(endpoint),
                                   endpoint->state ==
                                           rush_endpoint::st_CONNECTED
                                       ? Connectivity::kRushDisconnected
                                       : Connectivity::kRushConnectionFailed,
                                   ctx->callback_data);
      }
      RemoveEndpoint(ctx, endpoint);
    }
  }
}

} // namespace Rush
