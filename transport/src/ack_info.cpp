/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "ack_info.h"
#include "packet.h"

#include "base/core/assert.h"

namespace Rush {

AckInfo::AckInfo()
    : next_send_id(kInitialSequenceId), last_acked(kInitialRemoteSequenceId),
      remote_id(kInitialRemoteSequenceId), remote_id_bitfield(0), ctx(nullptr),
      endpoint(nullptr) {
  callbacks = {};
}

AckInfo::AckInfo(DeliveryCallbacks cbs, rush_t _ctx, endpoint_t _endpoint)
    : next_send_id(kInitialSequenceId), last_acked(kInitialRemoteSequenceId),
      remote_id(kInitialRemoteSequenceId), remote_id_bitfield(0),
      callbacks(cbs), ctx(_ctx), endpoint(_endpoint) {}

void AckInfo::CreateHeader(PacketHeader *header) {
  header->protocol = kGameProtocol;
  header->id = next_send_id++;
  header->ack = remote_id;
  header->ack_bitfield = remote_id_bitfield;
}

bool AckInfo::InOrder(const PacketHeader &header) {
  return IsMoreRecent(header.id, remote_id);
}

void AckInfo::Received(const PacketHeader &header, rush_time_t time) {
  BASE_ASSERT(InOrder(header));

  // have to ack packets <ack+1 ; header.ack>
  for(rush_sequence_t i = last_acked + 1; !IsMoreRecent(i, header.ack); ++i) {
    rush_sequence_t diff = Diff(header.ack, i);
    if(diff == 0) {
      if(callbacks.on_delivered)
        callbacks.on_delivered(i, time, ctx, endpoint);
    } else if(diff > sizeof(bitmask_t) * 8) {
      // out of bitmask. might have been delivered!
      if(callbacks.on_lost)
        callbacks.on_lost(i, time, ctx, endpoint);
    } else {
      BASE_ASSERT(diff < kAckCount, "diff: %d/%d. last acked: %d, header: %d",
                  diff, kAckCount, last_acked, header.ack);
      bool acked = header.ack_bitfield & (1 << (diff - 1));
      if(acked) {
        if(callbacks.on_delivered)
          callbacks.on_delivered(i, time, ctx, endpoint);
      } else {
        if(callbacks.on_lost)
          callbacks.on_lost(i, time, ctx, endpoint);
      }
    }
  }
  last_acked = header.ack;

  // update ack data.
  rush_sequence_t packet_hole = Diff(header.id, remote_id);
  remote_id_bitfield = remote_id_bitfield << packet_hole;
  remote_id_bitfield |= (1 << (packet_hole - 1));
  remote_id = header.id;
}

} // namespace Rush
