/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "rush/rush.h"

struct rush_endpoint;

namespace Rush {

struct PacketHeader;

struct DeliveryCallbacks {
	void (*on_delivered)(rush_sequence_t id, rush_time_t time, rush_t ctx, endpoint_t endpoint);
	void (*on_lost)(rush_sequence_t id, rush_time_t time, rush_t ctx, endpoint_t endpoint);
};

struct AckInfo {
	typedef u32 bitmask_t;
	static const rush_sequence_t kInitialSequenceId = 0;
	static const rush_sequence_t kInitialRemoteSequenceId = kInitialSequenceId - 1;
	static const rush_sequence_t kAckCount = sizeof(bitmask_t)*8 + 1;
	rush_sequence_t next_send_id;
	rush_sequence_t last_acked;
	rush_sequence_t remote_id;
	bitmask_t remote_id_bitfield;
	DeliveryCallbacks callbacks;
	rush_t ctx;
	endpoint_t endpoint;

	AckInfo();
	AckInfo(DeliveryCallbacks cbs, rush_t _ctx, endpoint_t _endpoint);
	void CreateHeader(PacketHeader* header);
	bool InOrder(const PacketHeader& header);
	void Received(const PacketHeader& header, rush_time_t time);
};

} // namespace Rush
