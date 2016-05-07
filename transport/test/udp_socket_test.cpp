/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "gtest/gtest.h"

#include "base/network/socket.h"
#include "base/network/url.h"

TEST(UDP_Socket, Creation) {
	u16 port = 5555;
	Base::Socket::Handle socket = Base::Socket::Udp::Open(&port);
	EXPECT_TRUE(socket != Base::Socket::InvalidHandle);
	Base::Socket::Close(socket);
}

TEST(UDP_Socket, Transmission) {
	u16 port_a = 4444, port_b = 5555;
	const char data[] = "test_data";
	const streamsize data_size = sizeof(data);

	Base::Url url_a(Base::AddressIPv4::kLocalhost, port_a), url_b(Base::AddressIPv4::kLocalhost, port_b);	

	Base::Socket::Handle socket_a = Base::Socket::Udp::Open(&port_a);
	EXPECT_TRUE(socket_a != Base::Socket::InvalidHandle);

	Base::Socket::Handle socket_b = Base::Socket::Udp::Open(&port_b);
	EXPECT_TRUE(socket_b != Base::Socket::InvalidHandle);

	int error;
	bool sent = Base::Socket::Udp::Send(socket_a, url_b, data, data_size, &error);
	EXPECT_TRUE(sent);

	Base::Url from;
	char recv_data[data_size];
	streamsize received = 0;
	while(received == 0) {
		received = Base::Socket::Udp::Recv(socket_b, &from, recv_data, data_size, &error);
	}
	EXPECT_TRUE(received == data_size);
	EXPECT_TRUE(memcmp(recv_data, data, data_size) == 0);
	EXPECT_TRUE(from == url_a);
	
	Base::Socket::Close(socket_a);
	Base::Socket::Close(socket_b);
}
