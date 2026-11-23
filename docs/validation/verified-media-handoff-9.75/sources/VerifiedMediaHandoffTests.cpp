#include "Core.hpp"
#ifdef UDM_BASELINE_MEDIA_HANDOFF
#include "BaselineMediaPublication.hpp"
#else
#include "MediaPublication.hpp"
#endif
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();std::string error;
 const auto check=[&](bool passed,const char* name){checks.push_back({{"passed",passed},{"name",name}});};
 try{
  for(bool relocated:{false,true}){
   const auto area=root/(relocated?L"relocated":L"same-folder");Manager manager(area/L"catalog");auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((area/L"files").wstring());settings["CategoryFolders"]=false;manager.setSettings(settings);
   auto job=manager.add("https://example.invalid/verified-video.bin","","video.bin","Main queue",true);
   const auto folder=relocated?area/L"source":job->target().parent_path();fs::create_directories(folder);const auto source=folder/L"verified.bin";
   atomicText(source,"verified original video",false);VerifiedMediaFile verified(source);const auto digest=fileHash(verified.file.h);
#ifdef UDM_BASELINE_MEDIA_HANDOFF
   verified.releaseForPublication(source);
#else
   auto publicationSource=verified.releaseForPublication(source);
#endif
   Handle intruder(CreateFileW(source.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
   check(!intruder,"No writer can enter between verification and publication");
   if(intruder){const char changed[]="substituted media bytes";DWORD written=0;if(!WriteFile(intruder.h,changed,sizeof(changed)-1,&written,nullptr)||written!=sizeof(changed)-1)throw std::runtime_error("Cannot reproduce the write gap");CloseHandle(intruder.h);intruder.h=INVALID_HANDLE_VALUE;}
#ifdef UDM_BASELINE_MEDIA_HANDOFF
   publishMediaOutput(manager,job,source,digest,std::make_shared<Cancel>());
#else
   publishMediaOutput(manager,job,source,digest,std::make_shared<Cancel>(),nullptr,std::move(publicationSource));
#endif
   check(fs::exists(job->target())&&fileHash(job->target())==digest,"Published bytes equal the digest measured before handoff");
   check(str(job->data,"Status")=="Complete"&&str(job->data,"Sha256")==fileHash(job->target()),"Accepted catalog digest describes the actual completed file");manager.stop();
  }
#ifndef UDM_BASELINE_MEDIA_HANDOFF
  const auto area=root/L"wrong-name";fs::create_directories(area);auto first=area/L"first.bin",second=area/L"second.bin";atomicText(first,"equal bytes",false);atomicText(second,"equal bytes",false);VerifiedMediaFile held(first);bool rejected=false;
  try{auto pin=held.releaseForPublication(second);}catch(const std::exception&){rejected=true;}
  check(rejected,"An equal-byte different file cannot supply the verified handoff");
  Handle writer(CreateFileW(first.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));check(!writer,"A rejected handoff retains the original verification protection");
  auto pin=held.releaseForPublication(first);check(bool(pin),"The correct file remains available after a rejected handoff");
  check(!PublicationSourcePin::openReadFrom(first,INVALID_HANDLE_VALUE),"Invalid verification handles cannot create a publication proof");
#endif
 }catch(const std::exception& e){error=e.what();}
 const bool passed=error.empty()&&!checks.empty()&&std::all_of(checks.begin(),checks.end(),[](const Json& row){return yes(row,"passed");});atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":"failed")<<"\n";return passed?0:1;
}
