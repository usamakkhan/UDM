#include "Core.hpp"
#include <iostream>
namespace udm {struct CatalogCheckpointTestAccess {static void observe(Manager& m,std::function<void(const char*)> callback){Lock lock(m.mutex);m.catalogCheckpoint=std::move(callback);}};}
using namespace udm;
int wmain(int argc,wchar_t** argv){if(argc!=4)return 2;const std::wstring command=argv[1];const auto folder=fs::absolute(argv[2]);const std::string point=utf8(argv[3]);try{
 if(command==L"--seed"){
  if(fs::exists(folder))return 2;fs::create_directories(folder);atomicText(folder/L".udm-metadata-recovery-owned","Owned process crash fixture",false);Manager manager(folder/L"catalog",Manager::StartupRecovery::Desktop);auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);auto job=manager.add("https://example.invalid/metadata.bin","","output.bin","Main queue",true);job->data["Size"]=14;job->data["Received"]=14;manager.save();auto stage=job->target().parent_path()/(L".udm-"+wide(job->id())+L".assembling");fs::create_directories(stage.parent_path());atomicText(stage,"verified bytes",false);const auto hash=fileHash(stage);atomicText(folder/L"request.json",Json{{"Id",job->id()},{"Stage",utf8(stage.wstring())},{"Target",utf8(job->target().wstring())},{"Hash",hash}}.dump(),false);CatalogCheckpointTestAccess::observe(manager,[point](const char* phase){if(point==phase){TerminateProcess(GetCurrentProcess(),197);ExitProcess(198);}});manager.publishFile(job,stage,hash);return 9;
 }
 if(command==L"--recover"){
  auto request=Json::parse(readText(folder/L"request.json"));Manager manager(folder/L"catalog",Manager::StartupRecovery::Desktop);auto job=manager.jobs.at(0);manager.recoverReplacements(job);const bool committed=point!="publication-intent-temporary-flushed";auto target=fs::path(wide(str(request,"Target"))),stage=fs::path(wide(str(request,"Stage")));bool good;
  if(committed)good=str(job->data,"Status")=="Complete"&&fs::exists(target)&&fs::file_size(target)==14&&fileHash(target)==str(request,"Hash")&&!fs::exists(stage);
  else good=str(job->data,"Status")!="Complete"&&!fs::exists(target)&&fs::exists(stage)&&fileHash(stage)==str(request,"Hash");manager.stop();const auto saved=Json::parse(readText(folder/L"catalog/state.json"));good=good&&((str(saved["Downloads"][0],"Status")=="Complete")==committed);atomicText(folder/L"results.json",Json{{"passed",good},{"point",point},{"expectedCommitted",committed},{"status",str(job->data,"Status")},{"physicalCatalogBytes",fs::file_size(folder/L"catalog/state.json")}}.dump(2),false);return good?0:1;
 }
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 2;}
