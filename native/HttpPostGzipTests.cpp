#include "Core.hpp"
#include "PostResponse.hpp"
#include <iostream>
#include <fstream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();bool passed=false;std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"passed",value},{"name",name}});if(!value)throw std::runtime_error(name);};
 try{
  WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("WSA startup");
  const auto origin=utf8(argv[2]),expected=utf8(argv[3]);const Bytes body={'f','o','r','m',0,255,'+','&','=','\r','\n'};
  const auto configure=[&](Manager& manager){auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((manager.root.parent_path()/L"files").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;prefs["Connections"]=8;prefs["PrefetchFileInfo"]=true;prefs["StopOnWebPage"]=false;prefs["CorrectFileExtensions"]=false;manager.setSettings(prefs);};
  Manager manager(root/L"main"/L"state");configure(manager);
  const auto make=[&](Manager& owner,const std::string& route){auto url=origin+"/"+route;return owner.add(url,"",route+".udmform","Main queue",true,{},"",Json{{"method","POST"},{"url",url},{"contentType","application/octet-stream"},{"body",b64(body)}});};
  const auto wait=[&](Manager& owner,const JobPtr& job,bool confirmation=false){const auto deadline=GetTickCount64()+15000;while(GetTickCount64()<deadline){owner.tick();{Lock lock(owner.mutex);if(confirmation&&str(job->data,"MediaPhase")=="Waiting for confirmation")return true;if(!owner.isActive(job))return !confirmation;}Sleep(10);}return false;};
  const auto start=[&](Manager& owner,const JobPtr& job){owner.resume(job);owner.tick();return wait(owner,job);};
  const auto exact=[&](const JobPtr& job){return str(job->data,"Status")=="Complete"&&fs::is_regular_file(job->target())&&fileHash(job->target())==expected;};
  const auto retain=[&](Manager& owner,const JobPtr& job){job->data["ExpectedSha256"]=std::string(64,'0');check(start(owner,job)&&str(job->data,"Status")=="Failed"&&!fs::exists(job->target()),"A final checksum rejection does not publish or resubmit the form");check(completedPostResponseCandidate(job->data)&&yes(job->data,"PostAttempted"),"The complete encoded response has a request-bound reusable receipt");check(!job->data["CompletedPrefetch"].contains("Request")&&str(job->data["CompletedPrefetch"],"RequestSha256").size()==64,"The response receipt fingerprints the request without duplicating its encrypted body");job->data["ExpectedSha256"]="";owner.save();};
  auto fresh=make(manager,"fresh");check(manager.beginPrefetch(fresh)==PrefetchAdmission::Unavailable&&!yes(fresh->data,"PostAttempted"),"Opening fresh form File Info never submits the POST speculatively");
  for(const char* route:{"post-known","post-unknown","post-redirect"}){auto job=make(manager,route);check(start(manager,job)&&exact(job),"An actual form response decodes and publishes the complete original bytes");check(num(job->data,"HttpWireSize")<num(job->data,"Size")&&num(job->data,"HttpDecodedSize")==num(job->data,"Size"),"Form completion keeps encoded and decoded byte counts separate");}
  auto info=make(manager,"post-info");retain(manager,info);
  check(manager.beginPrefetch(info)==PrefetchAdmission::Started&&wait(manager,info,true),"Reopening File Info verifies a completed form response without submitting again");
  const auto announced=num(info->data,"PrefetchedFinalSize",-1);
  check(announced>0&&!fs::exists(info->target()),"A verified decoded size is available before Start while the target remains absent");checks.back()["announcedBytes"]=announced;
  check(manager.confirmPrefetchedDownload(info,Json::object())&&wait(manager,info)&&exact(info)&&static_cast<i64>(fs::file_size(info->target()))==announced,"Start publishes exactly the form file size shown before confirmation");
  auto confirmedHash=make(manager,"post-confirmed-hash");retain(manager,confirmedHash);check(manager.beginPrefetch(confirmedHash)==PrefetchAdmission::Started&&wait(manager,confirmedHash,true),"Confirmed-checksum fixture reaches verified pre-Start output");
  bool editRejected=false;try{manager.confirmPrefetchedDownload(confirmedHash,{{"ExpectedSha256",std::string(64,'0')}});}catch(const std::exception&){editRejected=true;}
  check(editRejected&&str(confirmedHash->data,"ExpectedSha256").empty()&&yes(confirmedHash->data,"ConfirmationPending")&&manager.isActive(confirmedHash)&&!fs::exists(confirmedHash->target()),"A forbidden checksum edit during prefetch rolls back and leaves confirmation pending");
  check(manager.confirmPrefetchedDownload(confirmedHash,Json::object())&&wait(manager,confirmedHash)&&exact(confirmedHash),"Confirmation remains usable after a rejected transfer-property edit");
  auto deferred=make(manager,"post-later");retain(manager,deferred);check(manager.beginPrefetch(deferred)==PrefetchAdmission::Started&&wait(manager,deferred,true),"Completed form response is ready for Download Later");
  const FileInfoFolderChoice folder{};
  check(manager.deferPrefetchedDownload(deferred,Json::object(),folder)&&wait(manager,deferred)&&!fs::exists(deferred->target()),"Download Later leaves the saved form response unpublished");
  check(start(manager,deferred)&&exact(deferred)&&yes(deferred->data,"ReusedCompletedPrefetch"),"Resume after Download Later uses the saved form response");
  auto changed=make(manager,"post-changed");retain(manager,changed);auto request=readPostRequest(changed->data);request["body"]=b64(Bytes{'n','e','w'});changed->data["ProtectedRequest"]=protect(request.dump());manager.save();
  bool refused=false;try{transfer(manager,changed,std::make_shared<Cancel>());}catch(const std::exception& e){refused=std::string(e.what()).find("Nothing was submitted again")!=std::string::npos;}
  check(refused&&!fs::exists(changed->target())&&!changed->data.contains("CompletedPrefetch"),"Changing the form request invalidates reuse and sends no replacement request");
  auto damaged=make(manager,"post-damaged");retain(manager,damaged);
  const auto spool=manager.root/L"parts"/wide(damaged->id())/(wide(damaged->id())+L".data");
  {std::fstream file(spool,std::ios::binary|std::ios::in|std::ios::out);char byte=0;file.read(&byte,1);byte^=1;file.seekp(0);file.write(&byte,1);if(!file)throw std::runtime_error("Cannot alter owned damage fixture");}
  check(start(manager,damaged)&&str(damaged->data,"Status")=="Failed"&&yes(damaged->data,"PostAttempted")&&!damaged->data.contains("CompletedPrefetch")&&!fs::exists(damaged->target()),"A damaged saved response fails Resume without replaying POST");
  check(start(manager,damaged)&&exact(damaged),"A subsequent explicit Restart rearms the form and downloads a fresh complete response");
  auto interrupted=make(manager,"post-loss");check(start(manager,interrupted)&&str(interrupted->data,"Status")=="Failed"&&yes(interrupted->data,"PostAttempted")&&!yes(interrupted->data,"PostResponseComplete"),"Interrupted POST has no complete receipt and stays failed");
  const auto until=GetTickCount64()+500;while(GetTickCount64()<until){manager.tick();Sleep(10);}check(str(interrupted->data,"Status")=="Failed"&&!manager.isActive(interrupted),"Queue ticks do not automatically resubmit an ambiguous form response");
  for(const auto* key:{"ProtectedHeaders","HttpContentEncoding","ProtectedPostGetUrl","ProtectedPostGetHeaders"}){
   // Receipt admission is catalog-only; byte validation remains in the worker.
   Json candidate=info->data;candidate["Status"]="Paused";candidate["Size"]=20;candidate["Received"]=20;candidate["PostResponseComplete"]=true;candidate["PostResponseRequestSha256"]=postResponseRequestBinding(candidate);candidate["Segments"]=Json::array();
   candidate["CompletedPrefetch"]={{"Version",1},{"Url",str(candidate,"Url")},{"Headers",str(candidate,"ProtectedHeaders")},{"RequestSha256",str(candidate,"PostResponseRequestSha256")},{"ContentEncoding",str(candidate,"HttpContentEncoding")},{"PostGetUrl",str(candidate,"ProtectedPostGetUrl")},{"PostGetHeaders",str(candidate,"ProtectedPostGetHeaders")},{"Size",20},{"Plan",Json::array()},{"Hashes",Json::array()}};
   check(completedPostResponseCandidate(candidate),"Matching receipt metadata permits worker verification");candidate[key]="changed";check(!completedPostResponseCandidate(candidate),"Changed headers, encoding or redirect identity cannot reuse a form response");
  }
  manager.stop();
  const auto reopenRoot=root/L"reopened"/L"state";std::string id;
  {Manager first(reopenRoot);configure(first);auto job=make(first,"post-reopened");retain(first,job);id=job->id();first.stop();}
  {Manager second(reopenRoot);JobPtr job;for(auto item:second.jobs)if(item->id()==id)job=item;check(job&&completedPostResponseCandidate(job->data),"Request-bound completed response survives catalog reopening");check(start(second,job)&&exact(job)&&yes(job->data,"ReusedCompletedPrefetch"),"A reopened catalog publishes saved form bytes without another POST");second.stop();}
  passed=true;
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
