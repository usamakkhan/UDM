#include "DownloadListTooltip.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 try{
  Manager manager(root/L"catalog");const auto name=str(manager.state["Queues"][0],"Name");auto before=navigationTreeSnapshot(manager);
  Json live={{"Id",guid()},{"FileName","cached.bin"},{"Queue",name},{"QueueMember",true},{"Status","Paused"}};
  const auto row=downloadListRow(live,0,false,-1,{});const auto expected="cached.bin\n"+queueIndicatorDescription(manager.state["Queues"][0],queueDisplayLabel(before.queueLabels,name));
  check(downloadListTooltip(row,before)==expected,"Tooltip retains the reference filename and friendly queue description");
  live["FileName"]="changed.bin";manager.state["Queues"][0]["Enabled"]=false;manager.state["Queues"][0]["Scheduled"]=true;
  check(downloadListTooltip(row,before)==expected,"Later mutable job and queue edits cannot alter the existing UI snapshot");
  const auto after=navigationTreeSnapshot(manager);
  check(before.sameStructure(after)&&before.queueDescriptions!=after.queueDescriptions,"Queue descriptions refresh even when the tree structure is unchanged");
  const auto updated=downloadListRow(live,0,false,-1,row);const auto text=downloadListTooltip(updated,after);
  check(text.find("changed.bin\n")==0&&text.find("; scheduled start")!=std::string::npos&&text.find("; stopped")!=std::string::npos,"A new snapshot supplies the current filename and queue status");
  live["QueueMember"]=false;check(downloadListTooltip(downloadListRow(live,0,false,-1,{}),after)=="changed.bin\nNot in a queue","Explicit nonmembers retain the existing tooltip wording");
  live["QueueMember"]=true;live["Queue"]="missing";check(downloadListTooltip(downloadListRow(live,0,false,-1,{}),after)=="changed.bin\nNot in an existing queue","A missing queue retains its existing explanation");
  live.erase("QueueMember");live["Status"]="Complete";check(!downloadListQueueMember(live),"Legacy completed rows remain outside a queue by default");
  live["Status"]="Paused";check(downloadListQueueMember(live),"Legacy unfinished rows retain default queue membership");
  check(downloadListTooltip({},after).empty(),"Missing row snapshots produce no stale tooltip");
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"checks",checks},{"error",error}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
