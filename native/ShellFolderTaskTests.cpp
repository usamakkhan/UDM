#include "ShellFolderTask.hpp"
#include "third_party/json.hpp"
#include <fstream>
#include <atomic>
#include <iostream>
int wmain(int argc,wchar_t** argv){using namespace udm;namespace fs=std::filesystem;using Json=nlohmann::json;
 if(argc!=2||fs::exists(argv[1]))return 2;const fs::path root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array();std::string error;int exit=1;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  ShellFolderTask task;check(!task.valid()&&!task.ready(),"A fresh handoff has no active worker");
  bool rejected=false;try{task.start(L"relative");}catch(const std::exception&){rejected=true;}check(rejected&&!task.valid(),"Relative targets are rejected before worker admission");
  std::promise<void> entered,release;auto gate=release.get_future().share();auto running=entered.get_future();std::atomic_uint notifications=0;DWORD worker=0;fs::path target=root/L"Unicode 文件";
  task.start(target,[&]{++notifications;},[&](const fs::path& path){worker=GetCurrentThreadId();entered.set_value();if(gate.wait_for(std::chrono::seconds(5))!=std::future_status::ready)throw std::runtime_error("Owned launcher watchdog");if(path!=target)throw std::runtime_error("Wrong target");});
  // Always release the worker before any failing assertion can unwind it.
  const bool began=running.wait_for(std::chrono::seconds(2))==std::future_status::ready;const bool pending=task.valid()&&!task.ready();bool duplicate=false;try{task.start(target);}catch(const std::exception&){duplicate=true;}release.set_value();auto result=task.finish();
  check(began&&pending&&worker!=GetCurrentThreadId(),"A held Windows handoff leaves its caller free on a different thread");
  check(duplicate,"An active handoff rejects replacement instead of losing accepted work");check(result.path==target&&result.error.empty(),"The owned Unicode destination survives until joined acknowledgement");check(notifications==1&&!task.valid(),"Finish joins the notifier and retires exactly one result");
  task.start(target,[]{throw std::runtime_error("Owned notifier failure");},[](const fs::path&){throw std::runtime_error("Owned shell failure");});result=task.finish();check(result.path==target&&result.error=="Owned shell failure","Shell failure is returned despite a failed notification");
  task.start(target,{},[](const fs::path&){throw 7;});result=task.finish();check(!result.error.empty(),"Unknown launcher failures still produce an acknowledgement");
  task.start(target,{},[](const fs::path&){});const auto deadline=GetTickCount64()+2000;while(!task.ready()&&GetTickCount64()<deadline)Sleep(1);check(task.ready(),"Successful completion is pollable without a blocking wait");result=task.take();check(result.error.empty()&&!task.valid(),"Take joins and retires the successful worker");
  rejected=false;try{task.take();}catch(const std::exception&){rejected=true;}check(rejected,"Retired results cannot be taken twice");exit=0;
 }catch(const std::exception& e){error=e.what();}
 std::ofstream(root/L"results.json")<<Json{{"passed",exit==0},{"checks",checks},{"error",error}}.dump(2);return exit;
}
