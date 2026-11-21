#pragma once
#include "Core.hpp"
#include <algorithm>
#include <array>
namespace udm {
// UI-thread inputs remain unchanged while this runs. The formatter reads model
// data only; controls are updated by the caller after the snapshot lock ends.
template<class Format>
bool tryModalSizeChanges(Manager& manager,const std::vector<JobPtr>& visible,
 const std::vector<std::array<std::string,12>>& cached,size_t first,size_t count,
 std::vector<std::pair<size_t,std::string>>& changes,Format format){
 changes.clear();const auto limit=std::min(visible.size(),cached.size());
 if(first>=limit||!count)return true;
 std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return false;
 const auto end=first+std::min(count,limit-first);
 for(auto row=first;row<end;++row){
  auto value=format(visible[row]->data);
  if(cached[row][2]!=value)changes.emplace_back(row,std::move(value));
 }
 return true;
}
}
