#pragma once
#include <optional>
#include <string>
#include <cstdint>
namespace udm {
// Failed reads are retried; zero means Windows cannot supply a change counter.
class ClipboardSnapshot {
 std::optional<std::string> cached;
 std::uint32_t sequence=0;
public:
 void reset(){cached.reset();sequence=0;}
 template<class Sequence,class Reader>
 std::optional<std::string> read(Sequence current,Reader reader){
  const auto before=current();
  if(before&&cached&&before==sequence)return cached;
  auto value=reader();
  if(!value)return std::nullopt;
  const auto after=current();
  if(before&&before==after){cached=value;sequence=before;}
  else reset(); // Concurrent changes and delayed rendering need another read.
  return value;
 }
};
}
