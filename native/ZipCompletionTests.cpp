#include "ZipPreview.hpp"
#include "AuthenticationFixture.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
template<class F>static bool until(F predicate){const auto end=GetTickCount64()+8000;while(!predicate()&&GetTickCount64()<end)Sleep(1);return predicate();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA winsock{};if(WSAStartup(MAKEWORD(2,2),&winsock))throw std::runtime_error("Owned ZIP completion origin unavailable.");struct Sockets{~Sockets(){WSACleanup();}} sockets;
  AuthenticationFixture origin;origin.payload=std::string(22,'\0');origin.payload[0]='P';origin.payload[1]='K';origin.payload[2]=5;origin.payload[3]=6;
  Manager manager(root/L"catalog");auto settings=manager.state["Settings"];settings["PrefetchFileInfo"]=false;const auto path=root/L"empty.zip";{std::ofstream file(path,std::ios::binary);file.write(origin.payload.data(),origin.payload.size());}
  for(const bool throws:{false,true}){
   auto notices=std::make_shared<std::atomic_int>(0);LocalZipPreviewTask task(path,[notices,throws]{++*notices;if(throws)throw std::runtime_error("Owned notification failure");});
   check(until([&]{return *notices==1;}),"A local ZIP completion wakes its owner without polling the result");auto state=task.snapshot();check(state.status=="Ready"&&state.listing&&state.listing->size==22&&state.listing->entries.empty(),"The local ZIP listing is fully published before its completion wake-up");check(*notices==1,"A throwing or successful local notification cannot replay completion");
  }
  {
   auto notices=std::make_shared<std::atomic_int>(0);LocalZipPreviewTask task(root/L"missing.zip",[notices]{++*notices;});check(until([&]{return *notices==1;}),"A failed local ZIP read also wakes its owner");check(task.snapshot().status=="Error"&&!task.snapshot().message.empty(),"A local failure publishes its diagnostic before notification");
  }
  Json data={{"Url",origin.url("/basic")},{"FileName","archive.zip"},{"ProtectedHeaders",protect(legacyDictionary(Json(Headers{{"Authorization","Basic dTpwOjI="}})).dump())}};
  for(const bool throws:{false,true}){
   auto notices=std::make_shared<std::atomic_int>(0);ZipPreviewTask task(data,settings,{},30000,[notices,throws]{++*notices;if(throws)throw std::runtime_error("Owned HTTP notification failure");});
   check(until([&]{return *notices==1;}),"Actual authenticated HTTP ZIP completion wakes its owner without result polling");auto state=task.snapshot();check(state.status=="Ready"&&state.listing&&state.listing->size==22&&state.listing->entries.empty(),"The complete HTTP ZIP listing is published before its wake-up");check(*notices==1,"A throwing or successful HTTP notification cannot replay completion");
  }
  {
   data["ProtectedHeaders"]="";auto notices=std::make_shared<std::atomic_int>(0);ZipPreviewTask task(data,settings,{},30000,[notices]{++*notices;});check(until([&]{return *notices==1;}),"An actual HTTP authentication rejection wakes the ZIP owner");check(task.snapshot().status=="Error"&&!task.snapshot().message.empty(),"A rejected HTTP ZIP exposes its error through the same published result");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
