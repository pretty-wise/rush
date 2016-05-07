/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "gtest/gtest.h"

#include "base/network/socket.h"
#include "base/network/url.h"

#include "rush/rush.h"
#include "rush/utils.h"

#include <list>
#include <algorithm>

static const u16 kMTU = 128;
static const u16 kEphemeralPort = 0;

struct TestInfo {
  std::list<endpoint_t> connected;

  bool IsConnected(endpoint_t h) const {
    return std::find(connected.begin(), connected.end(), h) != connected.end();
  }
  u32 NumConnections() const { return connected.size(); }
};

static void Connectivity(rush_t ctx, endpoint_t endpoint, Connectivity event,
                         void *data) {
  TestInfo *info = static_cast<TestInfo *>(data);
  printf("connectivity context %p endpoint %p : %s\n", ctx, endpoint,
         ConnectivityToString(event));

  if(info) {
    if(event == kRushConnected || event == kRushEstablished) {
      ASSERT_TRUE(std::find(info->connected.begin(), info->connected.end(),
                            endpoint) == info->connected.end());
      info->connected.push_back(endpoint);
    } else if(event == kRushDisconnected) {
      ASSERT_TRUE(std::find(info->connected.begin(), info->connected.end(),
                            endpoint) != info->connected.end());
      info->connected.erase(
          std::find(info->connected.begin(), info->connected.end(), endpoint));
    }
  }
}

static RushCallbacks callbacks = {nullptr, &Connectivity, 0, 0};

TEST(RushApi, Context) {
  RushConfig config_a;
  config_a.mtu = kMTU;
  config_a.port = kEphemeralPort;
  config_a.callbacks = callbacks;
  config_a.data = nullptr;
  config_a.hostname = 0;

  rush_t rush_a = rush_create(&config_a);
  ASSERT_TRUE(rush_a != 0);

  rush_destroy(rush_a);
}
/* rush_open()
TEST(RushApi, Loopback) {
  TestInfo info;
  RushConfig config_a;
  config_a.mtu = kMTU;
  config_a.port = kEphemeralPort;
  config_a.callbacks = callbacks;
  config_a.data = nullptr;
  config_a.hostname = 0;

  rush_t rush_a = rush_create(&config_a);
  ASSERT_TRUE(rush_a != 0);

  Base::Url url_a(Base::AddressIPv4::kLocalhost, config_a.port);

  endpoint_t loopback = rush_open(rush_a, url_a);
  ASSERT_TRUE(loopback != 0);

  rush_time_t delta = 16.f;

  // it takes two updates to send data to self.
  // first update does the send
  // second update does receive and send back.
  ASSERT_NO_FATAL_FAILURE({ rush_update(rush_a, delta); });
  ASSERT_NO_FATAL_FAILURE({ rush_update(rush_a, delta); });

  ASSERT_TRUE(info.IsConnected(loopback) == true);

  rush_close(rush_a, loopback);

  ASSERT_TRUE(info.IsConnected(loopback) == false);

  rush_destroy(rush_a);
}

TEST(RushApi, Instances) {
  TestInfo info_a, info_b;
  RushConfig config_a;
  config_a.mtu = kMTU;
  config_a.port = kEphemeralPort;
  config_a.callbacks = callbacks;
  config_a.data = static_cast<void *>(&info_a);
  config_a.hostname = 0;

  RushConfig config_b;
  config_b.mtu = kMTU;
  config_b.port = kEphemeralPort;
  config_b.callbacks = callbacks;
  config_b.data = static_cast<void *>(&info_a);
  config_b.hostname = 0;

  rush_t rush_a = rush_create(&config_a);
  ASSERT_TRUE(rush_a != 0);

  rush_t rush_b = rush_create(&config_b);
  ASSERT_TRUE(rush_b != 0);

  rush_time_t delta = 16.f;
  for(u32 i = 0; i < 10; ++i) {
    ASSERT_NO_FATAL_FAILURE({
      rush_update(rush_a, delta);
      rush_update(rush_b, delta);
    });
  }

  rush_destroy(rush_a);
  rush_destroy(rush_b);
}

TEST(RushApi, Connect) {
  TestInfo info_a, info_b;

  RushConfig config_a;
  config_a.mtu = kMTU;
  config_a.port = kEphemeralPort;
  config_a.callbacks = callbacks;
  config_a.data = static_cast<void *>(&info_a);
  config_a.hostname = 0;

  RushConfig config_b;
  config_b.mtu = kMTU;
  config_b.port = kEphemeralPort;
  config_b.callbacks = callbacks;
  config_b.data = static_cast<void *>(&info_a);
  config_b.hostname = 0;

  rush_t rush_a = rush_create(&config_a);
  printf("rush_a at %p\n", rush_a);
  ASSERT_TRUE(rush_a != 0);

  rush_t rush_b = rush_create(&config_b);
  printf("rush_b at %p\n", rush_b);
  ASSERT_TRUE(rush_b != 0);

  Base::Url url_b(Base::AddressIPv4::kLocalhost, config_b.port);

  endpoint_t endpoint = rush_open(rush_a, url_b);
  ASSERT_TRUE(endpoint);

  ASSERT_TRUE(info_a.NumConnections() == 0);
  ASSERT_TRUE(info_b.NumConnections() == 0);

  rush_time_t delta = 16.f;

  const u16 kLoopCount = 16;
  for(u16 i = 0; i < kLoopCount; ++i) {
    ASSERT_NO_FATAL_FAILURE({
      rush_update(rush_a, delta);
      rush_update(rush_b, delta);
    });
  }

  ASSERT_TRUE(info_a.NumConnections() == 1);
  ASSERT_TRUE(info_a.IsConnected(endpoint) == true);

  ASSERT_TRUE(info_b.NumConnections() == 1);

  rush_close(rush_a, endpoint);
  ASSERT_TRUE(info_a.NumConnections() == 0);
  ASSERT_TRUE(info_b.NumConnections() == 1);

  // update to timeout rush_b connection.
  // do not update rush_a, because it would reconnect (as implemented now).
  rush_time_t acc = 0.f;
  while(acc < 2 * kRushConnectionTimeout) {
    rush_update(rush_b, delta);
    acc += delta;
  }

  ASSERT_TRUE(info_b.NumConnections() == 0);

  rush_destroy(rush_a);
  rush_destroy(rush_b);
}

TEST(RushApi, ConnectionTimeout) {
  TestInfo info_a, info_b;
  RushConfig config_a;
  config_a.mtu = kMTU;
  config_a.port = kEphemeralPort;
  config_a.callbacks = callbacks;
  config_a.data = static_cast<void *>(&info_a);
  config_a.hostname = 0;

  RushConfig config_b;
  config_b.mtu = kMTU;
  config_b.port = kEphemeralPort;
  config_b.callbacks = callbacks;
  config_b.data = static_cast<void *>(&info_a);
  config_b.hostname = 0;

  rush_t rush_a = rush_create(&config_a);
  printf("rush_a at %p\n", rush_a);
  ASSERT_TRUE(rush_a != 0);

  rush_t rush_b = rush_create(&config_b);
  printf("rush_b at %p\n", rush_b);
  ASSERT_TRUE(rush_b != 0);

  Base::Url url_b(Base::AddressIPv4::kLocalhost, config_b.port);

  endpoint_t endpoint = rush_open(rush_a, url_b);
  ASSERT_TRUE(endpoint);

  ASSERT_TRUE(info_a.NumConnections() == 0);
  ASSERT_TRUE(info_b.NumConnections() == 0);

  rush_time_t delta = 16.f;

  // update only a to timeout the connection.
  ASSERT_NO_FATAL_FAILURE({ rush_update(rush_a, delta); });

  ASSERT_TRUE(info_a.NumConnections() == 0);
  ASSERT_TRUE(info_a.IsConnected(endpoint) == false);

  rush_close(rush_a, endpoint);
  ASSERT_TRUE(info_b.NumConnections() == 0);

  rush_destroy(rush_a);
  rush_destroy(rush_b);
}*/
