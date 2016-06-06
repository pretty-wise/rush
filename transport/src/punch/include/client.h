/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once
#include "base/network/url.h"
#include "base/network/socket.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef s32 punch_result;
typedef s32 punch_op;

const punch_result kPunchSuccess = 0;
const punch_result kPunchFailure = -1;
const punch_result kPunchAborted = -2;

typedef struct context *punch_t;
typedef void (*PunchOnResolve)(punch_result res, const char *public_hostname,
                               u16 public_service, void *data);
typedef void (*PunchOnEstablished)(punch_result res, punch_op punch_id,
                                   const char *hostname, u16 service,
                                   void *data);
typedef void (*PunchOnConnected)(const char *hostname, u16 service, void *data);

struct PunchConfig {
  Base::Socket::Handle socket;
  Base::Url private_address;
  Base::Url punch;
  Base::Url nat_detect[2];
  PunchOnEstablished established_cb;
  PunchOnConnected connected_cb;
  PunchOnResolve resolved_cb;
  void *data;
};

// todo(kstasik): change url to const char* to not break the ABI

/// Creates a client punch service.
/// @param config Punch configuration
/// @return Punch service context
punch_t punch_create(const PunchConfig &config);

/// Terminates punch service.
/// @param ctx Punch service context to terminate
void punch_destroy(punch_t ctx);

/// Updates the punch client serivce. Needs to be called frequently.
/// @param ctx Punch context to update
/// @param delta_ms Time elapsed since last update
void punch_update(punch_t ctx, u32 delta_ms);

/// Data received on a socket used by punch service.
/// @param ctx Punch context
/// @param buffer Data received on punch socket
/// @param nbytes Number of bytes passed in buffer
/// @param from Address of a host the data was received from
void punch_read(punch_t ctx, s8 *buffer, streamsize nbytes,
                const Base::Socket::Address &from);

/// Resolves public IP address of the machine.
/// @param ctx Punch service context
/// @param timeout_ms Operation timeout
punch_op punch_resolve_public_address(punch_t ctx, u32 timeout_ms);

/// Performs a NAT punch through between 2 devices
/// @param ctx Punch service context.
/// @param a_private A's private address
/// @param a_public A's public address
/// @param b_private B's private address
/// @param b_public B's public address
punch_op punch_connect(punch_t ctx, const Base::Url &a_private,
                       const Base::Url &a_public, const Base::Url &b_private,
                       const Base::Url &b_public);

/// Aborts punch operation
/// @param ctx Punch service context
/// @param id Operation to abort
void punch_abort(punch_t ctx, punch_op id);
// void punch_detect_nat(struct context* ctx);

#ifdef __cplusplus
} // extern "C"
#endif
