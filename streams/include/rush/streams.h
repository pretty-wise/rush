/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#pragma once

typedef struct stream_context *stream_t;

struct stream_manager {};
/*

class IStream {
  virtual StreamId GetId() const;
  virtual u16 GetPriority(PriorityKey *priority, PriorityValue *priority_data,
                          u16 count);
  virtual u16 Pack(void *buffer, u16 nbytes, PriorityValue *priority_data);
};

} // namespace Rush

void Sort(PriorityKey *keys, PriorityValue *values, u16 count);

void Pack(void *buffer, u16 nbytes) {
  static const kMaxObjects = 100;

  u16 num_prioritized = 0;
  PriorityKey priority[kMaxObjects];
  PriorityValue priority_data[kMaxObjects];

  static const kStreamNum = 10;
  IStream streams[kStreamNum];

  // gather priority data.
  for(u16 i = 0; i < kStreamNum && num_prioritized < kMaxObjects; ++i)
    num_prioritized += streams[i].GetPriority(priority + num_prioritized,
                                              priority_data + num_prioritized,
                                              kMaxObjects - num_prioritized);
}

// sort by priority.
Sort(priority, priority_data, num_prioritized);

// pack with the priority.
u16 data_packed = 0;
for(u16 i = 0; i < num_prioritized && data_packed < nbytes; ++i) {
  IStream *stream = stream_manager.Get(priority_data[i]);
  data_packed += stream->Pack(buffer + data_packed, nbytes - data_packed);
}*/
