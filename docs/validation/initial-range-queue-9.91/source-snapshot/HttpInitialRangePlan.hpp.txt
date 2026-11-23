#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace udm {
inline constexpr std::int64_t httpInitialRangePiece=4*1048576;
// Inclusive spans, established only after the helper proves representation identity.
// Request ends then match ownership instead of repeatedly shortening live responses.
inline std::vector<std::pair<std::int64_t,std::int64_t>> initialHttpRangePlan(
 std::int64_t size,int workers){
 if(size<=0||workers<1||workers>32||size<workers)
  throw std::invalid_argument("Invalid initial HTTP range plan.");
 // Keep a short queue so fast connections can take new bounded requests
 // instead of shortening a healthy response already owned by a slower peer.
 // Two pieces per worker caps checkpoint/map growth for very large files.
 const auto pieces=workers==1?1:std::max<std::int64_t>(workers,
  std::min<std::int64_t>(workers*2,1+(size-1)/httpInitialRangePiece));
 std::vector<std::pair<std::int64_t,std::int64_t>> spans;
 spans.reserve(static_cast<std::size_t>(pieces));
 const auto width=size/pieces,remainder=size%pieces;
 std::int64_t at=0;
 for(int n=0;n<pieces;++n){
  const auto length=width+(n<remainder?1:0);
  spans.emplace_back(at,at+length-1);at+=length;
 }
 return spans;
}
}
