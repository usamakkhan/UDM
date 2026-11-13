#include "FileInfoDecision.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static bool ready(FileInfoDecisionTask& task){const auto end=GetTickCount64()+6000;while(!task.ready()&&GetTickCount64()<end)Sleep(5);return task.ready();}
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);std::string error;
 try{
  Manager manager(root/L"state");auto settings=manager.state["Settings"];settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((root/L"files").native());settings["PrefetchFileInfo"]=false;manager.setSettings(settings);
  FileInfoDecisionTask missing;bool emptyRejected=false;try{missing.startAction(manager,{});}catch(const std::exception&){emptyRejected=true;}
  check(emptyRejected&&!missing.valid()&&!manager.fileInfoDecisionPending,"An empty prepared action cannot claim or leak the Manager guard");
  auto job=manager.add("https://files.example.test/file",utf8((root/L"files").native()),"before.bin","Main queue",true);
  FileInfoDecisionDraft draft;draft.media=false;draft.loginUnchanged=true;draft.edit={{"Folder",utf8((root/L"files").native())},{"FileName","after.bin"},{"Category","Other"},{"Description","Owned input"},{"Queue","Main queue"}};
  FileInfoDecisionTask task;task.start(manager,job,draft);draft.edit["Description"]="Mutated outside worker";
  check(ready(task),"The owned decision worker reaches completion");check(manager.fileInfoDecisionPending&&task.valid(),"A completed worker retains the presentation guard until UI acknowledgement");
  FileInfoDecisionTask competing;bool blocked=false;try{competing.start(manager,job,draft);}catch(const std::exception&){blocked=true;}
  check(blocked&&manager.fileInfoDecisionPending,"Another task cannot claim or clear an outstanding presentation guard");
  check(task.get().warning.empty(),"An ordinary Download Later produces no spurious queue warning");
  check(str(job->data,"Description")=="Owned input"&&str(job->data,"FileName")=="after.bin"&&str(job->data,"Status")=="Paused","The worker applies one owned draft and leaves Download Later paused");
  task.release();check(!manager.fileInfoDecisionPending&&!task.valid(),"UI acknowledgement joins the worker and clears its guard");
  auto other=manager.add("https://files.example.test/other",utf8((root/L"files").native()),"owned.bin","Main queue",true);draft.edit["FileName"]="owned.bin";
  FileInfoDecisionTask failing;failing.start(manager,job,draft);check(ready(failing),"Validation failure completes its future without blocking the caller");
  bool rejected=false;try{failing.get();}catch(const std::exception& e){rejected=std::string(e.what())=="That destination belongs to another download.";}
  check(rejected&&manager.fileInfoDecisionPending,"The original backend validation error is delivered while the UI guard remains held");
  check(str(job->data,"FileName")=="after.bin"&&str(job->data,"Description")=="Owned input"&&manager.jobs.size()==2&&str(other->data,"FileName")=="owned.bin","A rejected command preserves both downloads without applying a mixed draft");
  failing.release();check(!manager.fileInfoDecisionPending,"Failure acknowledgement also joins and releases the task");
  draft.edit["FileName"]="sync.bin";applyFileInfoDecision(manager,job,draft);check(!manager.fileInfoDecisionPending&&str(job->data,"FileName")=="sync.bin","Direct command execution reuses the same decision semantics without claiming a UI task");
  draft.edit["FileName"]="joined.bin";{FileInfoDecisionTask abandoned;abandoned.start(manager,job,draft);}
  check(!manager.fileInfoDecisionPending&&str(job->data,"FileName")=="joined.bin","Destroying an unconsumed task joins its owned worker before releasing the Manager guard");manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
