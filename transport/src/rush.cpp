/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "base/core/macro.h"
#include "base/math/math.h"
#include "congestion.h"
#include "rush/rush.h"
#include "rush_context.h"

rush_t rush_create(RushConfig *config) {
  Base::Url private_addr(Base::AddressIPv4(config->hostname), config->port);
  rush_t ctx = new rush_context;
  if(!Rush::Create(ctx, private_addr, &config->port, config->mtu,
                   config->callbacks, config->data)) {
    delete ctx;
    BASE_ERROR(Rush::kRush, "failed to initialize");
    return nullptr;
  }
  return ctx;
}

void rush_destroy(rush_t ctx) {
  Rush::Destroy(ctx);
  delete ctx;
}

// int rush_startup(rush_t ctx, const Base::Url &server) {
int rush_startup(rush_t ctx, const char *hostname, const char *service) {
  Base::Url server(hostname, service);
  return Rush::Startup(ctx, server) ? 0 : -1;
}

void rush_shutdown(rush_t ctx) { Rush::Shutdown(ctx); }

endpoint_t rush_open(rush_t ctx, const Base::Url &private_addr,
                     const Base::Url &public_addr) {
  return Rush::Open(ctx, private_addr, public_addr);
}

void rush_close(rush_t ctx, endpoint_t endpoint) { Rush::Close(ctx, endpoint); }

int rush_connection_info(rush_t ctx, endpoint_t endpoint,
                         RushConnection *info) {
  return Rush::GetConnectionInfo(ctx, endpoint, info) ? 0 : (int)-1;
}

int rush_rtt(rush_t ctx, endpoint_t endpoint, rush_time_t *time_ms) {
  return Rush::GetRtt(ctx, endpoint, time_ms) ? 0 : (int)-1;
}

int rush_time(rush_t ctx, rush_time_t *time) {
  return Rush::GetTime(ctx, time) ? 0 : (int)-1;
}

int rush_limit(rush_t ctx, RushStreamType type, u16 bps) {
  if(bps == static_cast<u16>(-1)) {
    Rush::DisableRegulation(ctx, type);
    return 0;
  }
  return Rush::EnableRegulation(ctx, type, bps) ? 0 : (int)-1;
}

int rush_limit_info(rush_t ctx, RushStreamType type, u16 *bps) {
  bool enabled = false;
  bool result = Rush::GetRegulationInfo(ctx, type, &enabled, bps);
  if(!enabled) {
    *bps = static_cast<u16>(-1);
  }
  return result ? 0 : (int)-1;
}

void rush_update(rush_t ctx, rush_time_t dt) { Rush::Update(ctx, dt); }

int rush_log(rush_t ctx, const char *file_path) {
  return Rush::SetLog(ctx, file_path) ? 0 : -1;
}
int rush_log_register(void (*loghook)(int level, const char *log_line,
                                      int length, void *context)) {
  return Base::Log::Register((int)-1, loghook, 0);
}

void rush_log_unregister(int id) { Base::Log::Unregister(id); }
