#include "ModalSizeRefresh.hpp"
#include <future>
#include <iostream>
#include <limits>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");
  auto first=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Paused"},{"Folder",utf8(root.wstring())},{"FileName","first.bin"},{"Size",1024}});
  auto second=std::make_shared<Job>(Json{{"Id",guid()},{"Status","Paused"},{"Folder",utf8(root.wstring())},{"FileName","second.bin"},{"Size",4096}});
  manager.jobs={first,second};std::vector<JobPtr> visible={first,second};std::vector<std::array<std::string,12>> cached(2);cached[0][2]="1024";cached[1][2]="4096";
  std::vector<std::pair<size_t,std::string>> changes;unsigned formats=0;
  const auto format=[&](const Json& j){++formats;return std::to_string(num(j,"Size"));};
  check(tryModalSizeChanges(manager,visible,cached,0,2,changes,format)&&changes.empty(),"Unchanged visible size cells need no control update");
  std::promise<void> entered,release;auto ready=entered.get_future();auto released=release.get_future().share();std::atomic_bool holding=false,expired=false;
  std::thread writer([&]{Lock lock(manager.mutex);first->data["Size"]=2048;holding=true;entered.set_value();if(released.wait_for(std::chrono::seconds(3))!=std::future_status::ready)expired=true;holding=false;});
  struct Join{std::promise<void>& release;std::thread& writer;~Join(){try{release.set_value();}catch(...){}if(writer.joinable())writer.join();}} joined{release,writer};
  check(ready.wait_for(std::chrono::seconds(3))==std::future_status::ready,"An independent catalog writer owns the model before refresh");
  formats=0;changes.emplace_back(99,"stale");const bool snapshot=tryModalSizeChanges(manager,visible,cached,0,2,changes,format);
  check(!snapshot&&holding&&!expired,"Display-only modal refresh returns while the catalog remains held");
  check(changes.empty()&&formats==0,"A skipped refresh neither reads live rows nor returns stale changes");
  check(cached[0][2]=="1024"&&cached[1][2]=="4096","Skipping a busy catalog preserves every displayed size");
  check(tryModalSizeChanges(manager,visible,cached,9,2,changes,format)&&changes.empty(),"An out-of-view range never waits for the catalog");
  release.set_value();writer.join();
  check(tryModalSizeChanges(manager,visible,cached,0,2,changes,format)&&changes==std::vector<std::pair<size_t,std::string>>{{0,"2048"}},"A later tick catches up to the latest model value");
  check(cached[0][2]=="1024","Snapshot creation does not apply UI control changes under the lock");
  cached[0][2]=changes[0].second;
  check(tryModalSizeChanges(manager,visible,cached,0,2,changes,format)&&changes.empty(),"Applying the delivered value converges without duplicate changes");
  {Lock lock(manager.mutex);first->data["Size"]=3072;second->data["Size"]=8192;}
  check(tryModalSizeChanges(manager,visible,cached,1,1,changes,format)&&changes==std::vector<std::pair<size_t,std::string>>{{1,"8192"}},"Only the requested visible row is refreshed");
  check(tryModalSizeChanges(manager,visible,cached,0,std::numeric_limits<size_t>::max(),changes,format)&&changes.size()==2,"A maximal range count is clipped without overflow");
  cached.resize(1);
  check(tryModalSizeChanges(manager,visible,cached,0,5,changes,format)&&changes.size()==1&&changes[0].first==0,"Mismatched cached and live row counts cannot read beyond either list");
  check(tryModalSizeChanges(manager,visible,cached,0,0,changes,format)&&changes.empty(),"An empty range emits no stale changes");
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
