#include "HistoryReadSnapshot.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Json paused={{"Id","paused"},{"Status","Paused"},{"QueueMember",true},{"FileName","paused.bin"}};Json complete={{"Id","complete"},{"Status","Complete"},{"QueueMember",false},{"FileName","complete.bin"}};
  const auto idle=downloadListRow(paused,0,false,-1,{}),done=downloadListRow(complete,0,false,-1,{});const auto active=downloadListRow(paused,0,false,-1,idle,true,false);
  check(active!=idle&&active->active&&!idle->active&&active->data==idle->data,"Activity-only changes replace immutable rows without changing rendered metadata");
  check(downloadListRow(paused,0,false,-1,active,true,false)==active,"An unchanged activity snapshot retains shared row ownership");
  const auto prefetch=downloadListRow(paused,0,false,-1,active,true,true);check(prefetch!=active&&prefetch->prefetchDeferring&&!active->prefetchDeferring,"Prefetch-deferral-only changes cannot reuse a stale activity snapshot");
  check(downloadSelectionAvailable(DownloadSelectionAction::Stop,{active})&&!downloadSelectionAvailable(DownloadSelectionAction::Resume,{active}),"An active paused worker offers Stop without offering ordinary Resume");
  check(downloadSelectionAvailable(DownloadSelectionAction::Resume,{prefetch}),"A deferring paused prefetch retains its authoritative Resume exception");
  check(downloadSelectionAvailable(DownloadSelectionAction::Resume,{idle})&&!downloadSelectionAvailable(DownloadSelectionAction::Stop,{idle}),"An idle paused record offers Resume rather than Stop");
  auto queued=paused;queued["Status"]="Queued";const auto waiting=downloadListRow(queued,0,false,-1,{});
  check(downloadSelectionAvailable(DownloadSelectionAction::Stop,{waiting})&&!downloadSelectionAvailable(DownloadSelectionAction::Resume,{waiting}),"A queued record remains stoppable before a worker becomes active");
  check(!downloadSelectionAvailable(DownloadSelectionAction::Stop,{done})&&!downloadSelectionAvailable(DownloadSelectionAction::Resume,{done})&&downloadSelectionAvailable(DownloadSelectionAction::Remove,{done}),"An idle completed record permits Remove without Stop or Resume");
  check(downloadSelectionAvailable(DownloadSelectionAction::Stop,{active,done})&&!downloadSelectionAvailable(DownloadSelectionAction::Remove,{active,done}),"A mixed active and completed selection can Stop but cannot Remove the batch");
  check(downloadSelectionAvailable(DownloadSelectionAction::Resume,{idle,done})&&downloadSelectionAvailable(DownloadSelectionAction::Remove,{idle,done}),"An inactive mixed selection can Resume its unfinished member and Remove the batch");
  check(downloadSelectionAvailable(DownloadSelectionAction::Relocate,{done})&&!downloadSelectionAvailable(DownloadSelectionAction::Relocate,{idle,done}),"Move or rename remains restricted to an entirely completed inactive selection");
  check(downloadSelectionAvailable(DownloadSelectionAction::RemoveQueue,{idle})&&downloadSelectionAvailable(DownloadSelectionAction::Reorder,{idle})&&!downloadSelectionAvailable(DownloadSelectionAction::Reorder,{idle,idle}),"Queue membership and single-record reordering keep their existing constraints");
  check(!downloadSelectionAvailable(DownloadSelectionAction::RemoveQueue,{active})&&!downloadSelectionAvailable(DownloadSelectionAction::RemoveQueue,{done}),"Active records and completed nonmembers cannot leave a queue through stale advice");
  for(const auto action:{DownloadSelectionAction::Resume,DownloadSelectionAction::Stop,DownloadSelectionAction::Remove,DownloadSelectionAction::Relocate,DownloadSelectionAction::RemoveQueue,DownloadSelectionAction::Reorder})check(!downloadSelectionAvailable(action,{})&&!downloadSelectionAvailable(action,{nullptr}),"Empty or missing row snapshots never inherit another selection's action");
  Manager manager(root/L"catalog");auto modelData=paused;modelData["Id"]=guid();auto job=std::make_shared<Job>(modelData);job->data["OriginalResponse"]={{"State","Reserved"}};manager.jobs={job};HistoryReadRequest request;DownloadFilterCache cache;std::map<JobPtr,std::shared_ptr<const DownloadListRow>> records;
  auto first=tryHistoryReadSnapshot(manager,request,records,cache);check(first&&first->records.at(job)->active&&!first->records.at(job)->prefetchDeferring,"The model read captures authoritative activity even when status alone says Paused");
  job->prefetchDeferring=true;auto second=tryHistoryReadSnapshot(manager,request,first->records,cache);check(second->records.at(job)!=first->records.at(job)&&second->records.at(job)->prefetchDeferring,"The same model read captures prefetch behavior and invalidates the old row");
  job->data["OriginalResponse"]["State"]="Complete";job->prefetchDeferring=false;auto last=tryHistoryReadSnapshot(manager,request,second->records,cache);check(!last->records.at(job)->active&&first->records.at(job)->active,"Activity completion cannot mutate a previously displayed snapshot");manager.stop();
 }catch(const std::exception& failure){error=failure.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
