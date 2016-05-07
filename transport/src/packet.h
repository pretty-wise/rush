/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "base/core/types.h"
#include "rush/rush.h"

#include <limits>

namespace Rush {

typedef u32 Protocol;

static const Protocol kGameProtocol = 0xdeaddead;
static const Protocol kPunchProtocol = 0x01010101;

inline bool IsMoreRecent(rush_sequence_t a, rush_sequence_t b) {
	return ( (a > b) &&	(a - b <= std::numeric_limits<rush_sequence_t>::max()/2) ) ||
		( (b > a) &&	(b - a > std::numeric_limits<rush_sequence_t>::max()/2) );
}

inline rush_sequence_t Diff(rush_sequence_t a, rush_sequence_t b) {
	//return a - b;
	return (a >= b) ? a - b : (std::numeric_limits<rush_sequence_t>::max() - b) + a + 1;
}

struct PacketHeader{
	Protocol protocol;
	rush_sequence_t id;
	rush_sequence_t ack;
	u32 ack_bitfield;

	// todo(kstasik): endianness.
	void hton();
	void ntoh();
};

} // namespace Rush
