// Included inside udm by OptionsUi. Restore only contributions still matching
// this attempt; independently changed settings remain owned by their writer.
inline Json rollbackOptionsCommit(const Json& before,const Json& applied,const Json& current){
 if(!before.is_object()||!applied.is_object()||!current.is_object())throw std::runtime_error("Invalid settings rollback.");
 Json result=current;std::set<std::string> keys;
 for(auto i=before.begin();i!=before.end();++i)keys.insert(i.key());for(auto i=applied.begin();i!=applied.end();++i)keys.insert(i.key());
 for(const auto& key:keys){
  auto old=before.find(key),saved=applied.find(key),live=current.find(key);
  if(old!=before.end()&&saved!=applied.end()&&*old==*saved)continue;
  if(saved==applied.end()){if(live==current.end())result[key]=*old;continue;}
  if(live==current.end())continue;
  if(*live==*saved){if(old==before.end())result.erase(key);else result[key]=*old;continue;}
  if((old==before.end()||old->is_object())&&saved->is_object()&&live->is_object())result[key]=rollbackOptionsCommit(old==before.end()?Json::object():*old,*saved,*live);
  else if(key=="CategoryPaths"&&old!=before.end())result[key]=legacyDictionary(rollbackOptionsCommit(dictionary(*old),dictionary(*saved),dictionary(*live)));
 }
 return result;
}
