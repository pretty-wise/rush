/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "gtest/gtest.h"

#include "base/network/socket.h"
#include "base/network/url.h"
#include "base/threading/thread.h"
#include "base/core/time_utils.h"

#include "../src/packet.h"
#include "../src/rtt.h"
#include "../src/ack_info.h"

//
// globals
//
int error;
const u16 data_size = sizeof(Rush::PacketHeader) + 1;
char data[data_size];
Rush::PacketHeader *header = reinterpret_cast<Rush::PacketHeader *>(data);

void Create(Base::Socket::Handle *sock, u16 port) {
  Base::Url url(Base::AddressIPv4::kLocalhost, port);
  *sock = Base::Socket::Udp::Open(&port);
  ASSERT_TRUE(*sock != Base::Socket::InvalidHandle);
}

// IMPORTANT(kstasik): we have to receive before sending to send most up-to-date
// state!
void ReceiveAndSend(Rush::AckInfo &ack_from, Base::Socket::Handle socket_from,
                    const Base::Url &url_from, Rush::RttInfo &rtt_from,
                    const Base::Url &url_to, rush_time_t session_time) {
  //
  // a <- b
  //
  Base::Url from;
  streamsize received =
      Base::Socket::Udp::Recv(socket_from, &from, data, data_size, &error);
  if(received != 0) {
    ASSERT_TRUE(received != -1);
    ASSERT_TRUE(from == url_to);

    ack_from.Received(*header, session_time);
    // this is done in a callback
    // rtt_from.PacketReceived(header->ack,
    // session_time);
  }

  //
  // a -> b
  //
  ack_from.CreateHeader(header);

  bool sent =
      Base::Socket::Udp::Send(socket_from, url_to, data, data_size, &error);
  ASSERT_TRUE(sent);

  rtt_from.PacketSent(header->id, session_time);
}

void OnDelivered(rush_sequence_t id, rush_time_t time, rush_t ctx, endpoint_t data) {
  Rush::RttInfo *info = reinterpret_cast<Rush::RttInfo *>(data);
  info->PacketReceived(id, time);
}

void OnLost(rush_sequence_t id, rush_time_t time, rush_t ctx, endpoint_t data) {}

TEST(Rtt, SameDelta) {
  Base::Url url_a(Base::AddressIPv4::kLocalhost, 4444),
      url_b(Base::AddressIPv4::kLocalhost, 5555);
  Base::Socket::Handle socket_a, socket_b;
  ASSERT_NO_FATAL_FAILURE(Create(&socket_a, url_a.GetPort()));
  ASSERT_NO_FATAL_FAILURE(Create(&socket_b, url_b.GetPort()));

  const rush_time_t delta_a = 16, delta_b = 16;

  Rush::DeliveryCallbacks callbacks = {&OnDelivered, &OnLost};

  Rush::RttInfo rtt_a(0), rtt_b(0);
  Rush::AckInfo ack_a(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_a)),
      ack_b(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_b));

  rush_time_t time_a = 0, time_b = 1000; // different session time on host a and b.
  rush_time_t run_time = 5 * 1000;
  while(time_a < run_time) {
    time_a += delta_a;
    time_b += delta_b;

    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_a, socket_a, url_a, rtt_a, url_b, time_a); });
    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_b, socket_b, url_b, rtt_b, url_a, time_b); });

    printf("averate rtt: %fms %fms\n", rtt_a.GetAverage(), rtt_b.GetAverage());
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}

TEST(Rtt, StartRtt) {
  Base::Url url_a(Base::AddressIPv4::kLocalhost, 4444),
      url_b(Base::AddressIPv4::kLocalhost, 5555);
  Base::Socket::Handle socket_a, socket_b;
  ASSERT_NO_FATAL_FAILURE(Create(&socket_a, url_a.GetPort()));
  ASSERT_NO_FATAL_FAILURE(Create(&socket_b, url_b.GetPort()));

  const rush_time_t delta_a = 16, delta_b = 16;

  Rush::DeliveryCallbacks callbacks = {&OnDelivered, &OnLost};

  Rush::RttInfo rtt_a(delta_a), rtt_b(delta_b);
  Rush::AckInfo ack_a(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_a)),
      ack_b(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_b));

  rush_time_t time_a = 0, time_b = 1000; // different session time on host a and b.
  rush_time_t run_time = 5 * 1000;
  while(time_a < run_time) {
    time_a += delta_a;
    time_b += delta_b;

    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_a, socket_a, url_a, rtt_a, url_b, time_a); });
    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_b, socket_b, url_b, rtt_b, url_a, time_b); });

    printf("averate rtt: %fms %fms\n", rtt_a.GetAverage(), rtt_b.GetAverage());
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}

TEST(Rtt, Loopback) {
  Base::Url url_a(Base::AddressIPv4::kLocalhost, 4444);
  Base::Socket::Handle socket_a;
  ASSERT_NO_FATAL_FAILURE(Create(&socket_a, url_a.GetPort()));

  const rush_time_t delta = 16;

  Rush::DeliveryCallbacks callbacks = {&OnDelivered, &OnLost};

  Rush::RttInfo rtt_a(0);
  Rush::AckInfo ack_a(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_a));

  rush_time_t time_a = 0;
  rush_time_t run_time = 5 * 1000;
  while(time_a < run_time) {
    time_a += delta;

    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_a, socket_a, url_a, rtt_a, url_a, time_a); });

    printf("averate loopback rtt: %fms\n", rtt_a.GetAverage());
  }

  Base::Socket::Close(socket_a);
}

TEST(Rtt, DoubleDelta) {
  Base::Url url_a(Base::AddressIPv4::kLocalhost, 4444),
      url_b(Base::AddressIPv4::kLocalhost, 5555);
  Base::Socket::Handle socket_a, socket_b;
  ASSERT_NO_FATAL_FAILURE(Create(&socket_a, url_a.GetPort()));
  ASSERT_NO_FATAL_FAILURE(Create(&socket_b, url_b.GetPort()));

  const rush_time_t delta_a = 32, delta_b = 16;

  Rush::DeliveryCallbacks callbacks = {&OnDelivered, &OnLost};

  Rush::RttInfo rtt_a(0), rtt_b(0);
  Rush::AckInfo ack_a(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_a)),
      ack_b(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_b));

  rush_time_t time_a = 0, time_b = 1000; // different session time on host a and b.
  rush_time_t run_time = 5 * 1000;
  while(time_a < run_time) {
    time_a += delta_a;
    time_b += delta_b;

    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_a, socket_a, url_a, rtt_a, url_b, time_a); });
    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_b, socket_b, url_b, rtt_b, url_a, time_b); });

    printf("averate rtt: %fms %fms\n", rtt_a.GetAverage(), rtt_b.GetAverage());
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}

TEST(Rtt, RealDelta) {
  Base::Url url_a(Base::AddressIPv4::kLocalhost, 4444),
      url_b(Base::AddressIPv4::kLocalhost, 5555);
  Base::Socket::Handle socket_a, socket_b;
  ASSERT_NO_FATAL_FAILURE(Create(&socket_a, url_a.GetPort()));
  ASSERT_NO_FATAL_FAILURE(Create(&socket_b, url_b.GetPort()));

  Rush::DeliveryCallbacks callbacks = {&OnDelivered, &OnLost};

  Rush::RttInfo rtt_a(0), rtt_b(0);
  Rush::AckInfo ack_a(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_a)),
      ack_b(callbacks, nullptr, reinterpret_cast<endpoint_t>(&rtt_b));

  rush_time_t time_a = 0, time_b = 1000; // different session time on host a and b.
  rush_time_t run_time = 1000;

  rush_time_t last_time = Base::Time::GetTimeMs();
  while(time_a < run_time) {
    Base::Thread::Sleep(16);
    rush_time_t cur_time = Base::Time::GetTimeMs();
    rush_time_t delta = cur_time - last_time;
    printf("cur %f, last %f, delta %f\n", cur_time, last_time, delta);
    last_time = cur_time;

    time_a += delta;
    time_b += delta;

    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_a, socket_a, url_a, rtt_a, url_b, time_a); });
    ASSERT_NO_FATAL_FAILURE(
        { ReceiveAndSend(ack_b, socket_b, url_b, rtt_b, url_a, time_b); });

    printf("averate rtt: %fms %fms\n", rtt_a.GetAverage(), rtt_b.GetAverage());
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}
