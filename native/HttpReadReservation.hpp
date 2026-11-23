#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace udm {
struct HttpReadReservation {
 std::size_t count;
 std::int64_t protectedEnd; // Exclusive; an EOF check owns no additional file byte.
};

// The caller holds the range-map lock from this decision through publishing
// protectedEnd. A later split must start beyond both the write and this read.
inline HttpReadReservation reserveHttpBodyRead(std::int64_t cursor,std::int64_t end,
 std::size_t window,bool unreadResponse){
 if(cursor<0||end<0||window==0)throw std::invalid_argument("Invalid HTTP read reservation.");
 if(!unreadResponse)return {1,cursor}; // Retain the declared-body EOF/excess-data check.
 if(cursor>=end)return {0,cursor}; // A shortened response has no more owned bytes.
 const auto count=static_cast<std::size_t>(std::min<std::uint64_t>(window,
  static_cast<std::uint64_t>(end-cursor)));
 return {count,cursor+static_cast<std::int64_t>(count)};
}
}
