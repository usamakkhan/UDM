#include "Core.hpp"
#include "PostResponse.hpp"
#include "CapturedResponseTestAccess.hpp"
#include <fstream>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=5)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();bool passed=false;std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  WSADATA sockets{};if(WSAStartup(MAKEWORD(2,2),&sockets))throw std::runtime_error("WSA startup");
  const auto origin=utf8(argv[2]),expected=utf8(argv[3]),pdfHash=utf8(argv[4]);const Bytes body={'f','o','r','m',0,255,'+','&','=','\r','\n'};
  const auto configure=[&](Manager& m){auto p=m.state["Settings"];p["DownloadFolder"]=utf8((m.root.parent_path()/L"files").wstring());p["CategoryFolders"]=false;p["ProxyMode"]="Connect directly";p["Retries"]=0;p["Connections"]=8;p["PrefetchFileInfo"]=true;p["StopOnWebPage"]=false;p["CorrectFileExtensions"]=false;m.setSettings(p);};
  Manager manager(root/L"main"/L"state");configure(manager);
  const auto make=[&](Manager& m,const std::string& route){auto url=origin+"/"+route;return m.add(url,"",route+".udmform","Main queue",true,{},"",Json{{"method","POST"},{"url",url},{"contentType","application/octet-stream"},{"body",b64(body)}});};
  const auto wait=[&](Manager& m,const JobPtr& j,bool confirmation=false){const auto deadline=GetTickCount64()+15000;while(GetTickCount64()<deadline){m.tick();{Lock lock(m.mutex);if(confirmation&&str(j->data,"MediaPhase")=="Waiting for confirmation")return true;if(!m.isActive(j))return !confirmation;}Sleep(10);}return false;};
  const auto start=[&](Manager& m,const JobPtr& j){m.resume(j);m.tick();return wait(m,j);};
  const auto exact=[&](const JobPtr& j,const std::string& hash){return str(j->data,"Status")=="Complete"&&fs::is_regular_file(j->target())&&fileHash(j->target())==hash;};
  const auto retain=[&](Manager& m,const JobPtr& j){j->data["ExpectedSha256"]=std::string(64,'0');check(start(m,j)&&str(j->data,"Status")=="Failed"&&!fs::exists(j->target()),"A final verification failure retains an unpublished complete plain form response");check(completedPostResponseCandidate(j->data)&&yes(j->data,"PostAttempted"),"Failure checkpoints a request-bound response instead of requiring form resubmission");check(str(j->data,"Error").find("SHA-256 mismatch")!=std::string::npos,"Response retention preserves the original verification error");j->data["ExpectedSha256"]="";m.save();};
  auto fresh=make(manager,"fresh");check(manager.beginPrefetch(fresh)==PrefetchAdmission::Unavailable&&!yes(fresh->data,"PostAttempted"),"Fresh forms still do not submit on File Info open");
  auto normal=make(manager,"success");std::atomic_int receiptWrites=0;
  CapturedResponseTestAccess::saveCheckpoint(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed"&&normal->data.contains("CompletedPrefetch"))++receiptWrites;});
  check(start(manager,normal)&&exact(normal,expected),"Successful plain POST still publishes exact bytes");
  check(receiptWrites==0&&!normal->data.contains("CompletedPrefetch")&&!yes(normal->data,"PostResponseComplete")&&str(normal->data.at("FinishPhases"),"HashMode")=="inline","Successful plain POST adds no saved-response receipt pass and keeps inline hashing");
  CapturedResponseTestAccess::saveCheckpoint(manager,{});
  auto info=make(manager,"info");retain(manager,info);check(manager.beginPrefetch(info)==PrefetchAdmission::Started&&wait(manager,info,true),"Reopened File Info verifies retained plain response without submitting again");
  auto announced=num(info->data,"PrefetchedFinalSize",-1);check(announced>0&&!fs::exists(info->target()),"Retained plain response supplies verified size before Start");checks.back()["announcedBytes"]=announced;
  check(manager.confirmPrefetchedDownload(info,Json::object())&&wait(manager,info)&&exact(info,expected)&&static_cast<i64>(fs::file_size(info->target()))==announced,"Published plain form file matches the size shown before Start");
  check(!info->data.contains("CompletedPrefetch")&&!info->data.contains("PostResponseComplete"),"Completion discards the obsolete form response receipt");
  for(const char* route:{"unknown","redirect"}){auto j=make(manager,route);retain(manager,j);check(start(manager,j)&&exact(j,expected)&&yes(j->data,"ReusedCompletedPrefetch"),"Unknown-length and redirected plain responses reuse their complete saved body");}
  auto empty=make(manager,"empty");retain(manager,empty);check(manager.beginPrefetch(empty)==PrefetchAdmission::Started&&wait(manager,empty,true)&&num(empty->data,"PrefetchedFinalSize",-1)==0,"A retained empty response reports verified zero before Start");check(manager.confirmPrefetchedDownload(empty,Json::object())&&wait(manager,empty)&&str(empty->data,"Status")=="Complete"&&fs::file_size(empty->target())==0,"The retained empty response publishes an empty final file");
  auto later=make(manager,"later");retain(manager,later);check(manager.beginPrefetch(later)==PrefetchAdmission::Started&&wait(manager,later,true),"Retained plain response reaches confirmation before Download Later");
  check(manager.deferPrefetchedDownload(later,Json::object(),FileInfoFolderChoice{})&&wait(manager,later)&&!fs::exists(later->target()),"Download Later leaves the plain form target unpublished");check(start(manager,later)&&exact(later,expected),"Later resumes plain saved bytes instead of submitting again");
  auto collision=make(manager,"collision");const auto sentinel=collision->target();fs::create_directories(sentinel.parent_path());atomicText(sentinel,"owned destination sentinel",false);
  check(start(manager,collision)&&str(collision->data,"Status")=="Failed"&&completedPostResponseCandidate(collision->data)&&readText(sentinel)=="owned destination sentinel","A destination collision preserves both the existing file and the complete form response");
  manager.configure(collision,{{"Folder",utf8((root/L"new-destination").wstring())}});check(start(manager,collision)&&exact(collision,expected)&&readText(sentinel)=="owned destination sentinel","Changing destination after failure publishes saved bytes without overwriting the old file");
  auto journal=make(manager,"journal");std::atomic_bool injected=false;
  CapturedResponseTestAccess::saveCheckpoint(manager,[&](const char* point){if(std::string_view(point)=="temporary-flushed"&&str(journal->data,"Status")=="Complete"&&!injected.exchange(true))throw std::runtime_error("Owned publication checkpoint failure");});
  check(start(manager,journal)&&injected&&str(journal->data,"Status")=="Failed"&&fs::exists(manager.root/L"publications"/(wide(journal->id())+L".json")),"A real catalog failure after publication retains its recovery journal");
  check(!yes(journal->data,"PostResponseComplete")&&!journal->data.contains("CompletedPrefetch")&&fileHash(journal->target())==expected,"Failure retention leaves pending publication recovery intact rather than creating a competing response receipt");
  CapturedResponseTestAccess::saveCheckpoint(manager,{});check(start(manager,journal)&&exact(journal,expected),"Resume completes pending publication recovery without another form submission");
  auto changed=make(manager,"changed");retain(manager,changed);auto req=readPostRequest(changed->data);req["body"]=b64(Bytes{'n','e','w'});changed->data["ProtectedRequest"]=protect(req.dump());manager.save();
  bool refused=false;try{transfer(manager,changed,std::make_shared<Cancel>());}catch(const std::exception& e){refused=std::string(e.what()).find("Nothing was submitted again")!=std::string::npos;}
  check(refused&&!changed->data.contains("CompletedPrefetch")&&!fs::exists(changed->target()),"Failure retention never rebinds an old response to a changed form request");
  auto damaged=make(manager,"damaged");retain(manager,damaged);const auto spool=manager.root/L"parts"/wide(damaged->id())/(wide(damaged->id())+L".data");
  {std::fstream f(spool,std::ios::binary|std::ios::in|std::ios::out);char b=0;f.read(&b,1);b^=1;f.seekp(0);f.write(&b,1);if(!f)throw std::runtime_error("Cannot modify owned damage fixture");}
  check(start(manager,damaged)&&str(damaged->data,"Status")=="Failed"&&yes(damaged->data,"PostAttempted")&&!damaged->data.contains("CompletedPrefetch"),"Damaged saved plain bytes fail Resume without becoming a new response receipt");
  check(start(manager,damaged)&&exact(damaged,expected),"A separate explicit Restart is required before resubmitting the damaged plain form");
  auto prefix=make(manager,"prefix");check(start(manager,prefix)&&str(prefix->data,"Status")=="Failed"&&yes(prefix->data,"ContentPrefixPending")&&completedPostResponseCandidate(prefix->data),"Inserted text before a plain form file waits for a choice with a retained response");
  prefix->data["StripContentPrefix"]=true;prefix->data["ContentPrefixPending"]=false;manager.save();
  check(manager.beginPrefetch(prefix)==PrefetchAdmission::Started&&wait(manager,prefix,true),"An approved prefix-removal choice verifies retained form bytes before Start");const auto prefixSize=num(prefix->data,"PrefetchedFinalSize",-1);
  check(prefixSize>0&&prefixSize<num(prefix->data,"Size")&&!fs::exists(prefix->target()),"Prefix removal reports final output size rather than retained response length");checks.back()["announcedBytes"]=prefixSize;
  check(manager.confirmPrefetchedDownload(prefix,Json::object())&&wait(manager,prefix)&&exact(prefix,pdfHash)&&static_cast<i64>(fs::file_size(prefix->target()))==prefixSize,"The published prefix-stripped form exactly matches pre-Start size and original file hash");
  i64 covered=0;for(const auto& part:prefix->data["Segments"])covered+=num(part,"Done");check(covered==prefixSize&&num(prefix->data,"Received")==prefixSize&&num(prefix->data,"HttpWireSize")>prefixSize,"Completed range display describes the final form file while preserving response byte counts");
  manager.stop();std::string id;const auto reopenedRoot=root/L"reopened"/L"state";
  {Manager first(reopenedRoot);configure(first);auto j=make(first,"reopened");retain(first,j);id=j->id();first.stop();}
  {Manager second(reopenedRoot);JobPtr j;for(auto item:second.jobs)if(item->id()==id)j=item;check(j&&completedPostResponseCandidate(j->data),"Plain form response receipt survives a catalog reopen");check(start(second,j)&&exact(j,expected),"Reopened catalog publishes retained plain form bytes without resubmission");second.stop();}
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
