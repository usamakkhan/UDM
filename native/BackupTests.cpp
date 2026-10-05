#include "Backup.hpp"
#include <iostream>
using namespace udm;
static int passed=0,failed=0;
static void check(bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<"\n";(ok?passed:failed)++;}
template<class F>static void rejects(F f,const char* name){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,name);}
int wmain(int argc,wchar_t** argv){
 if(argc==3&&std::wstring(argv[1])==L"--junction"){
  try{Manager m(fs::absolute(argv[2]));backupInventory(m);std::cerr<<"Junction was followed\n";return 1;}
  catch(const std::exception& e){std::cout<<e.what()<<"\n";return std::string(e.what()).find("link or cloud placeholder")!=std::string::npos?0:1;}
 }
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root)){std::cerr<<"Choose a new test directory\n";return 2;}fs::create_directories(root);
 try{
 Manager m(root/L"source");auto p=m.state["Settings"];p["DownloadFolder"]=utf8((root/L"downloads").wstring());p["TemporaryFolder"]=utf8((root/L"temp").wstring());p["CategoryFolders"]=false;p["FontName"]="Segoe UI";m.setSettings(p);
 auto job=m.add("https://example.com/partial.zip","","partial.zip","Main queue",true,{{"Authorization","Bearer backup-fixture"}});
 job->data["PartsFolder"]=utf8((root/L"temp"/L"old-custom-parts").wstring());
 job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",9},{"Done",3}}});job->data["Received"]=3;
 fs::create_directories(root/L"temp"/L"old-custom-parts");writeBytes(root/L"temp"/L"old-custom-parts"/L"0000.part",Bytes{'a','b','c'});
 auto done=m.add("https://example.com/done.zip","","done.zip");fs::create_directories(done->target().parent_path());writeBytes(done->target(),Bytes{'d','o','n','e'});done->data["Status"]="Complete";done->data["Sha256"]=fileHash(done->target());
 auto missing=m.add("https://example.com/missing.zip","","missing.zip");missing->data["Status"]="Complete";
 auto video=std::make_shared<Job>(job->snapshot());video->data["Id"]=guid();video->data["FileName"]="video.part";video->data["Folder"]=utf8((root/L"media").wstring());video->data["PartsFolder"]=utf8((root/L"media-parts").wstring());job->video=video;
 fs::create_directories(root/L"media-parts");writeBytes(root/L"media-parts"/L"0000.part",Bytes{'v'});
 m.state["Queues"][0]["Description"]="retained schedule";m.state["BrowserCaptures"]=Json::object({{"receipt-fixture",{{"JobId",job->id()},{"State","accepted"}}}});
 m.save();auto before=m.snapshot();auto stateHash=fileHash(m.root/L"state.json");Cancel cancel;
 auto report=writeRecoveryImage(m,root/L"backup",cancel);auto verified=verifyRecoveryImage(root/L"backup");
 check(verified==report,"Published image verifies against its complete manifest");
 auto stateEntry=report["Files"][0];auto restoredState=Json::parse(readText(root/L"backup"/L"files"/wide(str(stateEntry,"Stored"))));
 check(restoredState==before,"Exact settings, queues, history, child tracks and receipts preserved");
 check(readHeaders(restoredState["Downloads"][0]).at("Authorization")=="Bearer backup-fixture"&&readText(root/L"backup"/L"files"/wide(str(stateEntry,"Stored"))).find("Bearer backup-fixture")==std::string::npos,"Credentials retain same-account encryption");
 auto has=[&](fs::path source){for(auto& f:report["Files"])if(backupKey(fs::path(wide(str(f,"Original"))))==backupKey(source))return true;return false;};
 check(has(root/L"temp"/L"old-custom-parts"/L"0000.part"),"Custom temporary partials included");
 check(has(root/L"media-parts"/L"0000.part"),"Nested video track partials included");
 check(has(done->target()),"Completed output included");
 check(report["MissingFiles"].size()==1&&report["MissingFiles"][0]==utf8(missing->target().wstring()),"Already missing completed output reported explicitly");
 check(m.snapshot()==before&&fileHash(m.root/L"state.json")==stateHash,"Backup does not mutate source state or save dirty changes");
 rejects([&]{writeRecoveryImage(m,root/L"backup",cancel);},"Existing backup cannot be overwritten");
 rejects([&]{writeRecoveryImage(m,m.root/L"nested",cancel);},"Backup inside source rejected");
 rejects([&]{writeRecoveryImage(m,root/L"temp"/L"old-custom-parts"/L"nested",cancel);},"Backup inside external resume data rejected");
 {
 Handle writer(CreateFileW(done->target().c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr));
 check((bool)writer,"Writer-conflict fixture opened");
 rejects([&]{writeRecoveryImage(m,root/L"locked",cancel);},"Concurrent source writer blocks backup");
 check(!fs::exists(root/L"locked"),"Locked-source failure publishes no backup");
 }
 {
 Cancel stop;stop.stop=true;rejects([&]{writeRecoveryImage(m,root/L"cancelled",stop);},"Pre-cancelled backup rejected");
 check(!fs::exists(root/L"cancelled"),"Cancellation publishes no image");
 }
 {
 Cancel stop;rejects([&]{writeRecoveryImage(m,root/L"mid-cancel",stop,[&](const char*){stop.stop=true;});},"Cancellation during copy rejected");
 check(!fs::exists(root/L"mid-cancel"),"Incomplete backup never has its final directory name");
 }
 {
 bool injected=false;rejects([&]{writeRecoveryImage(m,root/L"changed",cancel,[&](const char* point){if(!injected&&std::string(point)=="copied-file"){writeBytes(m.root/L"new-file",Bytes{'x'});injected=true;}});},"New source file during backup invalidates inventory");
 check(!fs::exists(root/L"changed"),"Changed inventory publishes no image");
 }
 {
  Manager active(root/L"active");auto j=active.add("http://127.0.0.1:9/no-network",utf8((root/L"active-files").wstring()),"paused.bin");
  {Lock guard(active.mutex);active.beginPrefetch(j);check(active.isActive(j),"Active-transfer guard fixture entered");
   rejects([&]{writeRecoveryImage(active,root/L"active-backup",cancel);},"Active transfer blocks backup before copying");active.pause(j);}
  active.endPrefetch(j);check(!fs::exists(root/L"active-backup"),"Active download refusal publishes no image");
 }
 {
  Cancel stop;rejects([&]{writeRecoveryImage(m,root/L"publish-cancel",stop,[&](const char* point){if(std::string(point)=="before-publish")stop.stop=true;});},"Cancellation immediately before publish honored");
  check(!fs::exists(root/L"publish-cancel"),"Final cancellation keeps image unpublished");
 }
 auto payload=root/L"backup"/L"files"/wide(str(report["Files"].back(),"Stored"));
 SetFileAttributesW(payload.c_str(),FILE_ATTRIBUTE_NORMAL);writeBytes(payload,Bytes{'b','a','d'});
 rejects([&]{verifyRecoveryImage(root/L"backup");},"Damaged payload rejected");
 auto traversal=report;traversal["Files"][0]["Stored"]="../state.json";atomicText(root/L"backup"/L"manifest.json",traversal.dump(),false);
 rejects([&]{verifyRecoveryImage(root/L"backup");},"Manifest traversal rejected");
 auto duplicate=report;duplicate["Files"].push_back(report["Files"][0]);atomicText(root/L"backup"/L"manifest.json",duplicate.dump(),false);
 rejects([&]{verifyRecoveryImage(root/L"backup");},"Duplicate manifest entry rejected");
 // Reparse points can be created without administrator rights using a junction by the outer runner.
 auto junction=root/L"source"/L"linked";
 if(fs::exists(junction))throw std::runtime_error("Unexpected junction fixture");
 check(m.snapshot()==before,"All failure paths leave in-memory catalog unchanged");
 Json result={{"passed",passed},{"failed",failed},{"scope","Recovery image creation and verification; restore/UI not yet implemented"}};
 atomicText(root/L"results.json",result.dump(2),false);
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
 std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
