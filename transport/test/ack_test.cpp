/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "gtest/gtest.h"

#include "base/network/socket.h"
#include "base/network/url.h"

#include "../src/ack_info.h"
#include "../src/packet.h"

TEST(Ack, HeaderCreation) {
  u16 port_a = 4444, port_b = 5555;
  Base::Url url_a(Base::AddressIPv4::kLocalhost, port_a),
      url_b(Base::AddressIPv4::kLocalhost, port_b);

  Base::Socket::Handle socket_a = Base::Socket::Udp::Open(&port_a);
  ASSERT_TRUE(socket_a != Base::Socket::InvalidHandle);

  Base::Socket::Handle socket_b = Base::Socket::Udp::Open(&port_b);
  ASSERT_TRUE(socket_b != Base::Socket::InvalidHandle);

  Rush::AckInfo ack_a, ack_b;

  int error;

  const u16 data_size = sizeof(Rush::PacketHeader) + 1;
  char out_data[data_size], in_data[data_size];

  u16 send_counter = 0, recv_counter = 0;
  u16 transmission_count =
      Rush::AckInfo::kAckCount -
      1; // -1 because AckInfo::ack is considered an acked packet.

  while(send_counter < transmission_count &&
        recv_counter < transmission_count) {
    if(send_counter < transmission_count) {
      Rush::PacketHeader *header =
          reinterpret_cast<Rush::PacketHeader *>(out_data);
      ack_a.CreateHeader(header);

      ASSERT_TRUE(header->id == send_counter);
      ASSERT_TRUE(header->protocol == Rush::kGameProtocol);
      ASSERT_TRUE(header->ack == Rush::AckInfo::kInitialSequenceId - 1);
      ASSERT_TRUE(header->ack_bitfield == 0);

      bool sent =
          Base::Socket::Udp::Send(socket_a, url_b, out_data, data_size, &error);
      ASSERT_TRUE(sent);

      send_counter++;
    }

    Base::Url from;
    streamsize received =
        Base::Socket::Udp::Recv(socket_b, &from, in_data, data_size, &error);
    if(received != 0) {
      ASSERT_TRUE(received != -1);
      ASSERT_TRUE(received == data_size);
      ASSERT_TRUE(from == url_a);

      Rush::PacketHeader *recv_header =
          reinterpret_cast<Rush::PacketHeader *>(in_data);
      ack_b.Received(*recv_header,
                     0 /* zero session time, because irrelevant here */);

      ASSERT_TRUE(recv_header->id == recv_counter);
      ASSERT_TRUE(recv_header->protocol == Rush::kGameProtocol);
      ASSERT_TRUE(recv_header->ack == Rush::AckInfo::kInitialSequenceId - 1);
      ASSERT_TRUE(recv_header->ack_bitfield == 0);

      recv_counter++;
    }
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}

static const u16 transmission_count = Rush::AckInfo::kAckCount - 1;
static bool transmission_acks_a[transmission_count],
    transmission_acks_b[transmission_count];

void PacketDelivered(rush_sequence_t id, rush_time_t, rush_t ctx, endpoint_t data) {
  ASSERT_TRUE(data);
  ASSERT_TRUE(id < transmission_count);
  ASSERT_TRUE(id >= 0);

  bool *acks = reinterpret_cast<bool *>(data);

  for(rush_sequence_t i = 0; i < id; ++i) {
    ASSERT_TRUE(acks[i] == true); // previous packets need to be acked.
  }

  ASSERT_TRUE(acks[id] == false); // current packet needs to be not acked.
  acks[id] = true;
}

void PacketLost(rush_sequence_t id, rush_time_t, rush_t ctx, endpoint_t endpoint) {
  ASSERT_TRUE(id < transmission_count);
  ASSERT_TRUE(id >= 0);
  ASSERT_TRUE(false); // we should ont lose any packets.
}

TEST(Ack, Acknowledgement) {
  u16 port_a = 4444, port_b = 5555;
  Base::Url url_a(Base::AddressIPv4::kLocalhost, port_a),
      url_b(Base::AddressIPv4::kLocalhost, port_b);

  Base::Socket::Handle socket_a = Base::Socket::Udp::Open(&port_a);
  ASSERT_TRUE(socket_a != Base::Socket::InvalidHandle);

  Base::Socket::Handle socket_b = Base::Socket::Udp::Open(&port_b);
  ASSERT_TRUE(socket_b != Base::Socket::InvalidHandle);

  Rush::DeliveryCallbacks cbs = {&PacketDelivered, &PacketLost};

  Rush::AckInfo ack_a(cbs, nullptr,
                      reinterpret_cast<endpoint_t>(transmission_acks_a)),
      ack_b(cbs, nullptr, reinterpret_cast<endpoint_t>(transmission_acks_b));

  int error;

  const u16 data_size = sizeof(Rush::PacketHeader) + 1;
  char out_data[data_size], in_data[data_size];

  u16 send_counter_a = 0, recv_counter_a = 0;
  u16 send_counter_b = 0, recv_counter_b = 0;
  rush_sequence_t last_received_a = -1, last_received_b = -1;

  // init transmission acks.
  for(u16 i = 0; i < transmission_count; ++i) {
    transmission_acks_a[i] = false;
    transmission_acks_b[i] = false;
  }

  while(send_counter_a < transmission_count &&
        recv_counter_a < transmission_count &&
        send_counter_b < transmission_count &&
        recv_counter_b < transmission_count) {
    Rush::PacketHeader *header =
        reinterpret_cast<Rush::PacketHeader *>(out_data);
    bool sent;

    //
    // send a -> b
    //
    ack_a.CreateHeader(header);

    // printf("a created %d, ack %d, ", header->id, header->ack);
    // for(u16 i = 0; i < Rush::AckInfo::kAckCount-1; ++i) printf("%d",
    // header->ack_bitfield & ( 1 << i ) ? 1 : 0);
    // printf(" recv_counter_b %d, last_received_b %d\n", recv_counter_b,
    // last_received_b);

    ASSERT_TRUE(header->id == send_counter_a);
    ASSERT_TRUE(header->protocol == Rush::kGameProtocol);
    ASSERT_TRUE(!Rush::IsMoreRecent(
        header->ack,
        last_received_b)); // ack is an id of last received packet from b.

    sent =
        Base::Socket::Udp::Send(socket_a, url_b, out_data, data_size, &error);
    ASSERT_TRUE(sent);

    send_counter_a++;
    //
    // send b -> a
    //
    ack_b.CreateHeader(header);

    // printf("b created %d, ack %d, ", header->id, header->ack);
    // for(u16 i = 0; i < Rush::AckInfo::kAckCount-1; ++i) printf("%d",
    // header->ack_bitfield & ( 1 << i ) ? 1 : 0);
    // printf(" recv_counter_a %d, last_received_a %d\n", recv_counter_a,
    // last_received_a);

    ASSERT_TRUE(header->id == send_counter_b);
    ASSERT_TRUE(header->protocol == Rush::kGameProtocol);
    ASSERT_TRUE(!Rush::IsMoreRecent(
        header->ack,
        last_received_a)); // ack is an id of last received packet from a.

    sent =
        Base::Socket::Udp::Send(socket_b, url_a, out_data, data_size, &error);
    ASSERT_TRUE(sent);

    send_counter_b++;

    Base::Url from;
    streamsize received = 0;

    //
    // recv a <- b
    //
    received =
        Base::Socket::Udp::Recv(socket_a, &from, in_data, data_size, &error);
    if(received != 0) {
      ASSERT_TRUE(received != -1);
      ASSERT_TRUE(received == data_size);
      ASSERT_TRUE(from == url_b);

      Rush::PacketHeader *recv_header =
          reinterpret_cast<Rush::PacketHeader *>(in_data);
      ack_a.Received(*recv_header,
                     0 /* time zero, because irrelevant for this test */);

      // printf("a received %d, ack %d, recv_counter_a %d\n", recv_header->id,
      // recv_header->ack, recv_counter_a);

      last_received_b = recv_header->id;

      ASSERT_TRUE(recv_header->id == recv_counter_a);
      ASSERT_TRUE(recv_header->protocol == Rush::kGameProtocol);
      ASSERT_TRUE(Rush::IsMoreRecent(
          send_counter_a, recv_header->ack)); // ack is an id of last known a
                                              // packet that arrived at the
                                              // endpoint b.
      recv_counter_a++;
    }

    //
    // recv b <- a
    //
    received =
        Base::Socket::Udp::Recv(socket_b, &from, in_data, data_size, &error);
    if(received != 0) {
      ASSERT_TRUE(received != -1);
      ASSERT_TRUE(received == data_size);
      ASSERT_TRUE(from == url_a);

      Rush::PacketHeader *recv_header =
          reinterpret_cast<Rush::PacketHeader *>(in_data);
      ack_b.Received(*recv_header, 0 /* time irrelevant for this test */);

      // printf("b received %d, ack %d, recv_counter_b %d\n", recv_header->id,
      // recv_header->ack, recv_counter_b);

      last_received_a = recv_header->id;

      ASSERT_TRUE(recv_header->id == recv_counter_b);
      ASSERT_TRUE(recv_header->protocol == Rush::kGameProtocol);
      ASSERT_TRUE(Rush::IsMoreRecent(
          send_counter_a, recv_header->ack)); // ack is an if of last known b
                                              // packet that arrived at the
                                              // endpoint a.
      recv_counter_b++;
    }
  }

  Base::Socket::Close(socket_a);
  Base::Socket::Close(socket_b);
}

static const u8 kUnset = 0;
static const u8 kDelivered = 1;
static const u8 kLost = 2;
static const u32 kHistorySize = 32;

void HolesDelivered(rush_sequence_t id, rush_time_t, rush_t ctx, endpoint_t data) {
  ASSERT_TRUE(id >= 0);
  ASSERT_TRUE(id < kHistorySize);

  u8 *history = reinterpret_cast<u8 *>(data);
  ASSERT_TRUE(history[id] == kUnset);
  history[id] = kDelivered;
}

void HolesLost(rush_sequence_t id, rush_time_t, rush_t ctx, endpoint_t data) {
  ASSERT_TRUE(id >= 0);
  ASSERT_TRUE(id < kHistorySize);

  u8 *history = reinterpret_cast<u8 *>(data);
  ASSERT_TRUE(history[id] == kUnset);
  history[id] = kLost;
}

TEST(Ack, Holes) {

  Rush::DeliveryCallbacks cbs = {&HolesDelivered, &HolesLost};

  u8 history_a[kHistorySize], history_b[kHistorySize];

  for(u32 i = 0; i < kHistorySize; ++i) {
    history_a[i] = kUnset;
    history_b[i] = kUnset;
  }

  Rush::AckInfo ack_a(cbs, nullptr, reinterpret_cast<endpoint_t>(history_a)),
      ack_b(cbs, nullptr, reinterpret_cast<endpoint_t>(history_b));

  const u16 data_size = sizeof(Rush::PacketHeader) + 1;
  char out_data[data_size];

  Rush::PacketHeader *header = reinterpret_cast<Rush::PacketHeader *>(out_data);

  // a -> b (a: id 0, ack -1, b: id -1, ack -1)
  ack_a.CreateHeader(header);
  ack_b.Received(*header, 0 /* time irrelevant for this test */);

  ASSERT_TRUE(history_a[0] == kUnset);
  ASSERT_TRUE(history_b[0] == kUnset);

  // b -> a (a: id 0, ack 0, b: id 0, ack -1)
  ack_b.CreateHeader(header);
  ack_a.Received(*header, 0 /* time irrelevant for this test */);

  ASSERT_TRUE(history_a[0] == kDelivered);
  ASSERT_TRUE(history_a[1] == kUnset);
  ASSERT_TRUE(history_b[0] == kUnset);

  // a -> b (a: id 1, ack 0, b: id 0, ack 0)
  ack_a.CreateHeader(header);
  ack_b.Received(*header, 0 /* time irrelevant for this test */);

  ASSERT_TRUE(history_a[0] == kDelivered);
  ASSERT_TRUE(history_a[1] == kUnset);
  ASSERT_TRUE(history_b[0] == kDelivered);
  ASSERT_TRUE(history_b[1] == kUnset);

  // two packet a -> b. one lost.
  // (a: id 3, ack 0, b: id 0, ack 0)
  ack_a.CreateHeader(header);
  ack_a.CreateHeader(header);
  ack_b.Received(*header, 0 /* time irrelevant for this test */);

  ASSERT_TRUE(history_b[1] == kUnset);
  ASSERT_TRUE(history_a[1] == kUnset);

  // b -> a (a: 3, ack 3, b: id 1, ack 0)
  ack_b.CreateHeader(header);
  ack_a.Received(*header, 0 /* time irrelevant for this test */);

  ASSERT_TRUE(history_a[1] == kDelivered);
  ASSERT_TRUE(history_a[2] == kLost);
  ASSERT_TRUE(history_a[3] == kDelivered);
  ASSERT_TRUE(history_a[4] == kUnset);
  ASSERT_TRUE(history_b[1] == kUnset);
}

TEST(Ack, BigHole) {

  Rush::DeliveryCallbacks cbs = {&HolesDelivered, &HolesLost};

  u8 history_a[kHistorySize], history_b[kHistorySize];

  for(u32 i = 0; i < kHistorySize; ++i) {
    history_a[i] = kUnset;
    history_b[i] = kUnset;
  }

  Rush::AckInfo ack_a(cbs, nullptr, reinterpret_cast<endpoint_t>(history_a)),
      ack_b(cbs, nullptr, reinterpret_cast<endpoint_t>(history_b));

  const u16 data_size = sizeof(Rush::PacketHeader) + 1;
  char out_data[data_size];

  Rush::PacketHeader *header = reinterpret_cast<Rush::PacketHeader *>(out_data);

  for(u32 i = 0; i < Rush::AckInfo::kAckCount - 1; ++i) {
    ack_a.CreateHeader(header);
  }

  ack_b.Received(*header, 0 /* time irrelevant for this test */);

  ack_b.CreateHeader(header);

  ASSERT_TRUE(history_a[0] == kUnset);

  ack_a.Received(*header, 0 /* time irrelevant for this test */);

  for(u32 i = 0; i < Rush::AckInfo::kAckCount - 1 - 1; ++i) {
    ASSERT_TRUE(history_a[i] == kLost);
  }
  ASSERT_TRUE(history_a[Rush::AckInfo::kAckCount - 1 - 1] == kDelivered);
}

struct Counter {
  u32 lost;
  u32 delivered;
};

void DeliveredBitmaskTest(rush_sequence_t id, rush_time_t, rush_t ctx,
                          endpoint_t data) {
  Counter *counter = reinterpret_cast<Counter *>(data);
  counter->delivered++;
}

void LostBitmaskTest(rush_sequence_t id, rush_time_t, rush_t ctx, endpoint_t data) {
  Counter *counter = reinterpret_cast<Counter *>(data);
  counter->lost++;
}

TEST(Ack, BitmaskOverflow) {
  Rush::DeliveryCallbacks cbs = {&DeliveredBitmaskTest, &LostBitmaskTest};

  Counter counter_a = {};
  Counter counter_b = {};

  Rush::AckInfo ack_a(cbs, nullptr, reinterpret_cast<endpoint_t>(&counter_a)),
      ack_b(cbs, nullptr, reinterpret_cast<endpoint_t>(&counter_b));

  Rush::PacketHeader header;
  const u32 kSendCount = 100;

  for(u32 i = 0; i < kSendCount; ++i) {
    ack_a.CreateHeader(&header);
    ack_b.Received(header, 0);
  }

  ack_b.CreateHeader(&header);
  ack_a.Received(header, 0);

  EXPECT_EQ(33, counter_a.delivered);
  EXPECT_EQ(kSendCount - 33, counter_a.lost);
  EXPECT_EQ(kSendCount, counter_a.delivered + counter_a.lost);

  EXPECT_EQ(0, counter_b.delivered);
  EXPECT_EQ(0, counter_b.lost);
}
