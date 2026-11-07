#include "Core.hpp"
#include <iostream>
using namespace udm;

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool condition,const char* name){
  checks.push_back({{"name",name},{"passed",condition}});
  if(!condition)throw std::runtime_error(name);
 };
 try{
  Manager manager(root/L"data");manager.save();
  const auto file=manager.root/L"state.json";
  auto awaitSaved=[&](const std::function<bool(const Json&)>& matches){
   const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
   do{if(matches(Json::parse(readText(file))))return true;Sleep(10);}
   while(std::chrono::steady_clock::now()<deadline);
   return false;
  };
  const auto original=readText(file);const auto snapshot=manager.snapshot();
  const auto stamp=fs::last_write_time(file)-std::chrono::hours(24);
  fs::last_write_time(file,stamp);
  for(int i=0;i<12;++i)manager.tick();
  check(manager.snapshot()==snapshot,"Idle ticks preserve a newly initialized catalog");
  check(readText(file)==original&&fs::last_write_time(file)==stamp,"Idle ticks do not rewrite or flush the catalog");
  // A finished daily cycle does have state to clear. The fix must retain this
  // transition, rather than merely suppressing checkpoints for idle queues.
  {Lock lock(manager.mutex);manager.state["Queues"][0]["DailyRunUntilComplete"]=true;manager.save();}
  for(int i=0;i<3;++i)manager.tick();
  check(!yes(manager.snapshot()["Queues"][0],"DailyRunUntilComplete"),"Finished daily cycle clears its running flag");
  check(awaitSaved([](const Json& saved){return !yes(saved["Queues"][0],"DailyRunUntilComplete");}),"Finished daily cycle is durably checkpointed");
  const auto settled=readText(file);
  fs::last_write_time(file,stamp);
  for(int i=0;i<12;++i)manager.tick();
  check(readText(file)==settled&&fs::last_write_time(file)==stamp,"Settled cycle returns to write-free idle operation");
  // Nested media metadata is serialized through Job::snapshot rather than
  // directly from the parent data object.
  auto media=std::make_shared<Job>(Json{{"Id",std::string(32,'a')},{"FileName","media.bin"},{"Status","Complete"},
   {"Video",Json{{"Id",std::string(32,'b')},{"FileName","video.bin"},{"Status","Complete"}}},
   {"Audio",Json{{"Id",std::string(32,'c')},{"FileName","audio.bin"},{"Status","Complete"}}}});
  {Lock lock(manager.mutex);manager.jobs.push_back(media);manager.save();}
  {Lock lock(manager.mutex);media->video->data["Description"]="updated video";}
  check(awaitSaved([](const Json& saved){return str(saved["Downloads"][0]["Video"],"Description")=="updated video";}),
   "Nested media changes are checkpointed without a desktop tick");
  {Lock lock(manager.mutex);media->audio->data["Description"]="updated audio";}
  check(awaitSaved([](const Json& saved){return str(saved["Downloads"][0]["Audio"],"Description")=="updated audio";}),
   "The worker continues checkpointing without desktop ticks");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 return error.empty()?0:1;
}
