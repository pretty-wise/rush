/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "gtest/gtest.h"

#include "base/network/socket.h"
#include "base/network/url.h"
#include "base/threading/thread.h"
#include "base/core/time_utils.h"

#include "rush/rush.h"
#include "../src/packet.h"
#include "../src/bandwidth_limiter.h"

#include <map>

TEST(BandwidthLimiter, General) {
  rush_time_t session_time = 0;
  const rush_time_t end_time = 1000;
  const rush_time_t delta = 16;

  u32 mtu = 1024;
  u32 throughput = 128 * 1000;
  Rush::BandwidthLimiter limiter(throughput, session_time);

  u32 bytes_transfered = 0;
  while(session_time < end_time) {
    session_time += delta;
    while(limiter.Consume(mtu, session_time)) {
      bytes_transfered += mtu;
    }
  }

  printf("bandwidth limiter test ended with %dB transmitted\n",
         bytes_transfered);

  ASSERT_TRUE(bytes_transfered <= throughput);
}

struct TestInfo {
  struct ConnectionData {
    ConnectionData()
        : bytes_sent(0), bytes_received(0), packets_sent(0),
          packets_received(0) {}

    u16 bytes_sent;
    u16 bytes_received;
    u16 packets_sent;
    u16 packets_received;
  };
  bool AddConnection(endpoint_t h) {
    if(connections.find(h) != connections.end()) {
      return false;
    }
    ConnectionData conn;
    connections.insert(std::make_pair(h, conn));
    return true;
  }

  bool RemoveConnection(endpoint_t h) {
    if(connections.find(h) == connections.end()) {
      return false;
    }
    connections.erase(connections.find(h));
    return true;
  }

  void AddSent(endpoint_t h, u16 nbytes) {
    if(!IsConnected(h)) {
      return;
    }
    connections[h].bytes_sent += nbytes;
    ++connections[h].packets_sent;
  }

  void AddReceived(endpoint_t h, u16 nbytes) {
    if(!IsConnected(h)) {
      return;
    }
    connections[h].bytes_received += nbytes;
    ++connections[h].packets_received;
  }

  u32 NumConnections() const { return connections.size(); }

  bool IsConnected(endpoint_t h) const {
    return connections.find(h) != connections.end();
  }

  void Print() const {
    for(std::map<endpoint_t, ConnectionData>::const_iterator it =
            connections.begin();
        it != connections.end(); ++it) {
      printf(
          "connection %p: sent %dB (%d packets), received %dB (%d packets)\n",
          (*it).first, (*it).second.bytes_sent, (*it).second.packets_sent,
          (*it).second.bytes_received, (*it).second.packets_received);
    }
  }

  std::map<endpoint_t, ConnectionData> connections;
};

static void OnStartup(rush_t ctx, u32 addr, u16 port, void *data) {}
static void Connectivity(rush_t ctx, endpoint_t endpoint, Connectivity event,
                         void *data) {
  TestInfo *info = static_cast<TestInfo *>(data);
  printf("connectivity context %p endpoint %p : %s\n", ctx, endpoint,
         ConnectivityToString(event));

  if(info) {
    if(event == kRushConnected || event == kRushEstablished) {
      ASSERT_TRUE(info->AddConnection(endpoint));
    } else if(event == kRushDisconnected) {
      ASSERT_TRUE(info->RemoveConnection(endpoint));
    } else if(event == kRushConnectionFailed) {
      info->RemoveConnection(endpoint);
    }
  }
}

static u16 Pack(rush_t ctx, endpoint_t endpoint, rush_sequence_t id,
                void *buffer, u16 nbytes, void *data) {
  TestInfo *info = static_cast<TestInfo *>(data);
  if(info) {
    info->AddSent(endpoint, nbytes);
  }
  return nbytes; // fully packed buffer.
}

static void Unpack(rush_t ctx, endpoint_t endpoint, rush_sequence_t id,
                   const void *buffer, u16 nbytes, void *data) {
  TestInfo *info = static_cast<TestInfo *>(data);
  if(info) {
    info->AddReceived(endpoint, nbytes);
  }
  printf("received %d\n", nbytes);
}

static RushCallbacks callbacks = {&OnStartup, &Connectivity, &Unpack, 0, &Pack};

TEST(Congestion, Main) {
  TestInfo info_a, info_b;

  RushConfig config_a, config_b;
  config_a.mtu = 128;
  config_a.port = 4444;
  config_a.callbacks = callbacks;
  config_a.data = static_cast<void *>(&info_a);
  config_a.hostname = 0;

  config_b.mtu = 128;
  config_b.port = 5555;
  config_b.callbacks = callbacks;
  config_b.data = static_cast<void *>(&info_b);
  config_b.hostname = 0;

  Base::Url url_a(Base::AddressIPv4::kLocalhost, config_a.port),
      url_b(Base::AddressIPv4::kLocalhost, config_b.port);

  rush_t rush_a = rush_create(&config_a);
  printf("rush_a at %p\n", rush_a);
  ASSERT_TRUE(rush_a != 0);

  rush_t rush_b = rush_create(&config_b);
  printf("rush_b at %p\n", rush_b);
  ASSERT_TRUE(rush_b != 0);

  endpoint_t endpoint = nullptr; // rush_open(rush_a, url_b);
  ASSERT_TRUE(endpoint); // need to have nat resolved.

  ASSERT_TRUE(info_a.NumConnections() == 0);
  ASSERT_TRUE(info_b.NumConnections() == 0);

  rush_time_t delta = 1.f; // small delta to overcome frame processing latency.
  rush_time_t runtime = 1000.f;

  ASSERT_NO_FATAL_FAILURE({
    for(rush_time_t t = 0.f; t < runtime; t += delta) {
      rush_update(rush_a, delta);
      rush_update(rush_b, delta);
    }
  });

  info_a.Print();
  info_b.Print();

  rush_destroy(rush_a);
  rush_destroy(rush_b);
}
