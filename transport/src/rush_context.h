/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"
#include "endpoint.h"
#include "bandwidth_limiter.h"
#include "punch/include/client.h"
#include "transmission_log.h"

namespace Rush {

static const Base::LogChannel kRush("rush");
static const u32 kMaxEndpoint = 64;

struct ThroughputRegulation {
  BandwidthLimiter limit;
  bool enabled;
};
} // namespace Rush

struct rush_context {
  Base::Socket::Handle socket;
  Base::Url private_addr;
  Base::Url public_addr;
  rush_time_t time;
  void *buffer;
  u16 mtu;
  RushCallbacks callback;
  void *callback_data;
  rush_endpoint endpoints[Rush::kMaxEndpoint];
  u32 num_endpoints;
  u32 send_index;
  Rush::ThroughputRegulation upstream;
  Rush::ThroughputRegulation downstream;
  punch_t punch;
  Rush::TransmissionLog *logger;
};

namespace Rush {

bool Create(rush_t ctx, Base::Url *private_addr, u16 _mtu, RushCallbacks _cbs,
            void *_udata);
void Destroy(rush_t ctx);
bool Startup(rush_t ctx, const Base::Url &server);
void Shutdown(rush_t ctx);
endpoint_t Open(rush_t ctx, const Base::Url &private_addr,
                const Base::Url &public_addr);
void Close(rush_t ctx, endpoint_t endpoint);
bool GetConnectionInfo(rush_t ctx, endpoint_t endpoint, RushConnection *info);
bool GetRtt(rush_t ctx, endpoint_t endpoint, rush_time_t *time_ms);
bool GetTime(rush_t ctx, rush_time_t *time);
void SetupPunch(rush_t ctx, const Base::Url &connection,
                const Base::Url nat_detect[2]);
bool EnableRegulation(rush_t ctx, RushStreamType type, u16 bytes_per_second);
bool DisableRegulation(rush_t ctx, RushStreamType type);
bool GetRegulationInfo(rush_t ctx, RushStreamType type, bool *enabled,
                       u16 *bytes_per_second);
void Update(rush_t ctx, rush_time_t delta_ms);
bool SetLog(rush_t ctx, const char *file_path);

} // namespace Rush
