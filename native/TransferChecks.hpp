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
  // Reproduce a 0.8.0 fixed-chunk layout with one partial file already present.
  Manager manager(testRoot/L"legacy-state");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","legacy-resume.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  auto& data=job->data;data["Size"]=fixture.size;data["ETag"]="\"persistent-fixture-v1\"";data["RangeSupported"]=true;data["Segments"]=Json::array();
  constexpr i64 chunk=2*1024*1024;int id=0;
  for(i64 begin=0;begin<fixture.size;begin+=chunk)data["Segments"].push_back({{"Index",id++},{"Start",begin},{"End",std::min(begin+chunk-1,fixture.size-1)},{"Done",0}});
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);Bytes saved(256*1024);for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);writeBytes(folder/L"0000.part",saved);fixture.resumedPrefix=false;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&fixture.resumedPrefix,"Legacy fixed-chunk resume retains existing bytes");
 }
}
