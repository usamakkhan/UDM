#include "Core.hpp"
#include "PropertiesRequestDraft.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);}
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);CoInitializeEx(nullptr,COINIT_MULTITHREADED);std::string error,active;Json cases=Json::array();
 const std::string emoji="🙂";const std::vector<std::pair<std::string,std::string>> names={
  {std::string(139,'a')+emoji+std::string(30,'b')+".mp4",std::string(139,'a')+".mp4"},
  {std::string(138,'a')+emoji+std::string(30,'b')+".mp4",std::string(138,'a')+emoji+".mp4"},
  {std::string(137,'a')+emoji+std::string(30,'b')+".mp4",std::string(137,'a')+emoji+"b.mp4"},
  {std::string(180,'a')+"."+std::string(17,'z')+emoji+std::string(20,'q'),std::string(140,'a')+"."+std::string(17,'z')},
  {std::string(139,'a')+emoji+std::string(30,'b')+"."+std::string(17,'z')+emoji+std::string(20,'q'),std::string(139,'a')+"."+std::string(17,'z')},
  {std::string(180,'a')+"."+std::string(16,'z')+emoji+std::string(20,'q'),std::string(140,'a')+"."+std::string(16,'z')+emoji}};
 try{for(size_t i=0;i<names.size();++i){
  const auto folder=root/std::to_wstring(i);const auto files=folder/L"f";fs::create_directories(files/L"a");fs::create_directories(files/L"b");const auto& input=names[i].first;const auto& expected=names[i].second;active=std::to_string(i)+"/manual";
  check(utf8(wide(input))==input&&wide(input).size()>160,"Regression input is a valid Unicode filename requiring truncation");
  std::string id,hash;fs::path target;
  {Manager m(folder/L"d");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((files/L"a").wstring());settings["CategoryFolders"]=false;settings["PrefetchFileInfo"]=false;settings["DuplicatePolicy"]="Numbered";m.setSettings(settings);
   auto job=m.add("https://fixture.invalid/manual/"+std::to_string(i),"",input,"Main queue",true);id=job->id();target=job->target();
   check(str(job->data,"FileName")==expected&&target.filename().wstring()==wide(expected),"Manual admission chooses the exact valid Unicode target");check(str(job->data,"Status")=="Paused"&&utf8(wide(expected))==expected,"Unicode normalization preserves paused admission and valid UTF-8");
   const std::string payload="owned Unicode target bytes";atomicText(target,payload,false);hash=fileHash(target);check(readText(target)==payload,"Windows creates and reads the exact normalized Unicode target");
   PropertiesHeaderPatch headers({},{});m.configureProperties(job,{{"FileName",expected},{"Description","Unicode properties accepted"}},headers);check(str(job->data,"Description")=="Unicode properties accepted"&&job->target()==target&&fileHash(target)==hash,"Properties accepts the normalized name without moving or changing existing bytes");
   active=std::to_string(i)+"/browser";settings["DownloadFolder"]=utf8((files/L"b").wstring());m.setSettings(settings);auto browser=m.receive({{"action","add"},{"url","https://fixture.invalid/browser/"+std::to_string(i)},{"filename",input},{"reportedServerName",input},{"downloadLater",true}});
   check(browser&&str(browser->data,"FileName")==expected&&str(browser->data,"BrowserReportedFileName")==expected,"Native browser-message admission normalizes both suggested and reported server names");check(str(browser->data,"Status")=="Paused"&&browser->target().parent_path()==files/L"b","Browser Download Later remains paused in the owned destination");
   const auto before=m.snapshot();const auto saved=readText(m.root/L"state.json");const auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);bool refused=false;try{m.add("https://fixture.invalid/refused/"+std::to_string(i),utf8((files/L"c").wstring()),input,"Main queue",true);}catch(...){refused=true;}fs::remove(blocked);
   check(refused&&m.snapshot()==before&&readText(m.root/L"state.json")==saved,"Actual catalog refusal rolls back Unicode admission without a partial saved row");m.stop();
  }
  active=std::to_string(i)+"/reload";{Manager m(folder/L"d");check(m.jobs.size()==2&&m.jobs[0]->id()==id&&str(m.jobs[0]->data,"FileName")==expected&&str(m.jobs[1]->data,"FileName")==expected,"Saved manual and browser Unicode names reload exactly");check(fileHash(target)==hash,"Reload leaves existing normalized output bytes exact");m.stop();}
  active=std::to_string(i)+"/raw-nested-load";const auto loaded=folder/L"l";fs::create_directories(loaded);auto row=Json{{"Id",guid()},{"FileName",input},{"Folder",utf8((files/L"a").wstring())},{"Url","https://fixture.invalid/raw"},{"Status","Paused"},{"Queue","Main queue"},{"Description","Raw Unicode metadata"},{"Segments",Json::array()}};auto child=row;child["Id"]=guid();row["Video"]=child;child["Id"]=guid();row["Audio"]=child;auto settings=defaultSettings();settings["DownloadFolder"]=utf8(files.wstring());settings["CategoryFolders"]=false;auto raw=Json{{"Schema",1},{"Settings",settings},{"Downloads",Json::array({row})},{"Queues",Json::array({defaultQueue()})}}.dump();atomicText(loaded/L"state.json",raw,false);
  {Manager m(loaded,Manager::StartupRecovery::Desktop);auto job=m.jobs.at(0);check(str(job->data,"FileName")==expected&&job->video&&job->audio&&str(job->video->data,"FileName")==expected&&str(job->audio->data,"FileName")==expected,"Raw catalog parent and nested audio/video names normalize without invalid surrogates");check(job->id()==str(row,"Id")&&str(job->data,"Description")=="Raw Unicode metadata","Raw catalog normalization preserves record identity and metadata");m.stop();check(readText(loaded/L"state.before-native.json")==raw,"First-load backup retains the original valid Unicode input byte for byte");}
  {Manager m(loaded);const auto saved=m.snapshot()["Downloads"].at(0);check(str(saved,"FileName")==expected&&str(saved["Video"],"FileName")==expected&&str(saved["Audio"],"FileName")==expected,"Normalized parent and nested track names remain stable through another save/load");m.stop();}
  cases.push_back({{"case",i},{"normalizedUnits",wide(expected).size()},{"existingOutputPreserved",true}});
 }}catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"case",active},{"error",error},{"checks",checks},{"cases",cases}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";CoUninitialize();return error.empty()?0:1;
}
