/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "rush_context.h"
#include "base/core/str.h"

namespace Rush {

void OnEstablished(punch_result res, punch_op id, const char *hostname,
                   u16 port, void *data) {
  rush_t ctx = static_cast<rush_t>(data);
  if(res == kPunchFailure) {
    BASE_ERROR(Rush::kRush, "punch(%d) failed", id);
    RemoveEndpoint(ctx, id);
    return;
  }
  if(res == kPunchAborted) {
    RemoveEndpoint(ctx, id);
    return;
  }
  Base::Socket::Address address;
  if(!Base::Socket::Address::CreateUDP(hostname, port, &address)) {
    BASE_ERROR(Rush::kRush, "failed to create address");
    RemoveEndpoint(ctx, id);
    return;
  }

  BASE_INFO(Rush::kRush, "connection established punch(%d) with %s", id,
            Base::Socket::Print(address));
  endpoint_t endpoint = ConnectEndpoint(ctx, id, address);
  if(!endpoint) {
    BASE_ERROR(Rush::kRush, "failed to connect endpoint");
  }
}

void OnConnected(const char *hostname, u16 port, void *data) {
  rush_t ctx = static_cast<rush_t>(data);
  Base::Socket::Address addr;
  Base::Socket::Address::CreateUDP(hostname, port, &addr);
  BASE_INFO(Rush::kRush, "endpoint connected %s", Base::Socket::Print(addr));
  endpoint_t endpoint = AddEndpoint(ctx, addr);
  if(!endpoint) {
    BASE_ERROR(Rush::kRush, "failed adding endpoint %s",
               Base::Socket::Print(addr));
    return;
  }
  if(ctx->callback.connectivity) {
    ctx->callback.connectivity(ctx, endpoint, Connectivity::kRushConnected,
                               ctx->callback_data);
  }
}

void OnPublicResolve(punch_result res, const char *public_hostname,
                     u16 public_port, void *data) {
  rush_t ctx = static_cast<rush_t>(data);
  if(res == kPunchFailure || res == kPunchAborted) {
    BASE_ERROR(Rush::kRush, "failed to resolve public address");
    ctx->public_addr = Base::Url(Base::AddressIPv4(0), 0);
    ctx->public_addr = ctx->private_addr; // todo(kstasik): remove this
    // todo(kstasik): report failure back
    ctx->callback.startup(ctx, public_hostname, public_port,
                          ctx->callback_data);
    return;
  }
  Base::Url public_url(public_hostname, public_port);
  BASE_INFO(Rush::kRush, "public address resolved to: %s:%s",
            PRINTF_URL(public_url));
  ctx->public_addr = public_url;
  ctx->callback.startup(ctx, public_hostname, public_port, ctx->callback_data);
}

bool Create(rush_t ctx, const Base::Url &private_addr, u16 *_port, u16 _mtu,
            RushCallbacks _cbs, void *_udata) {
  u16 port = private_addr.GetPort();
  Base::Socket::Handle socket = Base::Socket::Udp::Open(&port);
  if(socket == Base::Socket::InvalidHandle) {
    BASE_ERROR(Rush::kRush, "cound not open socket on port %d", port);
    return false;
  }
  *_port = port;
  ctx->socket = socket;
  ctx->private_addr = Base::Url(private_addr.GetHostname(), port);
  ctx->time = 0;
  ctx->callback = _cbs;
  ctx->callback_data = _udata;
  ctx->mtu = _mtu;
  ctx->buffer = malloc(ctx->mtu);
  ctx->num_endpoints = 0;
  ctx->send_index = 0;

  ctx->upstream.enabled = false;
  ctx->downstream.enabled = false;

  ctx->punch = nullptr;
  ctx->logger = new TransmissionLog(_mtu);

  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    ctx->endpoints[i].state = rush_endpoint::st_INVALID;
  }
  return true;
}

void Destroy(rush_t ctx) {
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state == rush_endpoint::st_INVALID) {
      continue;
    }
    if(ctx->callback.connectivity) {
      ctx->callback.connectivity(ctx, endpoint,
                                 endpoint->state == rush_endpoint::st_CONNECTED
                                     ? Connectivity::kRushDisconnected
                                     : Connectivity::kRushConnectionFailed,
                                 ctx->callback_data);
    }
    RemoveEndpoint(ctx, endpoint);
  }
  Base::Socket::Close(ctx->socket);
  delete ctx->logger;
  if(ctx->punch) {
    punch_destroy(ctx->punch);
  }
  free(ctx->buffer);
}

bool Startup(rush_t ctx, const Base::Url &server) {
  if(ctx->punch) {
    punch_destroy(ctx->punch);
  }
  PunchConfig config{ctx->socket,      ctx->private_addr,       server,
                     {server, server}, OnEstablished,           OnConnected,
                     OnPublicResolve,  static_cast<void *>(ctx)};
  ctx->punch = punch_create(config);
  if(!ctx->punch) {
    BASE_ERROR(Rush::kRush, "error creating punch");
    return false;
  }

  u32 timeout_ms = 10 * 1000; // todo(kstasik): move to config.
  punch_resolve_public_address(ctx->punch, timeout_ms);
  return true;
}

void Shutdown(rush_t context) {}

endpoint_t Open(rush_t ctx, const Base::Url &private_addr,
                const Base::Url &public_addr) {
  if(!ctx->punch) {
    BASE_ERROR(Rush::kRush, "no punch");
    return nullptr;
  }
  if(Base::String::strlen(ctx->private_addr.GetHostname()) > 0) {
    BASE_ERROR(Rush::kRush,
               "private address not known. should be provided at rush start");
    return nullptr;
  }
  if(Base::String::strlen(ctx->public_addr.GetHostname()) > 0) {
    BASE_ERROR(Rush::kRush, "public address not known. start rush first");
    return nullptr;
  }
  if(!CanAddEndpoint(ctx)) {
    BASE_ERROR(Rush::kRush, "max endpoints connected/connecting");
    return nullptr;
  }

  punch_op punch_id =
      punch_connect(ctx->punch, ctx->private_addr, ctx->public_addr,
                    private_addr, public_addr);
  if(punch_id == 0) {
    BASE_ERROR(Rush::kRush, "failed to start punch operation");
    return nullptr;
  }

  BASE_INFO(Rush::kRush, "adding endpoint punch(%d) operation", punch_id);
  endpoint_t endpoint = AddEndpoint(ctx, punch_id);
  if(!endpoint) {
    BASE_ERROR(Rush::kRush, "can't reserve an endpoint");
    punch_abort(ctx->punch, punch_id);
    return nullptr;
  }

  return endpoint;
}

void Close(rush_t ctx, endpoint_t endpoint) {
  endpoint_t peer = Validate(ctx, endpoint);
  if(!peer) {
    return;
  }
  bool is_connecting = peer->state == rush_endpoint::st_CONNECTING;
  if(is_connecting) {
    punch_abort(ctx->punch, endpoint->punch_id);
  }
  if(ctx->callback.connectivity) {
    ctx->callback.connectivity(
        ctx, endpoint, is_connecting ? Connectivity::kRushConnectionFailed
                                     : Connectivity::kRushDisconnected,
        ctx->callback_data);
  }
  RemoveEndpoint(ctx, peer);
}

bool GetConnectionInfo(rush_t ctx, endpoint_t endpoint, RushConnection *info) {
  // todo(kstasik): configure somewhere?
  static const u16 kPacketLossTimeframe = 1000;
  endpoint_t peer = Validate(ctx, endpoint);
  if(!peer || !info) {
    return false;
  }

  info->handle = endpoint;
  info->rtt = peer->rtt->GetAverage();
  info->send_interval = peer->congestion->SendInterval();
  Base::Url url(peer->addr);
  Base::String::strncpy(info->hostname, url.GetHostname(),
                        Base::Url::kHostnameMax);
  info->port = url.GetPort();

  const rush_time_t timeframe = 3000; // millisecond timeframe
  rush_time_t start = ctx->time - timeframe;
  rush_time_t end = ctx->time;
  info->upstream_bps = peer->upstream_log->GetDataReceived(&start, &end);
  info->upstream_bps = (info->upstream_bps) / ((ctx->time - start) / 1000);
  start = ctx->time - timeframe;
  end = ctx->time;
  info->downstream_bps = peer->downstream_log->GetDataReceived(&start, &end);
  info->downstream_bps = (info->downstream_bps * 1000) / (ctx->time - start);
  info->packet_loss = peer->congestion->NumLostPackets(
      ctx->time - kPacketLossTimeframe, ctx->time);
  return true;
}

bool GetRtt(rush_t ctx, endpoint_t _endpoint, rush_time_t *time_ms) {
  if(!time_ms) {
    return false;
  }
  for(u32 i = 0; i < kMaxEndpoint; ++i) {
    endpoint_t endpoint = &ctx->endpoints[i];
    if(endpoint->state != rush_endpoint::st_INVALID && endpoint == _endpoint) {
      *time_ms = endpoint->rtt->GetAverage();
      return true;
    }
  }
  return false;
}

bool GetTime(rush_t ctx, rush_time_t *time) {
  if(!time) {
    return false;
  }
  *time = ctx->time;
  return true;
}

bool EnableRegulation(rush_t ctx, RushStreamType type, u16 bytes_per_second) {
  ThroughputRegulation &reg =
      type == kRushUpstream ? ctx->upstream : ctx->downstream;
  reg.enabled = true;
  reg.limit.SetCapacity(bytes_per_second, ctx->time);
  return true;
}

bool DisableRegulation(rush_t ctx, RushStreamType type) {
  ThroughputRegulation &reg =
      type == kRushUpstream ? ctx->upstream : ctx->downstream;
  reg.enabled = false;
  return false;
}

bool GetRegulationInfo(rush_t ctx, RushStreamType type, bool *enabled,
                       u16 *bytes_per_second) {
  ThroughputRegulation &reg =
      type == kRushUpstream ? ctx->upstream : ctx->downstream;
  *enabled = reg.enabled;
  *bytes_per_second = reg.limit.capacity;
  return true;
}

void PacketReceived(rush_t ctx, const Base::Socket::Address &from,
                    const void *buffer, streamsize nbytes) {
  const PacketHeader &header = *reinterpret_cast<const PacketHeader *>(buffer);
  if(header.protocol == kPunchProtocol) {
    punch_read(ctx->punch, (s8 *)buffer + sizeof(PacketHeader),
               nbytes - sizeof(PacketHeader), from);
  }
  if(header.protocol == kGameProtocol) {
    endpoint_t endpoint = FindEndpoint(ctx, from);
    if(endpoint) {
      if(!IsConnected(endpoint)) {
        return;
      }
      // BASE_DEBUG(Rush::kRush, "recv game data on valid endpoint(%p)",
      // endpoint);
      endpoint->congestion->TickRecv(ctx->time);
      endpoint->downstream_log->Log(header.id, ctx->time, ctx->buffer,
                                    ctx->mtu);
      if(endpoint->ack.InOrder(header)) {
        endpoint->ack.Received(header, ctx->time);
        // processing done after ack.Received, because it will update
        // congestion
        // data.
        const void *game_data = (u8 *)ctx->buffer + sizeof(PacketHeader);
        const u16 game_nbytes = nbytes - sizeof(PacketHeader);
        ctx->logger->LogDownstream(endpoint->id, header.id, ctx->time,
                                   game_data, game_nbytes);
        if(ctx->callback.unpack) {
          ctx->callback.unpack(ctx, endpoint, header.id, game_data, game_nbytes,
                               ctx->callback_data);
        }
      } else {
        if(ctx->callback.out_of_order_unpack) {
          ctx->callback.out_of_order_unpack(
              ctx, endpoint, header.id,
              (u8 *)ctx->buffer + sizeof(PacketHeader),
              nbytes - sizeof(PacketHeader), ctx->callback_data);
        }
      }
    }
  }
}

void Update(rush_t ctx, rush_time_t dt) {
  static int s_frame_id = 0;
  s_frame_id++;
  ctx->time += dt;

  punch_update(ctx->punch, dt);

  int error;
  Base::Socket::Address from;
  streamsize received = 0;
  // todo(kstasik): limit this to some amount of time?
  while((received = Base::Socket::Udp::Recv(ctx->socket, &from, ctx->buffer,
                                            ctx->mtu, &error)) != 0) {
    if(received == -1) {
      BASE_LOG_LINE("socket error %d on connection", error);
      // todo(kstasik): process socket error. close connection.
      continue;
    }

    if(received < static_cast<streamsize>(sizeof(PacketHeader))) {
      BASE_LOG_LINE("packet size smaller then header (%d) - discarding",
                    received);
      continue;
    }

    /*BASE_LOG(
        "data received from %s:%s. %dB. header: %luB. time %f.\n ",
        PRINTF_URL(from), received, sizeof(PacketHeader), ctx->time);*/
    if(ctx->downstream.enabled &&
       ctx->downstream.limit.Consume(received, ctx->time) == false) {
      // BASE_LOG("downstream limit hit. discarding packet from
      // %s:%s.", PRINTF_URL(from));
      continue;
    }

    PacketReceived(ctx, from, ctx->buffer, received);
  }

  // ctx->send_index = Base::Math::Rand(kMaxEndpoint);
  u32 first_send_index = ctx->send_index;
  do {
    endpoint_t peer = &ctx->endpoints[ctx->send_index];
    if(!IsConnected(peer)) {
      continue;
    }
    if(peer->state == rush_endpoint::st_INVALID) {
      continue;
    }
    if(!peer->congestion->ShouldSendPacket(ctx->time)) {
      continue;
    }

    PacketHeader &header = *reinterpret_cast<PacketHeader *>(ctx->buffer);
    peer->ack.CreateHeader(&header);
    void *game_data = (u8 *)ctx->buffer + sizeof(PacketHeader);
    const u16 data_packed =
        ctx->callback.pack
            ? ctx->callback.pack(
                  ctx, reinterpret_cast<endpoint_t>(peer), header.id, game_data,
                  ctx->mtu - sizeof(PacketHeader), ctx->callback_data)
            : 0;
    u16 packet_size = data_packed + sizeof(PacketHeader);
    bool sent;
    bool should_break = false;
    if(ctx->upstream.enabled &&
       ctx->upstream.limit.Consume(packet_size, ctx->time) == false) {
      BASE_LOG("upstream limit reached. discarding packet");
      sent = true; // packet is considered sent and lost during transmission.
      should_break = true;
    } else {
      // BASE_DEBUG(Rush::kRush, "send game data to endpoint(%p)", peer);
      // TODO(kstasik): does not need to go through socket if it's a loopback
      // connection.
      // the loopback socket send introduces 'frame step' delay on packet send
      // and 'frame step' delay on ack receive therefore local packet RTT is
      // (2*frame_step+congestion_send_interval)
      sent = Base::Socket::Udp::Send(ctx->socket, peer->addr, ctx->buffer,
                                     packet_size, &error);
    }

    if(sent) {
      /*BASE_LOG("data sent to	 %s:%s. %dB. header: %luB. seq: %d "
               "ack: %d. time %f, frame: %d.\n",
               PRINTF_URL(peer->url), packet_size, sizeof(PacketHeader),
               header.id, header.ack, ctx->time, s_frame_id);*/
      peer->upstream_log->Log(header.id, ctx->time, ctx->buffer, ctx->mtu);
      peer->rtt->PacketSent(header.id, ctx->time);
      peer->congestion->TickSend(ctx->time, header.id);
      ctx->logger->LogUpstream(peer->id, header.id, ctx->time, game_data,
                               data_packed, peer->rtt->GetAverage(),
                               peer->congestion->SendInterval());
    } else {
      // todo(kstasik): check error and close endpoint.
      BASE_LOG("could not send data to %s. error: %d.\n",
               Base::Socket::Print(peer->addr), error);
    }

    if(should_break) {
      break;
    }
  } while((ctx->send_index = (ctx->send_index + 1) % kMaxEndpoint) !=
          first_send_index);

  CheckTimeouts(ctx);
}

bool SetLog(rush_t ctx, const char *file_path) {
  return ctx->logger->Open(file_path);
}

} // namespace Rush
