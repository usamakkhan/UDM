#pragma once
#include "ExportScope.hpp"
static void startupExportChecks(const udm::fs::path& root,Fixture& fixture){
 using namespace udm;
 auto state=root/L"startup-export";std::string firstId,secondId,removedId,unsafeId,completeId,disabledId;
 check(!yes(defaultQueue(),"StartOnStartup"),"New queues require an explicit startup opt-in");
 {
  Manager m(state);auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"startup-files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["Retries"]=0;m.setSettings(prefs);
  auto q=defaultQueue();q["StartOnStartup"]="true";auto prior=m.snapshot();rejects([&]{m.setQueue(q);},"Queue startup setting rejects non-boolean input");check(m.snapshot()==prior,"Invalid startup preference leaves state unchanged");
  q["StartOnStartup"]=true;q["Enabled"]=false;q["Scheduled"]=true;q["Days"]=127;q["StartMinute"]=0;q["StopMinute"]=1;m.setQueue(q);
  auto other=defaultQueue("Not automatic");other["Enabled"]=false;m.setQueue(other);
  auto a=m.add(fixture.url("/range"),"","startup-a.bin");firstId=a->id();auto b=m.add(fixture.url("/range"),"","startup-b.bin");b->data["Status"]="Failed";secondId=b->id();
  auto removed=m.add(fixture.url("/range"),"","removed.bin");removed->data["QueueMember"]=false;removedId=removed->id();
  auto post=m.add(fixture.url("/range"),"","unsafe-post.bin");post->data["PostAttempted"]=true;unsafeId=post->id();
  auto complete=m.add(fixture.url("/range"),"","done.bin");complete->data["Status"]="Complete";complete->data["QueueMember"]=false;completeId=complete->id();
  auto disabled=m.add(fixture.url("/range"),"","off.bin","Not automatic");disabledId=disabled->id();m.save();
  check(str(a->data,"Status")=="Paused"&&!m.isActive(a),"Saving startup preference does not begin a download in the current session");
 }
 {
  Manager m(state);auto find=[&](const std::string& id){for(auto j:m.jobs)if(j->id()==id)return j;throw std::runtime_error("Missing startup fixture");};
  auto a=find(firstId),b=find(secondId);check(yes(m.state["Queues"][0],"StartOnStartup")&&!yes(m.state["Queues"][0],"Enabled"),"Startup preference persists independently of current queue enablement");
  auto before=m.snapshot();auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);rejects([&]{m.startQueuesOnStartup();},"Startup preparation reports a failed state write before any transfer");fs::remove(blocked);
  check(m.snapshot()==before&&!m.isActive(a)&&!m.isActive(b),"Failed startup preparation restores jobs and queue state atomically");
  m.startQueuesOnStartup();check(str(a->data,"Status")=="Queued"&&str(b->data,"Status")=="Queued"&&yes(m.state["Queues"][0],"Enabled"),"Startup prepares paused and failed members of opted-in queues");
  check(str(find(removedId)->data,"Status")=="Paused"&&str(find(unsafeId)->data,"Status")=="Paused"&&str(find(completeId)->data,"Status")=="Complete"&&str(find(disabledId)->data,"Status")=="Paused","Startup skips removed members, attempted nonreplayable requests, completed files and non-opted-in queues");
  m.pause(b);m.startQueuesOnStartup();check(str(b->data,"Status")=="Paused","Startup trigger is applied only once per application session");
  auto requests=fixture.requests.load();m.tick();for(int i=0;i<800&&m.isActive(a);++i)Sleep(10);m.tick();check(str(a->data,"Status")=="Complete"&&readText(a->target())==fixture.payload&&fixture.requests>requests,"Opted-in queue downloads exact bytes at startup outside its timed window");
  m.queueRun("Main queue",false);check(!yes(m.state["Queues"][0],"Enabled")&&yes(m.state["Queues"][0],"StartOnStartup"),"Stop suspends this run without erasing next-start preference");m.stop();
 }
 {
  Manager m(state);m.startQueuesOnStartup();JobPtr b;for(auto job:m.jobs)if(job->id()==secondId)b=job;check(b&&str(b->data,"Status")=="Queued","Next application session can restart a stopped opted-in queue");m.queueRun("Main queue",false);auto q=m.state["Queues"][0];q["StartOnStartup"]=false;m.setQueue(q);m.stop();
 }
 {
  Manager m(state);auto before=m.snapshot();m.startQueuesOnStartup();check(m.snapshot()==before&&!yes(m.state["Queues"][0],"Enabled"),"Clearing the startup preference preserves a stopped queue at the next launch");
 }
 {
  Manager legacy(root/L"startup-legacy");auto job=legacy.add(fixture.url("/range"),"","legacy.bin");legacy.state["Queues"][0].erase("StartOnStartup");legacy.save();Manager restored(legacy.root);restored.startQueuesOnStartup();check(str(restored.jobs[0]->data,"Status")=="Paused"&&!restored.state["Queues"][0].contains("StartOnStartup"),"Legacy queues without the setting remain unarmed and need no history migration");
 }
 {
  Manager m(root/L"export-scopes");auto q=defaultQueue("Other");q["Enabled"]=false;m.setQueue(q);
  auto a=m.add(fixture.url("/range?a"),utf8((root/L"export-files").wstring()),"a.bin");auto b=m.add(fixture.url("/range?b"),"","b.bin","Other");auto done=m.add(fixture.url("/range?done"),"","done.bin");done->data["Status"]="Complete";done->data["QueueMember"]=false;
  auto removed=m.add(fixture.url("/range?removed"),"","removed.bin");removed->data["QueueMember"]=false;auto recycled=m.add(fixture.url("/range?recycled"),"","recycled.bin");recycled->data["RecycledAt"]=date();recycled->data["QueueMember"]=false;
  std::set<std::string> selected{b->id(),done->id(),"stale-id"};auto all=exportScopeRows(m.jobs,selected,ExportScope::All);auto chosen=exportScopeRows(m.jobs,selected,ExportScope::Selected);auto queued=exportScopeRows(m.jobs,selected,ExportScope::Queue);auto other=exportScopeRows(m.jobs,selected,ExportScope::Queue,"Other");
  check(all.size()==5,"All-download scope retains the existing complete history export");check(chosen==std::vector<JobPtr>{b,done},"Selected scope preserves history order and ignores stale selection IDs");check(queued==std::vector<JobPtr>{a,b},"Queue scope uses pending membership rather than retained queue labels");check(other==std::vector<JobPtr>{b},"Named queue export excludes other queues");check(exportScopeRows(m.jobs,{},ExportScope::Selected).empty()&&exportScopeRows(m.jobs,selected,ExportScope::Queue,"Missing").empty(),"Empty selections and absent queues do not fall back to exporting everything");
  auto target=root/L"scope.txt";writeDownloadExport(m,other,target,false,true);check(readText(target)==str(b->data,"Url")+"\r\n","URL export writes only the checked scoped records");writeDownloadExport(m,chosen,root/L"scope.udmcatalog",true,false);auto catalog=Json::parse(readText(root/L"scope.udmcatalog"));check(catalog["Downloads"].size()==2&&catalog["Queues"].size()==2&&str(catalog,"Credentials")!="Windows account encrypted","Catalog export preserves scoped metadata and omits credentials by default");
  rejects([&]{writeDownloadExport(m,{},target,false,false);},"Empty export is rejected before replacing an output");check(readText(target)==str(b->data,"Url")+"\r\n","Rejected export preserves an existing output file");m.jobs.erase(std::find(m.jobs.begin(),m.jobs.end(),b));rejects([&]{writeDownloadExport(m,other,target,false,false);},"A record removed while the export dialog is open cannot be silently exported");rejects([&]{writeDownloadExport(m,{a,a},target,true,false);},"Duplicate export records are rejected");
  catalog["Queues"][0]["StartOnStartup"]=true;Manager imported(root/L"export-startup-import");importCatalog(imported,catalog,utf8((root/L"export-import-files").wstring()));bool armed=false;for(auto queue:imported.state["Queues"])armed|=yes(queue,"StartOnStartup");check(!armed,"Import cannot arm a startup queue from a catalog");
 }
}
