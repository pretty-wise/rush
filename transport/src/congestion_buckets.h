/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

#include "rush/rush.h"
#include "congestion.h"

namespace Rush {

struct bucket_t {
  rush_time_t duration;
  rush_sequence_t first_sent;
  rush_sequence_t last_sent;
  u16 num_lost;
  u16 num_delivered;
  enum Status {
    st_OPENED,   // packets sent through this bucket.
    st_CLOSED,   // gathering acks for sent packets. packet sending ended.
    st_FINALIZED // gathered all packet acks.
  } status;

  bucket_t()
      : duration(0), first_sent(0), last_sent(0), num_lost(0), num_delivered(0),
        status(st_OPENED) {}
};

class BucketCongestion : public ICongestionControl {
private:
  static const u16 kBucketNum = 10;
  static constexpr rush_time_t kBucketTime = 300;

  bucket_t buckets[kBucketNum];
  bucket_t *current_send_bucket;
  bucket_t *current_recv_bucket;
  rush_time_t bucket_time;

public:
  BucketCongestion(const CongestionInfo &info, rush_time_t *interval,
                  rush_time_t now);
  void TickSend(rush_time_t now, rush_sequence_t id);
  void TickDelivered(rush_time_t now, rush_sequence_t id, rush_time_t rtt);
  void TickLost(rush_time_t now, rush_sequence_t id);

private:
  void Adjust();
};

} // namespace Rush
