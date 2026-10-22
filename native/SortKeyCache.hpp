#pragma once
#include <map>
#include <memory>
#include <string>
#include <utility>
namespace udm {
class SortKeyCache {
 struct Entry {std::string revision,value;};
 // Keep the previous refresh intact until finish(), without copying potentially
 // large encrypted header revisions and decrypted keys on every cache hit.
 std::map<std::string,std::shared_ptr<const Entry>> saved,current;
public:
 void begin(){current.clear();}
 template<class Loader> const std::string& get(const std::string& id,const std::string& revision,Loader load){
  auto found=saved.find(id);
  auto entry=found!=saved.end()&&found->second->revision==revision?found->second:std::make_shared<const Entry>(Entry{revision,load()});
  return current.insert_or_assign(id,std::move(entry)).first->second->value;
 }
 void finish(){saved.swap(current);current.clear();}
 void clear(){saved.clear();current.clear();}
};
}
