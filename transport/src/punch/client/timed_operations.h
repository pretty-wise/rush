/*
 * Copywrite 2014-2015 Krzysztof Stasik. All rights reserved.
 */
#include "base/core/types.h"

#include <functional>

template <class T, class Id, u32 Capacity> class TimedOperations {
public:
  TimedOperations() : m_size(0) {}
  bool Add(Id id, const T &data, u32 timeout);
  bool Remove(Id id, std::function<void(T &data)> op);
  bool Remove(Id id) { return Remove(id, nullptr); }
  void Timeout(std::function<u32(Id id, T &data)> op, u32 delta_ms);
  bool SetTimeout(Id id, std::function<u32(T &data)> op);
  bool Has(Id id) const;

private:
  bool IsFull() const;

private:
  u32 m_size;
  u32 m_timeout[Capacity];
  Id m_id[Capacity];
  T m_data[Capacity];
};

template <class T, class Id, u32 Capacity>
bool TimedOperations<T, Id, Capacity>::Add(Id id, const T &data, u32 timeout) {
  if(IsFull()) {
    return false;
  }
  if(Has(id)) {
    return false;
  }
  m_data[m_size] = data;
  m_id[m_size] = id;
  m_timeout[m_size] = timeout;
  ++m_size;
  return true;
}

template <class T, class Id, u32 Capacity>
bool TimedOperations<T, Id, Capacity>::Remove(Id id,
                                              std::function<void(T &data)> op) {
  u32 index = 0;
  while(index < m_size) {
    if(m_id[index] == id) {
      if(op) {
        op(m_data[index]);
      }
      u32 last_index = m_size - 1;
      if(index != last_index) {
        m_data[index] = m_data[last_index];
        m_id[index] = m_id[last_index];
        m_timeout[index] = m_timeout[last_index];
      }
      --m_size;
      return true;
    }
    ++index;
  }
  return false;
}

template <class T, class Id, u32 Capacity>
void TimedOperations<T, Id, Capacity>::Timeout(
    std::function<u32(Id id, T &data)> op, u32 delta_ms) {
  u32 index = 0;
  while(index < m_size) {
    s32 left = m_timeout[index] - delta_ms;
    if(left <= 0) {
      m_timeout[index] = op(m_id[index], m_data[index]);
      if(m_timeout[index] <= 0) {
        u32 last_index = m_size - 1;
        --m_size;
        if(index != last_index) {
          m_data[index] = m_data[last_index];
          m_id[index] = m_id[last_index];
          m_timeout[index] = m_timeout[last_index];
          continue; // no index++ here.
        }
      }
    } else {
      m_timeout[index] -= delta_ms;
    }
    ++index;
  }
}

template <class T, class Id, u32 Capacity>
bool TimedOperations<T, Id, Capacity>::SetTimeout(
    Id id, std::function<u32(T &data)> op) {
  for(u32 i = 0; i < m_size; ++i) {
    if(m_id[i] == id) {
      m_timeout[i] = op(m_data[i]);
      return true;
    }
  }
  return false;
}

template <class T, class Id, u32 Capacity>
bool TimedOperations<T, Id, Capacity>::IsFull() const {
  return m_size == Capacity;
}

template <class T, class Id, u32 Capacity>
bool TimedOperations<T, Id, Capacity>::Has(Id id) const {
  for(u32 i = 0; i < m_size; ++i) {
    if(m_id[i] == id) {
      return true;
    }
  }
  return false;
}
