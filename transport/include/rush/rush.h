/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"

#if defined __cplusplus
extern "C" {
#endif

typedef struct rush_context *rush_t;
typedef struct rush_endpoint *endpoint_t;

typedef float rush_time_t;
typedef u32 rush_sequence_t;

static const rush_time_t kRushConnectionTimeout = 10 * 1000.f;
static const u32 kHostnameMax = 128;

enum Connectivity {
  kRushConnected,
  kRushConnectionFailed,
  kRushEstablished,
  kRushDisconnected
};

enum Ack { kRushDelivered, kRushLost };

enum RushStreamType { kRushUpstream, kRushDownstream };

struct RushConnection {
  endpoint_t handle;
  u32 rtt;
  char hostname[kHostnameMax];
  u16 port;
  u32 upstream_bps;
  u32 downstream_bps;
  u16 packet_loss; // num packets lost during last kRushPacketLossTimeframe ms.
  rush_time_t send_interval; // congestion controller send interval in ms.
};

struct RushCallbacks {
  void (*startup)(rush_t ctx, const char *hostname, u16 port, void *data);
  void (*connectivity)(rush_t ctx, endpoint_t endpoint, Connectivity info,
                       void *data);
  void (*unpack)(rush_t ctx, endpoint_t endpoint, rush_sequence_t id,
                 const void *buffer, u16 nbytes, void *data);
  void (*out_of_order_unpack)(rush_t ctx, endpoint_t endpoint,
                              rush_sequence_t id, const void *buffer,
                              u16 nbytes, void *data);
  u16 (*pack)(rush_t ctx, endpoint_t endpoint, rush_sequence_t id, void *buffer,
              u16 nbytes, void *data);
  void (*ack)(rush_t ctx, endpoint_t endpoint, rush_sequence_t id, Ack type);
};

struct RushConfig {
  u32 hostname;
  u16 mtu;
  u16 port;
  RushCallbacks callbacks;
  void *data;
};

/// Creates a rush context instance operating on a given udp port.
/// @param port Port to operate on
/// @param cbs Rush callback.
/// @param data Data attached to callback.
rush_t rush_create(RushConfig *config);

/// Destroys given instance.
/// @param ctx Rush context.
void rush_destroy(rush_t ctx);

/// Updates a ginen context.
/// @param ctx Rush context.
/// @param dt Tick time in milliseconds.
void rush_update(rush_t ctx, rush_time_t dt);

/// Opens a udp connection to given destination.
/// @param ctx Rush context.
/// @param dest Destination address.
/// @return Endpoint handle. 0 if connection failed.
endpoint_t rush_open(rush_t ctx, const char *private_addr, u32 private_addr_len,
                     const char *public_addr, u32 public_addr_len);

/// Closes given endpoint.
/// @param ctx Rush context.
/// @param endpoint Endpoint to close.
void rush_close(rush_t ctx, endpoint_t endpoint);

/// Retrieves information about specified connection.
/// @param ctx rush_t
/// @param endpoint Connection of interest.
/// @param info Retrieved info.
/// @return 0 if info retrieved successfully.
int rush_connection_info(rush_t, endpoint_t endpoint, RushConnection *info);

/// Retrieves average round-trip time fot a given endpoint.
/// @param ctx Rush context.
/// @param endpoint Endpoint of interest.
/// @param time_ms Output time in milliseconds.
/// @return 0 if endpoint exists and rtt retrieved successfully.
int rush_rtt(rush_t ctx, endpoint_t endpoint, rush_time_t *time_ms);

/// Retrieve current rush library time.
/// @param ctx Rush context.
/// @param time Returned time
int rush_time(rush_t ctx, rush_time_t *time);

/// Starts module by reolving NAT assigned public address.
/// @param ctx Rush context.
/// @param server Punch server address.
/// @return 0 on success, otherwise false.
int rush_startup(rush_t ctx, const char *hostname, const char *service);

/// Shutdown rush.
/// @param ctx Rush context.
void rush_shutdown(rush_t ctx);

/// Change network regulation settings
/// @param ctx Rush context.
/// @param type Type of the stream limit to modify.
/// @param bps Set the number of bytes per second. -1 to disable the limit.
int rush_limit(rush_t ctx, RushStreamType type, u16 bps);

/// Get current regulation info.
/// @param ctx Rush context.
/// @param type Type of the stream to get the info of.
/// @param bps Bytes per second of the limit. -1 for no limit.
int rush_limit_info(rush_t ctx, RushStreamType type, u16 *bps);

/// Enable/disable network traffic logging
/// @param ctx Rush context.
/// @param file_path A path to the log file.
int rush_log(rush_t ctx, const char *file_path);

int rush_log_register(void (*loghook)(int level, const char *log_line,
                                      int length, void *context));

void rush_log_unregister(int logid);

#if defined __cplusplus
} // extern "C"
#endif

/*
Maximum packet RTT between A and B is:
  (A->B network time) + (B's frame delta) + (B's send interval) +
(B->A network time) + (A's frame delta).

For A_dt = 10ms, B_dt = 10ms, A_send = 33ms, B_send = 33ms and 0ms
network latency:
  Max RTT: 10ms + 33ms + 10ms = 53ms.
  Min RTT: 10ms. (at least one A's frame delta)
*/
