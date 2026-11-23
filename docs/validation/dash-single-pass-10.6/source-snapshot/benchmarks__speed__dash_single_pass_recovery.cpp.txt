#include "DashManifest.hpp"
#include "AdaptiveJournal.hpp"
#include <iostream>
using namespace udm;
static Json load(const fs::path& root){return Json::parse(reveal(str(Json::parse(readText(root/L"recording.json")),"protected")));}
static void save(const fs::path& root,const Json& index){fs::create_directories(root);atomicText(root/L"recording.json",Json{{"schema",1},{"protected",protect(index.dump())}}.dump(),false);}
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto seed=fs::absolute(argv[1]),root=fs::absolute(argv[2]);const auto mode=utf8(argv[3]);Json checks=Json::array();std::string error;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  auto cancel=std::make_shared<Cancel>();
  if(mode=="old-reader"){
   const auto before=readText(seed/L"recording.json");bool rejected=false;try{reconcileDashRecording(seed,*cancel);}catch(const std::exception&){rejected=true;}
   check(rejected,"Old reader rejects new pending-version schema");check(readText(seed/L"recording.json")==before,"Old reader rejection preserves the complete checkpoint");
  }else{
   check(!fs::exists(root),"Recovery control uses a fresh owned directory");auto index=load(seed);auto& record=index["tracks"][0]["segments"]["1"];record["committed"]=false;record.erase("resource");
   Json binding=index["tracks"][0]["identity"];binding["number"]=record["number"];binding["time"]=record["time"];binding["duration"]=record["duration"];
   if(mode=="unversioned"){record.erase("resourceVersion");}else binding["resourceVersion"]=record["resourceVersion"];
   record["binding"]=adaptiveSelectionDigest(binding);save(root,index);
   std::vector<std::string> urls=record["urls"].get<std::vector<std::string>>();auto stored=storeDashResource(root,binding,urls,{},Json::object(),cancel);
   check(stored.bytes==4096&&fileHash(stored.path)==stored.sha256,"Independent writer commits a fully verified receipt before the index");
   auto recovered=reconcileDashRecording(root,*cancel);const auto& accepted=recovered["tracks"][0]["segments"]["1"];
   if(mode=="unversioned"){
    check(!yes(accepted,"committed")&&!accepted.contains("resource"),"Pending version cannot adopt an old unversioned orphan receipt");
    check(fs::exists(stored.path)&&fileHash(stored.path)==stored.sha256,"Unaccepted orphan remains intact");
    auto legacy=index;legacy["schema"]=1;legacy["tracks"][0]["segments"]["1"].erase("versionRequired");save(root,legacy);auto restored=reconcileDashRecording(root,*cancel);
    check(yes(restored["tracks"][0]["segments"]["1"],"committed"),"Legacy schema still recovers its own unversioned receipt");
   }else check(yes(accepted,"committed")&&str(accepted["resource"],"sha256")==stored.sha256,"Pinned receipt-before-index completion recovers without another request");
  }
 }catch(const std::exception& e){error=e.what();}
 fs::create_directories(root);Json result={{"passed",error.empty()},{"error",error},{"checks",checks}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump()<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
