#include "Restore.hpp"
#include "TransferFixture.hpp"
#include <iostream>
using namespace udm;
static int passed=0,failed=0;
static void check(bool ok,const char* name){std::cout<<(ok?"PASS ":"FAIL ")<<name<<"\n";(ok?passed:failed)++;}
template<class F>static void rejects(F f,const char* name){bool rejected=false;try{f();}catch(const std::exception& e){std::cout<<"  "<<e.what()<<"\n";rejected=true;}check(rejected,name);}
struct Fixture{
 fs::path root,image;Json manifest,state;std::set<std::string> approved;
 Fixture(fs::path p,bool large=false):root(p),image(p/L"image"){
  fs::create_directories(root);{
   Manager m(root/L"data");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"outputs").wstring());prefs["CategoryFolders"]=false;prefs["FontName"]="Verdana";m.setSettings(prefs);
   auto job=m.add("https://example.com/file.zip","","file.zip","Main queue",true,{{"Authorization","Bearer restore-test"}});
   job->data["Status"]="Complete";fs::create_directories(job->target().parent_path());writeBytes(job->target(),large?Bytes(16*1024*1024,0x65):Bytes{'e','x','a','c','t'});
   auto parts=m.root/L"parts"/wide(job->id());fs::create_directories(parts);writeBytes(parts/L"0000.part",Bytes{'p','a','r','t'});
   fs::create_directories(m.root/L"empty"/L"nested");
   m.save();state=m.snapshot();Cancel c;manifest=writeRecoveryImage(m,image,c);
  }
  approved=recoveryDestinations(manifest);
 }
 void change(){for(auto& row:manifest["Files"]){auto p=fs::path(wide(str(row,"Original")));SetFileAttributesW(p.c_str(),FILE_ATTRIBUTE_NORMAL);writeBytes(p,Bytes{'b','e','f','o','r','e'});}}
 bool oldIntact(){for(auto& row:manifest["Files"])if(readText(fs::path(wide(str(row,"Original"))))!="before")return false;return true;}
};
int wmain(int argc,wchar_t** argv){
 if(argc==3&&std::wstring(argv[1])==L"--resume"){
  auto root=fs::absolute(argv[2]);if(fs::exists(root))return 2;fs::create_directories(root);
  WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))return 6;
  TransferFixture server(4*1024*1024,0,0);server.expected(root/L"expected.bin");auto hash=fileHash(root/L"expected.bin");fs::path part;Json manifest;
  {Manager m(root/L"data");auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"downloads").wstring());prefs["CategoryFolders"]=false;prefs["ProxyMode"]="Connect directly";m.setSettings(prefs);
   auto job=m.add(server.url("/steady"),"","resume.bin","Main queue",true,{},hash);job->data["Size"]=server.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["RangeSupported"]=true;job->data["Received"]=262144;
   job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",server.size-1},{"Done",262144}}});
   part=m.root/L"parts"/wide(job->id())/L"0000.part";fs::create_directories(part.parent_path());Bytes prefix(262144);for(size_t i=0;i<prefix.size();++i)prefix[i]=(unsigned char)TransferFixture::value((i64)i);writeBytes(part,prefix);m.save();Cancel c;manifest=writeRecoveryImage(m,root/L"image",c);
  }
  writeBytes(part,Bytes{'l','o','s','t'});std::set<std::string> approved;approved=recoveryDestinations(manifest);
  Cancel c;restoreRecoveryImage(root/L"image",approved,root/L"transaction",c);
  {Manager m(root/L"data");transfer(m,m.jobs[0],std::make_shared<Cancel>());check(server.resumedPrefix,"Restored partial download resumes with an HTTP range");check(fileHash(m.jobs[0]->target())==hash,"Resumed restored download completes with exact expected bytes");}
  atomicText(root/L"results.json",Json{{"passed",passed},{"failed",failed},{"requests",server.requests.load()}}.dump(2),false);return failed?1:0;
 }
 if(argc==4&&std::wstring(argv[1])==L"--crash"){
  auto point=utf8(argv[3]);Fixture f(fs::absolute(argv[2]),point=="copy-progress");f.change();if(point.rfind("directory-",0)==0){fs::remove(f.root/L"data"/L"empty"/L"nested");fs::remove(f.root/L"data"/L"empty");}Cancel c;
  restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(point==at)ExitProcess(73);});return 3;
 }
 if(argc==3&&std::wstring(argv[1])==L"--recover"){
  auto root=fs::absolute(argv[2]);auto manifest=verifyRecoveryImage(root/L"image");std::set<std::string> approved;
  approved=recoveryDestinations(manifest);
  auto journal=rollbackRecovery(root/L"transaction"/L"restore.json",approved);
  for(auto& row:manifest["Files"])if(readText(fs::path(wide(str(row,"Original"))))!="before")return 4;
  std::cout<<"PASS Process-interrupted restore rolled back every original\n";return str(journal,"Status")=="Rolled back"?0:5;
 }
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 try{
 Cancel c;
 {Fixture f(root/L"roundtrip");f.change();auto result=restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c);
 check(str(result,"Status")=="Committed","Restore commits after verification");
 bool exact=true,retained=true;for(auto& row:result["Files"]){exact&=fileHash(fs::path(wide(str(row,"Target"))))==str(row,"After");retained&=readText(fs::path(wide(str(row,"Old"))))=="before";}
 check(exact,"Every restored file matches the backup hash");check(retained,"Pre-restore files retained for recovery");
 {Manager m(f.root/L"data");auto expected=f.state;for(auto& job:expected["Downloads"])job["ConfirmationPending"]=false;check(m.snapshot()==expected,"Reopened manager preserves catalog with normal startup confirmation reset");check(readHeaders(m.jobs[0]->data).at("Authorization")=="Bearer restore-test","Restored encrypted credential decrypts in original account");}
 check(str(rollbackRecovery(f.root/L"transaction"/L"restore.json",f.approved),"Status")=="Committed","Committed transaction cannot be accidentally rolled back");
 }
 for(auto point:{"file-staged","original-moved","file-installed","state-installed"}){
  Fixture f(root/wide(point));f.change();
  rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)==point)throw std::runtime_error("Injected interruption");});},"Injected restore interruption reported");
  check(f.oldIntact(),"All originals recovered after an interrupted apply");
  check(str(Json::parse(readText(f.root/L"transaction"/L"restore.json")),"Status")=="Rolled back","Rollback journal records completion");
  check(str(rollbackRecovery(f.root/L"transaction"/L"restore.json",f.approved),"Status")=="Rolled back","Repeated rollback is idempotent");
 }
 {Fixture f(root/L"cancel");f.change();Cancel stop;rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",stop,[&](const char* phase){if(std::string(phase)=="file-installed")stop.stop=true;});},"Cancellation during apply reported");check(f.oldIntact(),"Cancellation restores all originals");}
 {Fixture f(root/L"new-file");auto path=fs::path(wide(str(f.manifest["Files"].back(),"Original")));auto exact=fileHash(path);fs::remove(path);
 restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c);check(fileHash(path)==exact,"Restore recreates a missing file");}
 {Fixture f(root/L"missing-rollback");f.change();auto path=fs::path(wide(str(f.manifest["Files"][1],"Original")));fs::remove(path);
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)=="file-installed")throw std::runtime_error("Stop after new file");});},"Restore of initially missing file can roll back");
 check(!fs::exists(path),"Rollback removes only its newly restored file");
 }
 {Fixture f(root/L"copy-cancel",true);f.change();Cancel stop;bool mid=false;
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",stop,[&](const char* at){if(std::string(at)=="copy-progress"){mid=true;stop.stop=true;}});},"Cancellation inside large restore copy reported");
 check(mid&&f.oldIntact(),"In-copy cancellation preserves every original");
 check(str(Json::parse(readText(f.root/L"transaction"/L"restore.json")),"Status")=="Rolled back","In-copy cancellation completes rollback");
 }
 {Fixture f(root/L"readonly");f.change();for(auto& row:f.manifest["Files"]){auto payload=f.image/L"files"/wide(str(row,"Stored"));SetFileAttributesW(payload.c_str(),FILE_ATTRIBUTE_READONLY);}
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)=="state-installed")throw std::runtime_error("Read-only rollback");});},"Read-only restored files can be rolled back");
 check(f.oldIntact(),"Read-only restore failure recovers original files");
 }
 {Fixture f(root/L"unapproved");f.change();auto wrong=f.approved;wrong.erase(wrong.begin());rejects([&]{restoreRecoveryImage(f.image,wrong,f.root/L"transaction",c);},"Unreviewed destination set rejected");check(f.oldIntact()&&!fs::exists(f.root/L"transaction"),"Destination review failure writes nothing");}
 {Fixture f(root/L"locked");f.change();auto path=fs::path(wide(str(f.manifest["Files"].back(),"Original")));Handle lease(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c);},"Locked target restore fails");check(f.oldIntact(),"Locked target failure preserves original data");}
 {Fixture f(root/L"damaged");f.change();auto path=f.image/L"files"/wide(str(f.manifest["Files"][0],"Stored"));writeBytes(path,Bytes{'b','a','d'});rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c);},"Corrupt image rejected before restore");check(f.oldIntact()&&!fs::exists(f.root/L"transaction"),"Corrupt image cannot change existing files");}
 {Fixture f(root/L"edited-during-rollback");f.change();std::string changed;
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){
  if(std::string(at)=="file-installed"){
   auto j=Json::parse(readText(f.root/L"transaction"/L"restore.json"));
   changed=str(j["Files"][0],"Target");writeBytes(fs::path(wide(changed)),Bytes{'e','d','i','t'});throw std::runtime_error("Injected external edit");
  }
 });},"Rollback refuses a later edit");
 check(!changed.empty()&&readText(fs::path(wide(changed)))=="edit","Rollback does not discard newer file contents");
 auto j=Json::parse(readText(f.root/L"transaction"/L"restore.json"));auto row=j["Files"][0];
 check(readText(fs::path(wide(str(row,"Old"))))=="before","Original retained when rollback requires attention");
 }

 {Fixture f(root/L"empty-roundtrip");auto folder=f.root/L"data"/L"empty"/L"nested";fs::remove(folder);fs::remove(folder.parent_path());
 restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c);
 check(fs::is_directory(folder)&&fs::is_empty(folder),"Restore recreates nested empty folders");}
 for(auto point:{"directory-created","directory-recorded","file-staged"}){
  Fixture f(root/wide(std::string("folder-")+point));auto folder=f.root/L"data"/L"empty";fs::remove(folder/L"nested");fs::remove(folder);
  rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)==point)throw std::runtime_error("Directory interruption");});},"Directory interruption rolls back");
  check(std::string(point)=="directory-created"?fs::is_directory(folder):!fs::exists(folder),"Unrecorded folders retained; recorded empty folders removed");
  check(fs::is_directory(f.root/L"data"),"Pre-existing folder survives rollback");
 }
 {Fixture f(root/L"folder-new-content");auto folder=f.root/L"data"/L"empty"/L"nested";fs::remove(folder);
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)=="directory-recorded"){writeBytes(folder/L"user.txt",Bytes{'u'});throw std::runtime_error("External file created");}});},"Cancel after external file creation reported");
 check(readText(folder/L"user.txt")=="u","Rollback preserves new files inside created folder");}
 {Fixture f(root/L"folder-substitution");auto folder=f.root/L"data"/L"empty"/L"nested";fs::remove(folder);
 rejects([&]{restoreRecoveryImage(f.image,f.approved,f.root/L"transaction",c,[&](const char* at){if(std::string(at)=="directory-recorded"){fs::rename(folder,folder.parent_path()/L"retained-original");fs::create_directory(folder);throw std::runtime_error("Folder replaced");}});},"Substituted directory cancellation reported");
 check(fs::is_directory(folder),"Rollback preserves substituted folder identity");}
 {Fixture f(root/L"legacy-image");auto legacy=f.manifest;legacy.erase("Directories");atomicText(f.image/L"manifest.json",legacy.dump(),false);
 restoreRecoveryImage(f.image,recoveryDestinations(legacy),f.root/L"transaction",c);
 check(str(Json::parse(readText(f.root/L"transaction"/L"restore.json")),"Status")=="Committed","Legacy images without directory inventory remain restorable");}
 for(auto invalid:{"relative","duplicate","file-conflict"}){
  Fixture f(root/wide(std::string("invalid-folder-")+invalid));auto manifest=f.manifest;
  if(std::string(invalid)=="relative")manifest["Directories"].push_back("relative");
  if(std::string(invalid)=="duplicate")manifest["Directories"].push_back(manifest["Directories"][0]);
  if(std::string(invalid)=="file-conflict")manifest["Directories"].push_back(str(manifest["Files"][0],"Original"));
  atomicText(f.image/L"manifest.json",manifest.dump(),false);
  rejects([&]{verifyRecoveryImage(f.image);},"Invalid directory inventory rejected");
 }
 atomicText(root/L"results.json",Json{{"passed",passed},{"failed",failed}}.dump(2),false);
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
 std::cout<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
