#include "AdaptiveJournal.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;auto root=fs::absolute(argv[1]);fs::create_directories(root);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Json checks=Json::array(),benchmark;std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 auto rejects=[&](auto operation,const char* name){bool rejected=false;try{operation();}catch(const std::exception&){rejected=true;}check(rejected,name);};
 try{
  const Json a={{"size",12},{"sha256",std::string(64,'a')}},b={{"size",14},{"sha256",std::string(64,'b')}},empty={{"size",0},{"sha256",std::string(64,'c')}};
  auto path=root/L"journal";fs::create_directories(path);atomicText(path/L"completed.json",Json{{"0",a}}.dump());AdaptiveJournal journal(path,3);auto state=journal.load();check(state==Json{{"0",a}},"Legacy completed index loads unchanged");
  journal.commit(1,b);state["1"]=b;journal.commit(2,empty);state["2"]=empty;
  check(Json::parse(readText(path/L"completed.json"))==Json{{"0",a}},"Individual commits do not rewrite the complete index");
  {AdaptiveJournal resumed(path,3);check(resumed.load()==state,"Committed normal and empty receipts survive restart before snapshot");}
  atomicText(path/L"1.receipt.json.tmp","unfinished");{AdaptiveJournal resumed(path,3);check(resumed.load()==state,"Interrupted temporary receipt does not replace committed data");}
  journal.checkpoint(state);auto committed=readText(path/L"completed.json");
  {AdaptiveJournal resumed(path,3);check(resumed.load()==state,"Old-generation receipts cannot overwrite the committed snapshot");}
  auto next=state;next["0"]=b;
  {Handle hold(CreateFileW((path/L"completed.json.tmp").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));check((bool)hold,"Snapshot contention fixture holds temporary file");rejects([&]{journal.checkpoint(next);},"Failed snapshot cannot advance generation");}
  check(readText(path/L"completed.json")==committed,"Failed snapshot leaves the old index intact");journal.commit(0,b);{AdaptiveJournal resumed(path,3);check(resumed.load()==next,"Receipt after failed snapshot retains the active generation");}
  {Handle hold(CreateFileW((path/L"0.receipt.json.tmp").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr));check((bool)hold,"Receipt contention fixture holds temporary file");rejects([&]{journal.commit(0,a);},"Failed receipt publication reports failure");}
  {AdaptiveJournal resumed(path,3);check(resumed.load()==next,"Failed receipt publication preserves previous committed receipt");}
  atomicText(path/L"0.receipt.json","truncated");{AdaptiveJournal resumed(path,3);auto recovered=resumed.load();check(!recovered.contains("0")&&recovered["1"]==b,"Malformed receipt invalidates only its part");}
  rejects([&]{journal.commit(3,a);},"Receipt path cannot exceed selected segment count");rejects([&]{journal.commit(0,Json{{"binding",std::string(121*1024,'x')}});},"Oversized receipt is rejected");
  atomicText(path/L"keep.txt","personal fixture");journal.clean();check(fs::exists(path/L"keep.txt")&&fs::exists(path/L"completed.json")&&!fs::exists(path/L"1.receipt.json")&&!fs::exists(path/L"1.receipt.json.tmp"),"Cleanup preserves unknown files and compatibility snapshot");
  auto damaged=root/L"damaged-index";fs::create_directories(damaged);atomicText(damaged/L"completed.json","broken");AdaptiveJournal repair(damaged,2);check(repair.load().empty(),"Corrupt legacy snapshot starts an empty durable generation");repair.commit(1,b);{AdaptiveJournal resumed(damaged,2);check(resumed.load()==Json{{"1",b}},"Recovery after corrupt snapshot preserves newly committed receipts across restart");}
  constexpr size_t count=1000;Json all=Json::object();uint64_t oldBytes=0,newBytes=0;auto oldFolder=root/L"old",newFolder=root/L"new";fs::create_directories(oldFolder);fs::create_directories(newFolder);
  auto started=std::chrono::steady_clock::now();for(size_t i=0;i<count;++i){all[std::to_string(i)]=a;auto text=all.dump();oldBytes+=text.size();atomicText(oldFolder/L"completed.json",text);}double oldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
  AdaptiveJournal linear(newFolder,count);linear.load();started=std::chrono::steady_clock::now();for(size_t i=0;i<count;++i){linear.commit(i,a);newBytes+=fs::file_size(newFolder/(std::to_wstring(i)+L".receipt.json"));}linear.checkpoint(all);newBytes+=fs::file_size(newFolder/L"completed.json");double newMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
  {AdaptiveJournal resumed(newFolder,count);check(resumed.load()==all,"All 1000 receipts match after compaction and restart");}
  check(newBytes*100<oldBytes,"Recovery serialization drops by more than 99 percent for 1000 segments");benchmark={{"segments",count},{"oldSerializedBytes",oldBytes},{"newSerializedBytes",newBytes},{"oldElapsedMs",oldMs},{"newElapsedMs",newMs},{"scope","Local checkpoint storage only, excluding network and IDM"}};
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks},{"benchmark",benchmark}}.dump(2));std::cout<<checks.size()<<" checks; "<<error<<std::endl;CoUninitialize();return error.empty()?0:1;
}
