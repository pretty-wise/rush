/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "congestion_buckets.h"
#include "base/core/macro.h"
#include "base/math/math.h"
#include "packet.h"

namespace Rush {

namespace {

void open(bucket_t *bucket, rush_sequence_t first_send, rush_time_t _duration) {
  bucket->duration = _duration;
  bucket->first_sent = first_send;
  bucket->last_sent = first_send;
  bucket->num_lost = 0;
  bucket->num_delivered = 0;
  bucket->status = bucket_t::st_OPENED;
}

void sent(bucket_t *bucket, rush_sequence_t id) {
  BASE_ASSERT(bucket->status == bucket_t::st_OPENED);
  bucket->last_sent = id;
}

void close(bucket_t *bucket) {
  BASE_ASSERT(bucket->status == bucket_t::st_OPENED);
  bucket->status = bucket_t::st_CLOSED;
}

bool contains(const bucket_t *bucket, rush_sequence_t id) {
  // id is in <first_sent, last_sent>.
  return id == bucket->first_sent || id == bucket->last_sent ||
         (IsMoreRecent(id, bucket->first_sent) &&
          !IsMoreRecent(id, bucket->last_sent));
}

void finalize(bucket_t *bucket, rush_sequence_t id) {
  if(bucket->status == bucket_t::st_CLOSED) {
    if(id == bucket->last_sent || IsMoreRecent(id, bucket->last_sent)) {
      BASE_ASSERT(Diff(bucket->last_sent, bucket->first_sent) + 1 ==
                      bucket->num_delivered + bucket->num_lost,
                  "bucket finalized error: on lost %d. %d -> %d. %d %d", id,
                  bucket->first_sent, bucket->last_sent,
                  Diff(bucket->last_sent, bucket->first_sent) + 1,
                  bucket->num_delivered + bucket->num_lost);
      bucket->status = bucket_t::st_FINALIZED;
    }
  }
}

void lost(bucket_t *bucket, rush_sequence_t id) {
  if(bucket->status == bucket_t::st_OPENED ||
     bucket->status == bucket_t::st_CLOSED) {
    if(contains(bucket, id)) {
      bucket->num_lost++;
    }
    finalize(bucket, id);
  }
}

void delivered(bucket_t *bucket, rush_sequence_t id) {
  if(bucket->status == bucket_t::st_OPENED ||
     bucket->status == bucket_t::st_CLOSED) {
    if(contains(bucket, id)) {
      bucket->num_delivered++;
    }
    finalize(bucket, id);
  }
}

float loss_ratio(const bucket_t *bucket) {
  return (float)bucket->num_lost /
         (float)(bucket->num_delivered + bucket->num_lost);
}

bool finalized(const bucket_t *bucket) {
  return bucket->status == bucket_t::st_FINALIZED;
}

} // unnamed namespace

BucketCongestion::BucketCongestion(const CongestionInfo &info,
                                 rush_time_t *interval, rush_time_t now)
    : ICongestionControl(info, interval) {
  current_send_bucket = buckets;
  current_recv_bucket = buckets; //&buckets[kBucketNum-1];
  bucket_time = now - kBucketTime;
}

void BucketCongestion::TickSend(rush_time_t now, rush_sequence_t id) {
  if(now > bucket_time + current_send_bucket->duration) {
    close(current_send_bucket);

    ++current_send_bucket;
    if(current_send_bucket == &buckets[kBucketNum]) {
      current_send_bucket = buckets;
    }

    bucket_time = now;
    open(current_send_bucket, id, kBucketTime);
  } else {
    sent(current_send_bucket, id);
  }
}

void BucketCongestion::TickDelivered(rush_time_t now, rush_sequence_t id,
                                    rush_time_t rtt) {
  delivered(current_recv_bucket, id);
  if(finalized(current_recv_bucket)) {
    Adjust();

    ++current_recv_bucket;
    if(current_recv_bucket == &buckets[kBucketNum]) {
      current_recv_bucket = buckets;
    }
    delivered(current_recv_bucket, id);
  }
}

void BucketCongestion::TickLost(rush_time_t now, rush_sequence_t id) {
  lost(current_recv_bucket, id);
  if(finalized(current_recv_bucket)) {
    Adjust();
    ++current_recv_bucket;
    if(current_recv_bucket == &buckets[kBucketNum]) {
      current_recv_bucket = buckets;
    }
    lost(current_recv_bucket, id);
  }
}

float Factor(float x) {
  BASE_ASSERT(x <= 1.f, "%f", x);
  BASE_ASSERT(x >= 0.f, "%f", x);
  // return Base::Math::Pow<float>(10, Base::Math::Log(1-x));
  return Base::Math::Pow<float>(10, Base::Math::Log(1.3f - x)) * 0.6f;
}

void BucketCongestion::Adjust() {
  static const u16 kThroughputDelta = 256;

  if(finalized(current_recv_bucket)) {
    rush_time_t interval = *m_current_interval;
    u16 throughput_target =
        (1000.f / m_info.GetMinInterval()) * m_info.GetMtu();
    u16 throughput_current = (1000.f / *m_current_interval) * m_info.GetMtu();
    u16 delta_increase = kThroughputDelta * Factor((float)throughput_current /
                                                   (float)throughput_target);
    u16 throughput_slower = (float)throughput_current * 0.95f;
    u16 throughput_faster = throughput_current + delta_increase;
    rush_time_t interval_slow = 1000.f * m_info.GetMtu() / throughput_slower;
    rush_time_t interval_fast = 1000.f * m_info.GetMtu() / throughput_faster;

    float ratio_3 = loss_ratio(current_recv_bucket);
    bucket_t *prev_bucket = current_recv_bucket - 1;
    if(prev_bucket < buckets)
      prev_bucket = &buckets[kBucketNum - 1];
    if(finalized(prev_bucket)) {
      ratio_3 += loss_ratio(prev_bucket);
    }
    prev_bucket--;
    if(prev_bucket < buckets)
      prev_bucket = &buckets[kBucketNum - 1];
    if(finalized(prev_bucket)) {
      ratio_3 += loss_ratio(prev_bucket);
    }
    if(loss_ratio(current_recv_bucket) > 0.f) {
      interval = interval_slow;
    } else if(ratio_3 == 0.f) {
      interval = interval_fast;
    } else {
    }
    *m_current_interval = interval;
  }
  if(*m_current_interval > m_info.GetMaxInterval())
    *m_current_interval = m_info.GetMaxInterval();
  if(*m_current_interval < m_info.GetMinInterval())
    *m_current_interval = m_info.GetMinInterval();
}

} // namespace Rush
