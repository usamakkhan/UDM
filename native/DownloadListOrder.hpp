#pragma once
#include "Core.hpp"

namespace udm {
struct DownloadListSortKey {std::string text;i64 integer=0;double rate=0;int progress=-1;bool completed=false;i64 secondaryInteger=0;};
// UI-thread cache. Keep the original candidate sequence as well as its sorted
// order: stable-sort ties must follow current catalog order, not an old view.
class DownloadListOrder {
 int column=-1;bool ascending=true,valid=false;
 std::vector<JobPtr> source,ordered;std::vector<DownloadListSortKey> keys;
public:
 template<class SameKey> bool reuse(int selectedColumn,bool direction,const std::vector<JobPtr>& candidates,SameKey sameKey,std::vector<JobPtr>& output)const{
  if(!valid||selectedColumn!=column||direction!=ascending||candidates!=source||keys.size()!=candidates.size())return false;
  for(size_t i=0;i<candidates.size();++i)if(!sameKey(candidates[i],keys[i]))return false;
  output=ordered;return true;
 }
 void remember(int selectedColumn,bool direction,std::vector<JobPtr> candidates,std::vector<DownloadListSortKey> values,const std::vector<JobPtr>& result){
  if(candidates.size()!=values.size()||candidates.size()!=result.size())throw std::runtime_error("Invalid download-list sort cache.");
  // Commit only after all potentially allocating copies succeed.
  auto nextOrder=result;source=std::move(candidates);keys=std::move(values);ordered=std::move(nextOrder);column=selectedColumn;ascending=direction;valid=true;
 }
 void clear(){source.clear();ordered.clear();keys.clear();valid=false;}
};
}
