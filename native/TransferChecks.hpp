#pragma once
#include "TransferFixture.hpp"

static void transferChecks(const fs::path& root){
 TransferFixture fixture(32*1024*1024,240);
 auto testRoot=root/L"dynamic-transfers";fs::create_directories(testRoot);
 auto expected=testRoot/L"expected.bin";fixture.expected(expected);auto hash=fileHash(expected);
 auto prefs=defaultSettings();Cancel cancel;
 {
  auto pool=std::make_shared<HttpSession>(prefs);auto before=fixture.connections.load();
  for(i64 first:{0LL,32LL}){
   Http response(fixture.url("/steady"),{},prefs,cancel,first,first+31,"",nullptr,true,pool);
   auto bytes=response.all(32,cancel);bool equal=bytes.size()==32;
   for(size_t i=0;i<bytes.size();++i)equal&=bytes[i]==(unsigned char)TransferFixture::value(first+(i64)i);
   check(equal,"Persistent HTTP request receives its exact range");
  }
  check(fixture.connections.load()-before==1,"Successive ranges reuse one actual TCP connection");
 }
 auto configure=[&](Manager& manager){manager.state["Settings"]["DownloadFolder"]=utf8(testRoot.wstring());manager.state["Settings"]["CategoryFolders"]=false;};
 {
  Manager manager(testRoot/L"failed-assembly-state");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","bad-hash.bin","Main queue",true,{},std::string(64,'0'));job->data["Connections"]=4;
  auto staging=fs::path(wide(str(job->data,"Folder")))/(L".udm-"+wide(job->id())+L".assembling");
  rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"A mismatched digest rejects an assembled download");
  auto data=manager.root/L"parts"/wide(job->id())/(wide(job->id())+L".data");
  check(fs::exists(data)&&fs::file_size(data)==(uintmax_t)fixture.size&&!fs::exists(staging)&&!fs::exists(job->target()),"Failed verification publishes nothing and retains the resumable output file");
 }
 {
  Manager manager(testRoot/L"missing-validator-state");configure(manager);
  auto job=manager.add(fixture.url("/missing-validator"),"","missing-validator.bin","Main queue",true);job->data["Connections"]=1;
  // Establish a saved range plan: the next response must retain its validator.
  job->data["Size"]=fixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",fixture.size-1},{"Done",0}}});
  rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"A ranged response without its established ETag is rejected");
  check(!fs::exists(job->target())&&num(job->data,"Received")==0,"Unverified range does not publish or retain mixed bytes");
 }
 {
  Manager manager(testRoot/L"complete-state");configure(manager);
  auto job=manager.add(fixture.url("/straggler"),"","split.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  // An older state backup may omit an orphan tail produced by a later split.
  job->data["Size"]=fixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["Segments"]=Json::array();
  for(int i=0;i<4;++i)job->data["Segments"].push_back({{"Index",i},{"Start",i*(fixture.size/4)},{"End",(i+1)*(fixture.size/4)-1},{"Done",0}});
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);writeBytes(folder/L"0004.part",Bytes(400000,'x'));
  transfer(manager,job,std::make_shared<Cancel>());
  check(num(job->data,"DynamicSplits")>0,"Idle workers split an active slow range");
  check(num(job->data,"Received")==fixture.size&&fileHash(job->target())==hash,"Dynamically split file preserves byte order and SHA-256");
  check(job->workers.size()==4,"Dynamic splitting respects configured connection worker limit");
 }
 {
  TransferFixture earlyFixture(32*1024*1024,240,20);
  Manager manager(testRoot/L"early-state");configure(manager);
  auto job=manager.add(earlyFixture.url("/straggler"),"","early-help.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  // Retain coverage of early help for a saved, older sixteen-chunk plan.
  job->data["Size"]=earlyFixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["Segments"]=Json::array();
  for(int i=0;i<16;++i)job->data["Segments"].push_back({{"Index",i},{"Start",i*(earlyFixture.size/16)},{"End",(i+1)*(earlyFixture.size/16)-1},{"Done",0}});
  transfer(manager,job,std::make_shared<Cancel>());
  auto requests=earlyFixture.rangeStarts();size_t helper=requests.size(),lastQueued=requests.size();
  // Four workers initially have sixteen 2 MiB chunks. A new request inside
  // the first chunk is help for its slow owner, observable at the server.
  for(size_t i=0;i<requests.size();++i){if(requests[i]>0&&requests[i]<2*1024*1024&&helper==requests.size())helper=i;if(requests[i]==30*1024*1024&&lastQueued==requests.size())lastQueued=i;}
  check(helper<lastQueued,"Server observes help for the slow range before the last queued chunk starts");
  check(fileHash(job->target())==hash,"Early redistribution produces the exact original file");
 }
 {
  TransferFixture freshFixture(32*1024*1024,240,8);
  Manager manager(testRoot/L"fresh-plan-state");configure(manager);
  auto job=manager.add(freshFixture.url("/steady"),"","fresh-plan.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  transfer(manager,job,std::make_shared<Cancel>());
  auto starts=freshFixture.rangeStarts();bool large=starts.size()>=4;
  if(large){auto first=std::vector<i64>(starts.begin(),starts.begin()+4);std::sort(first.begin(),first.end());large=first==std::vector<i64>{0,4*1024*1024,8*1024*1024,12*1024*1024};}
  check(large,"Fresh plan hands the first chunks to the workers front to back");
  check(fileHash(job->target())==hash,"Front-to-back chunks preserve exact completed bytes");
 }
 i64 retained=0;std::string pausedId;
 {
  Manager manager(testRoot/L"resume-state");configure(manager);
  auto job=manager.add(fixture.url("/straggler"),"","split-resume.bin","Main queue",true,{},hash);job->data["Connections"]=4;pausedId=job->id();
  auto stop=std::make_shared<Cancel>();
  auto running=std::async(std::launch::async,[&]{try{transfer(manager,job,stop);}catch(const Cancelled&){};});
  bool split=false;
  for(int i=0;i<1500;++i){Sleep(10);{Lock lock(manager.mutex);split=num(job->data,"DynamicSplits")>0;}if(split||running.wait_for(std::chrono::seconds(0))==std::future_status::ready)break;}
  stop->stop=true;running.get();retained=num(job->data,"Received");manager.save();
  check(split&&retained>0&&retained<fixture.size&&!fs::exists(job->target()),"Pause after a live split retains unfinished parts");
 }
 {
  Manager manager(testRoot/L"resume-state");JobPtr job;
  for(auto candidate:manager.jobs)if(candidate->id()==pausedId)job=candidate;
  check(job&&num(job->data,"Received")==retained,"Split ownership and byte count survive process-state reload");
  if(!job)throw std::runtime_error("Split resume job was lost.");
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&num(job->data,"Received")==fixture.size,"Reloaded split download resumes with exact SHA-256");
 }
 {
  // A crash leaves the catalog behind the file: bytes written after the last save
  // sit in the output file past each range's saved Done. Resume must trust the
  // saved Done values, re-request from them and overwrite in place: the served
  // bytes equal exactly what the catalog was missing and the file hashes exactly.
  TransferFixture killFixture(32*1024*1024,240,4);Json snapshot,savedSegments;i64 savedReceived=0,written=0;std::string killedId;
  {
   Manager manager(testRoot/L"kill-state");configure(manager);
   auto job=manager.add(killFixture.url("/steady"),"","killed.bin","Main queue",true,{},hash);job->data["Connections"]=4;killedId=job->id();
   auto stop=std::make_shared<Cancel>();
   auto running=std::async(std::launch::async,[&]{try{transfer(manager,job,stop);}catch(const Cancelled&){}});
   for(int i=0;i<1500&&snapshot.is_null();++i){Sleep(10);Lock lock(manager.mutex);if(num(job->data,"Received")>=4*1024*1024){snapshot=manager.snapshot();savedSegments=job->data["Segments"];savedReceived=num(job->data,"Received");}}
   for(int i=0;i<1500&&!snapshot.is_null();++i){Sleep(10);Lock lock(manager.mutex);if(num(job->data,"Received")>=savedReceived+2*1024*1024)break;}
   stop->stop=true;running.get();written=num(job->data,"Received");
  }
  // The process died here: the file holds `written` bytes, the catalog on disk knows `savedReceived`.
  if(!snapshot.is_null())atomicText(testRoot/L"kill-state"/L"state.json",snapshot.dump(),false);
  check(savedReceived>=4*1024*1024&&written>savedReceived,"Kill fixture leaves the output file ahead of the saved catalog");
  {
   Manager manager(testRoot/L"kill-state");JobPtr job;for(auto candidate:manager.jobs)if(candidate->id()==killedId)job=candidate;
   check(job&&num(job->data,"Received")==savedReceived&&job->data["Segments"]==savedSegments,"Reopened catalog carries the saved Done values, not the file size");
   if(!job)throw std::runtime_error("Kill-and-resume job was lost.");
   const auto servedBefore=killFixture.bodyBytes.load();const auto requestsBefore=killFixture.rangeStarts().size();
   transfer(manager,job,std::make_shared<Cancel>());
   const auto starts=killFixture.rangeStarts();std::set<i64> resumedAt(starts.begin()+(std::ptrdiff_t)requestsBefore,starts.end());bool fromSaved=true;
   for(const auto& s:savedSegments)if(num(s,"Done")<num(s,"End")-num(s,"Start")+1)fromSaved&=resumedAt.count(num(s,"Start")+num(s,"Done"))>0;
   check(fromSaved,"Resume requests every unfinished range from its saved Done offset");
   const auto served=killFixture.bodyBytes.load()-servedBefore,missing=killFixture.size-savedReceived;std::cout<<"  resume served "<<served<<" of "<<missing<<" missing bytes, splits "<<num(job->data,"DynamicSplits")<<"\n";
   // A dynamic split discards the bytes in flight past the new boundary, so the origin may serve slightly more than the gap; never less, and never the saved bytes again.
   check(served>=missing&&served<missing+4*1048576,"Resume fetches the bytes the saved catalog was missing and no saved bytes again");
   check(fileHash(job->target())==hash&&num(job->data,"Received")==killFixture.size&&str(job->data,"Status")=="Complete","Kill-and-resume on the single output file completes with the exact SHA-256");
  }
 }
 {
  // Reproduce a 0.8.0 fixed-chunk layout with one partial file already present.
  Manager manager(testRoot/L"legacy-state");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","legacy-resume.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  auto& data=job->data;data["Size"]=fixture.size;data["ETag"]="\"persistent-fixture-v1\"";data["RangeSupported"]=true;data["Segments"]=Json::array();
  constexpr i64 chunk=2*1024*1024;int id=0;
  for(i64 begin=0;begin<fixture.size;begin+=chunk)data["Segments"].push_back({{"Index",id++},{"Start",begin},{"End",std::min(begin+chunk-1,fixture.size-1)},{"Done",0}});
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);Bytes saved(256*1024);for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);writeBytes(folder/L"0000.part",saved);fixture.resumedPrefix=false;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&fixture.resumedPrefix,"Legacy fixed-chunk resume retains existing bytes");
  bool legacyLeft=false;for(const auto& entry:fs::directory_iterator(folder))legacyLeft|=entry.path().extension()==L".part";
  check(!legacyLeft,"Legacy part files are folded into the single output file and removed");
 }
 {
  // Every connection is reset after 2 MiB. Progress restores the retry budget, so
  // the download reconnects until the exact file completes instead of failing
  // after Retries drops per range.
  TransferFixture dropFixture(32*1024*1024,240,1);dropFixture.dropEvery=2*1024*1024;
  Manager manager(testRoot/L"drop-state");configure(manager);
  auto job=manager.add(dropFixture.url("/steady"),"","dropped.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  auto started=std::chrono::steady_clock::now();
  transfer(manager,job,std::make_shared<Cancel>());
  auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
  check(fileHash(job->target())==hash&&dropFixture.drops>=8,"Connections reset after every 2 MiB are reopened until the exact file completes");
  check(seconds<6,"Reconnection after progress does not wait out the retry backoff");
 }
 {
  TransferFixture dropFixture(32*1024*1024,240,1);dropFixture.dropEvery=2*1024*1024;
  Manager manager(testRoot/L"drop-no-retry-state");configure(manager);manager.state["Settings"]["Retries"]=0;
  auto job=manager.add(dropFixture.url("/steady"),"","dropped-no-retry.bin","Main queue",true,{},hash);job->data["Connections"]=2;
  rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"Retries=0 still fails a download whose connection is reset");
  check(!fs::exists(job->target()),"No file is published after a reset without retries");
 }
 {
  // A 1.5 s outage after 8 MiB: every connection is reset and new ones refused.
  // Workers that were receiving probe twice a second instead of waiting out the
  // 1, 2, 4 s retry backoff, so the download is back within a second of the network.
  TransferFixture outageFixture(32*1024*1024,240,1);outageFixture.outageAt=8*1024*1024;outageFixture.outageMs=1500;
  Manager manager(testRoot/L"outage-state");configure(manager);
  auto job=manager.add(outageFixture.url("/steady"),"","outage.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&str(job->data,"Status")=="Complete"&&outageFixture.rejected>0,"A download survives a 1.5 s outage that resets every connection and refuses new ones");
  check(outageFixture.recoveredAfterMs>=0&&outageFixture.recoveredAfterMs<1000,"Connections are back within a second of the outage ending (measured "+std::to_string(outageFixture.recoveredAfterMs.load())+" ms)");
 }
 {
  // The server accepts two concurrent connections; four workers are requested.
  // Refused workers hand their ranges to the accepted connections instead of
  // failing the download once their retry budget is spent.
  TransferFixture limitFixture(32*1024*1024,240,1);limitFixture.maxConnections=2;
  Manager manager(testRoot/L"limit-state");configure(manager);
  auto job=manager.add(limitFixture.url("/steady"),"","limited.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&str(job->data,"Status")=="Complete","A two-connection server limit completes the exact file with four workers requested");
  check(limitFixture.rejected>0&&num(job->data,"Received")==limitFixture.size,"Refused connections leave the download running on the accepted ones");
 }
}
