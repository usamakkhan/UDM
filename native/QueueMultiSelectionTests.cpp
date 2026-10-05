#include "QueueMembership.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
void check(bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});}
template<class F>void rejects(F f,const char*name){bool ok=false;try{f();}catch(...){ok=true;}check(ok,name);}
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;fs::path root=fs::absolute(argv[1]);if(fs::exists(root))return 3;fs::create_directories(root);CoInitializeEx(nullptr,COINIT_MULTITHREADED);std::string error;try{
 Manager m(root/L"data");m.state["Settings"]["PrefetchFileInfo"]=false;m.state["Settings"]["CategoryFolders"]=false;m.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());
 std::vector<JobPtr> j;for(int i=0;i<6;++i)j.push_back(m.add("http://127.0.0.1:9/"+std::to_string(i),"",std::to_string(i)+".bin"));
 for(auto p:j){p->data["QueueMember"]=true;p->data["Status"]="Queued";}j[5]->data["Queue"]="Other";
 auto reset=[&](){m.jobs={j[0],j[5],j[1],j[2],j[3],j[4]};m.save();};
 auto expect=[&](std::initializer_list<int> ids,const char*name){std::vector<JobPtr> e;for(int i:ids)e.push_back(j[i]);check(m.jobs==e,name);};
 reset();moveQueueSelection(m,{j[1],j[2]},-1);expect({1,5,2,0,3,4},"Up moves a contiguous selection together, leaving other queue slot unchanged");
 moveQueueSelection(m,{j[1],j[2]},-1);expect({1,5,2,0,3,4},"Up at top preserves selected order");
 moveQueueSelection(m,{j[1],j[2]},1);expect({0,5,1,2,3,4},"Down restores contiguous group");
 reset();moveQueueSelection(m,{j[1],j[3]},-1);expect({1,5,0,3,2,4},"Up moves separated selections one queue position");
 reset();moveQueueSelection(m,{j[1],j[3]},1);expect({0,5,2,1,4,3},"Down moves separated selections one queue position");
 reset();moveQueueSelection(m,{j[3],j[4]},1);expect({0,5,1,2,3,4},"Bottom group cannot reverse or move beyond end");
 reset();reorderQueueSelection(m,{j[3],j[1],j[1]},j[0]);expect({1,3,0,5,2,4},"Drag uses queue order, deduplicates selection and inserts before destination");
 reorderQueueSelection(m,{j[1],j[3]},j[3]);expect({1,3,0,5,2,4},"Drop on selected member is a no-op");
 reset();reorderQueueSelection(m,{j[1],j[3]});expect({0,5,2,4,1,3},"Drop after last row appends selected group");
 reset();rejects([&]{moveQueueSelection(m,{j[0],j[5]},1);},"Mixed queue selection rejected");
 rejects([&]{moveQueueSelection(m,{JobPtr{}},1);},"Missing selected job rejected");
 rejects([&]{moveQueueSelection(m,{j[1]},0);},"Invalid arrow direction rejected");
 rejects([&]{reorderQueueSelection(m,{j[1]},j[5]);},"Wrong queue drag destination rejected");
 j[2]->data["QueueMember"]=false;rejects([&]{moveQueueSelection(m,{j[1],j[2]},1);},"Removed selected member rejects whole batch");
 rejects([&]{reorderQueueSelection(m,{j[1]},j[2]);},"Removed destination rejected");j[2]->data["QueueMember"]=true;
 expect({0,5,1,2,3,4},"Invalid operations leave all jobs unchanged");
 auto tmp=root/L"data"/L"state.json.tmp";fs::create_directory(tmp);
 rejects([&]{moveQueueSelection(m,{j[1],j[2]},-1);},"Arrow save failure reported");expect({0,5,1,2,3,4},"Arrow save failure rolls back full order");
 rejects([&]{reorderQueueSelection(m,{j[1],j[3]},j[0]);},"Drag save failure reported");expect({0,5,1,2,3,4},"Drag save failure rolls back full order");
 auto a=j[1]->data,b=j[3]->data;rejects([&]{setQueueMembershipBatch(m,{j[1],j[3]},false);},"Removal save failure reported");check(j[1]->data==a&&j[3]->data==b,"Removal save failure restores every selected record");
 if(!fs::is_empty(tmp))throw std::runtime_error("Fault directory not empty");fs::remove(tmp);
 fs::create_directories(j[1]->target().parent_path());atomicText(j[1]->target(),"retained",false);
 setQueueMembershipBatch(m,{j[1],j[3]},false);check(!yes(j[1]->data,"QueueMember")&&!yes(j[3]->data,"QueueMember"),"Removal updates entire selection");check(str(j[1]->data,"Status")=="Paused"&&str(j[3]->data,"Status")=="Paused","Removed queued jobs become paused");check(fs::is_regular_file(j[1]->target())&&m.jobs.size()==6,"Removal retains downloaded file and catalog records");
 {Manager reopened(root/L"data");check(reopened.jobs.size()==6&&!yes(reopened.jobs[2]->data,"QueueMember")&&!yes(reopened.jobs[4]->data,"QueueMember"),"Removal persists across catalog reopen");}
 for(auto p:j)p->data["QueueMember"]=true;reset();moveQueueSelection(m,{j[1],j[2]},-1);{Manager reopened(root/L"data");check(reopened.jobs[0]->id()==j[1]->id()&&reopened.jobs[2]->id()==j[2]->id(),"Arrow group order persists across catalog reopen");}
 reset();auto beforeA=j[1]->data,beforeB=j[3]->data;fs::create_directory(tmp);
 rejects([&]{setQueueMembershipBatch(m,{j[1],j[3]},true,"Synchronization queue");},"Cross-queue save failure reported");check(j[1]->data==beforeA&&j[3]->data==beforeB,"Cross-queue failure restores every original membership");if(!fs::is_empty(tmp))throw std::runtime_error("Fault directory not empty");fs::remove(tmp);
 setQueueMembershipBatch(m,{j[1],j[3]},true,"Synchronization queue");check(str(j[1]->data,"Queue")=="Synchronization queue"&&str(j[3]->data,"Queue")=="Synchronization queue","Cross-queue batch changes both memberships");
 {Manager reopened(root/L"data");check(str(reopened.jobs[2]->data,"Queue")=="Synchronization queue"&&str(reopened.jobs[4]->data,"Queue")=="Synchronization queue","Cross-queue membership persists on reopen");}
 j[1]->data["Status"]="Complete";beforeA=j[1]->data;beforeB=j[3]->data;
 rejects([&]{setQueueMembershipBatch(m,{j[3],j[1]},true,"Main queue");},"Completed member prevents entire batch entering ordinary download queue");check(j[1]->data==beforeA&&j[3]->data==beforeB,"Completed-file rejection leaves earlier validated members unchanged");
 setQueueMembershipBatch(m,{j[1],j[3]},true,"Synchronization queue");check(str(j[1]->data,"Status")=="Complete"&&yes(j[1]->data,"QueueMember"),"Saved completed HTTP file may stay in synchronization queue");
 rejects([&]{setQueueMembershipBatch(m,{j[1],j[3]},true,"Removed queue");},"Stale queue destination rejected");
 }catch(const std::exception&e){error=e.what();}bool passed=error.empty();for(auto c:checks)passed&=yes(c,"passed");atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);CoUninitialize();return passed?0:1;}