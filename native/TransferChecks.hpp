#pragma once
#include "CapturedResponseTestAccess.hpp"
#include "TransferFixture.hpp"
#include <winioctl.h>

static void deferredDateRangeChecks(const fs::path& root){
 const i64 size=480*1024,resumeOffset=262144-64;
 for(const std::string mode:{"complete","resume","changed","weak"}){
  TransferFixture fixture(size,0,5);
  Manager manager(root/wide("date-only-"+mode));
  auto prefs=manager.state["Settings"];
  prefs["ProxyMode"]="Connect directly";
  prefs["Connections"]=1;
  prefs["Retries"]=2;
  manager.setSettings(prefs);
  const auto expected=manager.root/L"expected.bin";
  fixture.expected(expected);
  const auto digest=fileHash(expected);
  const auto route=mode=="complete"?"/date-only":"/date-only-"+mode;
  auto job=manager.add(fixture.url(route.c_str()),utf8(manager.root.wstring()),"download.bin","Main queue",true,{},digest);
  transfer(manager,job,std::make_shared<Cancel>());
  const auto starts=mode=="complete"?std::vector<i64>{0}:mode=="resume"?std::vector<i64>{0,resumeOffset}:
   std::vector<i64>{0,resumeOffset,0};
  check(fixture.rangeStarts()==starts&&fixture.requests==(int)starts.size(),"Strong date request sequence for "+mode);
  check(fileHash(job->target())==digest&&yes(job->data,"RangeSupported")== (mode=="resume"),
   "Strong date transfer preserves exact output and reports only verified ranges for "+mode);
 }
}

static void savedRangeChecks(const fs::path& root,bool dateOnly){
 const i64 size=480*1024;
 const std::string validator=dateOnly?"strong date":"strong ETag";
 std::vector<std::string> modes={"matching","changed","weak","ignored","overlap"};
 if(!dateOnly)modes.insert(modes.end(),{"refuse-403","refuse-405","refuse-501","refuse-403-delayed",
  "refuse-403-challenge","refuse-403-refresh","refuse-403-denied"});
 for(const auto& mode:modes){
  TransferFixture fixture(size,0,100);
  const auto directory=root/wide((dateOnly?"saved-date-":"saved-etag-")+mode);
  std::string id,digest;
  i64 prefix=0;
  {
   Manager manager(directory);
   auto prefs=manager.state["Settings"];
   prefs["ProxyMode"]="Connect directly";
   prefs["Connections"]=1;
   manager.setSettings(prefs);
   const auto expected=directory/L"expected.bin";
   fixture.expected(expected);
   digest=fileHash(expected);
   const auto route=(dateOnly?"/date-only-paused-":"/etag-paused-")+mode;
   auto job=manager.add(fixture.url(route.c_str()),utf8(directory.wstring()),"download.bin","Main queue",true,{},digest);
   id=job->id();
   manager.resume(job);
   manager.tick();
   const auto deadline=GetTickCount64()+5000;
   while(GetTickCount64()<deadline){
    {Lock lock(manager.mutex);if(num(job->data,"Received")>=131072)break;}
    Sleep(10);
   }
   manager.pause(job);
   manager.stop();
   prefix=num(job->data,"Received");
   const bool hasValidator=dateOnly?!str(job->data,"HttpStrongLastModified").empty():
    str(job->data,"ETag")=="\"persistent-fixture-v1\"";
   if(prefix<131072||prefix>=size||job->data.contains("RangeSupported")||!hasValidator)
    throw std::runtime_error("Fixture did not save a partial response with unproven ranges and a "+validator+".");
   check(fixture.requests==1&&!fs::exists(job->target()),"Pause keeps one unpublished response before reopen for "+mode);
  }
  Manager reopened(directory);
  auto job=reopened.jobs.front();
  check(job->id()==id&&num(job->data,"Received")==prefix,"Reopening restores the saved offset for "+mode);
  const bool delayed=mode=="refuse-403-delayed",challenge=mode=="refuse-403-challenge";
  const bool refresh=mode=="refuse-403-refresh",denied=mode=="refuse-403-denied";
  std::vector<i64> offsets{0,prefix-64};
  if(delayed||challenge||refresh||denied){
   if(refresh)job->data["RefreshPendingValidation"]=true;
   const auto partial=reopened.root/L"parts"/wide(id)/(wide(id)+L".data");
   const auto savedHash=fileHash(partial);
   bool expectedError=false;
   try{transfer(reopened,job,std::make_shared<Cancel>());}
   catch(const HttpRejected& error){
    expectedError=error.status==403&&(error.retryAfterMs>=0)==delayed&&
     (dynamic_cast<const BrowserVerificationRequired*>(&error)!=nullptr)==challenge;
   }
   if(denied)offsets.push_back(0);
   check(expectedError&&fixture.rangeStarts()==offsets&&fixture.requests==(int)offsets.size(),
    "Saved range preserves refusal type and bounded request count for "+mode);
   check(fileHash(partial)==savedHash&&num(job->data,"Received")==prefix&&!fs::exists(job->target())&&
    yes(job->data,"RefreshPendingValidation")==refresh,"Saved bytes remain untouched for "+mode);
   continue;
  }
  transfer(reopened,job,std::make_shared<Cancel>());
  if(mode!="matching"&&mode!="ignored")offsets.push_back(0);
  check(fixture.rangeStarts()==offsets&&fixture.requests==(int)offsets.size(),"Saved "+validator+" request sequence for "+mode);
  if(!dateOnly)check(fixture.etagResumeValidators==1,"One continuation sends the saved ETag in If-Range for "+mode);
  check(fileHash(job->target())==digest&&yes(job->data,"RangeSupported")== (mode=="matching"),
   "Saved "+validator+" produces exact output and reports only verified ranges for "+mode);
 }
}

static void legacyHttpResumeChecks(const fs::path& root){
 constexpr i64 size=480*1024,retained=262144;
 TransferFixture fixture(size,0,5);
 const auto state=root/L"legacy-http-resume";
 std::string id,expectedHash;
 fs::path part;
 {
  Manager manager(state);
  auto prefs=manager.state["Settings"];prefs["ProxyMode"]="Connect directly";prefs["Connections"]=1;
  manager.setSettings(prefs);
  const auto expected=state/L"expected.bin";
  fixture.expected(expected);expectedHash=fileHash(expected);
  auto job=manager.add(fixture.url("/steady"),utf8(state.wstring()),"legacy.bin","Main queue",true,{},expectedHash);
  id=job->id();
  job->data["Size"]=size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["RangeSupported"]=true;
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",size-1},{"Done",0}}});
  manager.save();
  const auto parts=state/L"parts"/wide(id);fs::create_directories(parts);
  part=parts/L"0000.part";
  Bytes saved(retained);
  for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);
  writeBytes(part,saved);
 }
 const auto savedHash=fileHash(part);
 {
  Handle locked;
  Manager manager(state);
  auto job=manager.jobs.front();
  locked.h=CreateFileW((state/L"state.json.tmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
  if(!locked)throw std::runtime_error("Cannot block the legacy resume checkpoint.");
  rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"A failed checkpoint prevents adopting a legacy continuation");
  check(fileHash(part)==savedHash&&num(job->data,"TransferredBytes")==0&&!fs::exists(job->target()),
   "The source part survives a failed checkpoint and no continuation bytes are committed");
 }
 Manager manager(state);
 auto job=manager.jobs.front();
 check(job->id()==id&&num(job->data,"Received")==0,"Reopening preserves the original catalog when the migration did not commit");
 const auto before=fixture.requests.load();
 const auto rangeBefore=fixture.rangeStarts().size();
 transfer(manager,job,std::make_shared<Cancel>());
 const auto ranges=fixture.rangeStarts();
 check(fixture.requests==before+1&&ranges.size()==rangeBefore+1&&ranges.back()==retained-64,
  "Legacy resume uses the first continuation and checks overlap without a byte-zero probe");
 check(!fs::exists(part)&&fileHash(job->target())==expectedHash,"Legacy resume removes the migrated part after commit and publishes exact bytes");
}

static void transferChecks(const fs::path& root){
 legacyHttpResumeChecks(root);
 deferredDateRangeChecks(root);
 savedRangeChecks(root,true);
 savedRangeChecks(root,false);
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
  TransferFixture smallResponse(4*1024*1024,0,0);
  Manager manager(testRoot/L"small-direct-state");configure(manager);
  const auto expectedFile=manager.root/L"expected.bin";
  smallResponse.expected(expectedFile);
  const auto digest=fileHash(expectedFile);
  auto job=manager.add(smallResponse.url("/steady"),"","small-direct.bin","Main queue",true,{},digest);
  transfer(manager,job,std::make_shared<Cancel>());
  check(smallResponse.requests==1&&str(job->data.at("FinishPhases"),"HashMode")=="inline",
   "A fresh 4 MiB transfer hashes its single response from written buffers");
  check(str(job->data,"Sha256")==digest&&fileHash(job->target())==digest&&job->data.contains("ContentCheck"),
   "Inline verification preserves the exact bytes and still classifies the downloaded content");
 }
 {
  TransferFixture delayed(4*1024*1024,0,0);
  auto delayedExpected=testRoot/L"adjacent-expected.bin";delayed.expected(delayedExpected);
  const auto digest=fileHash(delayedExpected);
  const auto middle=delayed.size/2;
  Manager manager(testRoot/L"adjacent-pending-state");configure(manager);
  auto job=manager.add(delayed.url("/adjacent-pending"),"","adjacent-pending.bin","Main queue",true,{},digest);
  job->data["Connections"]=2;
  job->data["Size"]=delayed.size;
  job->data["ETag"]="\"persistent-fixture-v1\"";
  job->data["RangeSupported"]=true;
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",middle-1},{"Done",0}},
   {{"Index",1},{"Start",middle},{"End",delayed.size-1},{"Done",0}}});
  transfer(manager,job,std::make_shared<Cancel>());
  const auto requests=delayed.rangeStarts();
  check(delayed.helperHeadersWaiting&&std::count(requests.begin(),requests.end(),middle-64)==1,"An adjacent range already waiting for headers is requested once");
  check(fileHash(job->target())==digest,"Keeping the assigned response preserves exact bytes");
  const auto& checks=job->data.at("HttpBoundaryChecks");
  check(num(checks,"FromWriteBuffer")>0,
   "Completed neighboring ranges can verify their HTTP overlaps without disk reads");
 }
 {
  TransferFixture saved(4*1024*1024,0,0);
  const auto middle=saved.size/2;
  Manager manager(testRoot/L"adjacent-saved-state");configure(manager);
  auto job=manager.add(saved.url("/steady"),"","adjacent-saved.bin","Main queue",true);
  job->data["Connections"]=1;
  job->data["Size"]=saved.size;
  job->data["Received"]=middle;
  job->data["ETag"]="\"persistent-fixture-v1\"";
  job->data["RangeSupported"]=true;
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",middle-1},{"Done",middle}},
   {{"Index",1},{"Start",middle},{"End",saved.size-1},{"Done",0}}});
  const auto parts=manager.root/L"parts"/wide(job->id());fs::create_directories(parts);
  Bytes prefix((size_t)middle);
  for(i64 i=0;i<middle;++i)prefix[(size_t)i]=TransferFixture::value(i);
  writeBytes(parts/(wide(job->id())+L".data"),prefix);
  transfer(manager,job,std::make_shared<Cancel>());
  const auto& checks=job->data.at("HttpBoundaryChecks");
  check(num(checks,"FromWriteBuffer")==0&&num(checks,"FromDisk")==1,
   "A previously saved neighboring range still verifies its overlap from disk");
  auto expectedSaved=testRoot/L"adjacent-saved-expected.bin";saved.expected(expectedSaved);
  check(fileHash(job->target())==fileHash(expectedSaved),"Disk fallback preserves the exact resumed output");
 }
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
  const auto& ticks=job->data.at("NetworkTicks");
  const auto last=num(ticks,"LastByte"),workers=num(ticks,"WorkersJoined"),boundaries=num(ticks,"Joined");
  const auto hashStopped=num(ticks,"PrefixHashStopped"),networkEnd=num(ticks,"NetworkEnd");
  check(last>0&&last<=workers&&workers<=boundaries&&boundaries<=hashStopped&&hashStopped<=networkEnd,
   "HTTP finish timing separates the last byte, range workers, boundary checks and prefix hash shutdown");
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
  for(size_t i=0;i<requests.size();++i){if(requests[i]>0&&requests[i]<2*1024*1024-64&&helper==requests.size())helper=i;if(requests[i]==30*1024*1024-64&&lastQueued==requests.size())lastQueued=i;}
  check(helper<lastQueued,"Server observes help for the slow range before the last queued chunk starts");
  check(fileHash(job->target())==hash,"Early redistribution produces the exact original file");
 }
 for(const bool advertised:{false,true})for(const bool lateHelper:{false,true}){
  TransferFixture freshFixture(32*1024*1024,240,8);
  std::atomic_int startupCheckpoints=0,receivingCheckpoints=0;
  auto folder=testRoot/(advertised?L"advertised":L"unadvertised");
  Manager manager(folder/(lateHelper?L"late-helper-state":L"fresh-plan-state"));configure(manager);
  auto job=manager.add(freshFixture.url(advertised?"/delayed-helper":"/delayed-unadvertised-helper"),"","fresh-plan.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  CapturedResponseTestAccess::saveCheckpoint(manager,[&](const char* point){
   if(std::string_view(point)!="temporary-flushed")return;
   Lock lock(manager.mutex);
   const auto received=num(job->data,"Received");
   if(received==0)++startupCheckpoints;
   else if(freshFixture.allowHelperHeaders&&received<freshFixture.size)++receivingCheckpoints;
  });
  auto running=std::async(std::launch::async,[&]{transfer(manager,job,std::make_shared<Cancel>());});
  i64 receivedBeforeHelper=0;
  bool rangesBeforeHelper=false;
  const auto deadline=GetTickCount64()+7000;
  while(GetTickCount64()<deadline){
   {Lock lock(manager.mutex);if(freshFixture.helperHeadersWaiting){receivedBeforeHelper=num(job->data,"Received");rangesBeforeHelper=yes(job->data,"RangeSupported");}}
   if(receivedBeforeHelper>(lateHelper?freshFixture.size/2:0)||running.wait_for(std::chrono::seconds(0))==std::future_status::ready)break;
   Sleep(10);
  }
  freshFixture.allowHelperHeaders=true;
  running.get();
  check(startupCheckpoints==0,"A fresh HTTP download writes its first bytes without flushing the catalog again");
  check(receivingCheckpoints==0,"Admitting the midpoint helper does not flush the catalog during an active response");
  auto starts=freshFixture.rangeStarts();
  check(rangesBeforeHelper==advertised&&yes(job->data,"RangeSupported"),"An unadvertised range becomes resumable only after the helper validates");
  check(starts.size()>=2&&starts[0]==0&&starts[1]==freshFixture.size/2-64,"Fresh download opens the full response and a midpoint helper with overlap first");
  if(lateHelper){
   check(receivedBeforeHelper>freshFixture.size/2&&receivedBeforeHelper<freshFixture.size,"The primary crosses the midpoint while helper response headers are pending");
   check(starts.size()>2&&starts[2]>receivedBeforeHelper,"A late validated helper allows a new range beyond the already written prefix");
  }else check(receivedBeforeHelper>0,"The full response writes bytes while midpoint response headers are pending");
  check(num(job->data,"DynamicSplits")>0&&job->workers.size()==4,"Additional workers obtain ranges by splitting within the connection limit");
  check(fileHash(job->target())==hash,"Dynamic startup preserves exact completed bytes");
 }
 {
  TransferFixture oddWorkers(12*1024*1024,0,4);
  auto expectedOdd=testRoot/L"odd-worker-expected.bin";oddWorkers.expected(expectedOdd);
  const auto digest=fileHash(expectedOdd);
  Manager manager(testRoot/L"odd-worker-start");configure(manager);
  auto job=manager.add(oddWorkers.url("/steady"),"","odd-worker-start.bin","Main queue",true,{},digest);
  job->data["Connections"]=8;
  transfer(manager,job,std::make_shared<Cancel>());
  const auto starts=oddWorkers.rangeStarts();
  check(job->workers.size()==3&&starts.size()>=3&&starts[0]==0&&starts[1]==oddWorkers.size-oddWorkers.size/3-64,
   "The first HTTP helper leaves two shares on the full response for a three-worker start");
  check(fileHash(job->target())==digest,"An odd HTTP worker set publishes the exact file");
 }
 {
  TransferFixture boundaryFixture(32*1024*1024,0,2);
  Manager manager(testRoot/L"http-adjacent-overlap");configure(manager);
  auto job=manager.add(boundaryFixture.url("/boundary-mismatch"),"","http-adjacent-overlap.bin","Main queue",true,{},hash);
  job->data["Connections"]=2;
  transfer(manager,job,std::make_shared<Cancel>());
  auto starts=boundaryFixture.rangeStarts();
  check(starts.size()>=3&&starts[1]==boundaryFixture.size/2-64&&starts.back()==0&&std::count(starts.begin(),starts.end(),0)==2&&job->workers.size()==1,"A fresh boundary mismatch retries from zero on a single connection");
  check(fileHash(job->target())==hash,"The single-connection retry publishes the exact file");
 }
 {
  Manager manager(testRoot/L"sparse-recovery");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","sparse-recovery.bin","Main queue",true,{},hash);
  constexpr i64 prefix=4*1024*1024,saved=2*prefix;
  auto parts=manager.root/L"parts"/wide(job->id());fs::create_directories(parts);
  const auto path=parts/(wide(job->id())+L".data");
  Bytes first((size_t)prefix);
  {std::ifstream source(expected,std::ios::binary);source.read(reinterpret_cast<char*>(first.data()),first.size());if(!source)throw std::runtime_error("Cannot read the sparse recovery fixture prefix.");}
  writeBytes(path,first);
  bool hole=false;
  {
   Handle file(CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr));
   if(!file)throw std::runtime_error("Cannot open the sparse recovery fixture.");
   DWORD bytes=0;
   if(DeviceIoControl(file.h,FSCTL_SET_SPARSE,nullptr,0,nullptr,0,&bytes,nullptr)){
    LARGE_INTEGER end{};end.QuadPart=fixture.size;
    if(!SetFilePointerEx(file.h,end,nullptr,FILE_BEGIN)||!SetEndOfFile(file.h))throw std::runtime_error("Cannot size the sparse recovery fixture.");
    FILE_ALLOCATED_RANGE_BUFFER query{},allocated{};query.Length.QuadPart=saved;
    hole=DeviceIoControl(file.h,FSCTL_QUERY_ALLOCATED_RANGES,&query,sizeof(query),&allocated,sizeof(allocated),&bytes,nullptr)&&
     bytes==sizeof(allocated)&&allocated.FileOffset.QuadPart==0&&allocated.Length.QuadPart==prefix;
   }
  }
  if(hole){
   job->data["Size"]=fixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["Connections"]=1;
   job->data["Received"]=saved;job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",fixture.size-1},{"Done",saved}}});
   const auto before=fixture.rangeStarts().size();
   const auto requestsBefore=fixture.requests.load();
   transfer(manager,job,std::make_shared<Cancel>());
   const auto starts=fixture.rangeStarts();
   check(starts.size()==before+1&&starts[before]==prefix-64&&fixture.requests==requestsBefore+1,
    "Resume makes one request with overlap just before the first hole in a preallocated file");
   check(fileHash(job->target())==hash,"Resuming across a sparse hole restores exact file bytes");
  }else std::cout<<"  Sparse hole recovery fixture unavailable on this volume\n";
 }
 i64 retained=0;std::string pausedId;Json beforeSplitCheckpoint;
 {
  Manager manager(testRoot/L"resume-state");configure(manager);
  auto job=manager.add(fixture.url("/straggler"),"","split-resume.bin","Main queue",true,{},hash);job->data["Connections"]=4;pausedId=job->id();
  auto stop=std::make_shared<Cancel>();
  auto running=std::async(std::launch::async,[&]{try{transfer(manager,job,stop);}catch(const Cancelled&){};});
  bool split=false,received=false;
  for(int i=0;i<1500;++i){
   Sleep(10);
   {
    Lock lock(manager.mutex);split=num(job->data,"DynamicSplits")>0;received=num(job->data,"Received")>0;
    if(split&&received)beforeSplitCheckpoint=Json::parse(readText(manager.root/L"state.json"));
   }
   // A range can split after its headers arrive and before any payload is written.
   if((split&&received)||running.wait_for(std::chrono::seconds(0))==std::future_status::ready)break;
  }
  stop->stop=true;running.get();retained=num(job->data,"Received");manager.save();
  check(split&&retained>0&&retained<fixture.size&&!fs::exists(job->target()),"Pause after a live split retains unfinished parts");
 }
 if(!beforeSplitCheckpoint.is_null()){
  auto& saved=beforeSplitCheckpoint["Downloads"].at(0);
  check(num(saved,"Received")==0&&saved.at("Segments").empty()&&
    str(saved,"Url")==fixture.url("/straggler"),
   "The initial checkpoint preserves the download without claiming uncheckpointed bytes");
  saved["FileName"]="split-before-checkpoint.bin";
  const auto replayRoot=testRoot/L"split-before-checkpoint-state";
  const auto relative=fs::path(L"parts")/wide(pausedId)/(wide(pausedId)+L".data");
  fs::create_directories((replayRoot/relative).parent_path());
  fs::copy_file(testRoot/L"resume-state"/relative,replayRoot/relative);
  atomicText(replayRoot/L"state.json",beforeSplitCheckpoint.dump(),false);
  Manager recovered(replayRoot);
  auto job=recovered.jobs.at(0);
  transfer(recovered,job,std::make_shared<Cancel>());
  check(num(job->data,"Received")==fixture.size&&fileHash(job->target())==hash,
   "Reopening the checkpoint from before a live split recovers the exact file");
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
   for(const auto& s:savedSegments)if(num(s,"Done")<num(s,"End")-num(s,"Start")+1){
    const auto start=num(s,"Start"),done=num(s,"Done");
    fromSaved&=resumedAt.count(start+done-std::min<i64>(64,done>0?done:start))>0;
   }
   check(fromSaved,"Resume verifies overlap before each unfinished range's saved offset");
   const auto served=killFixture.bodyBytes.load()-servedBefore,missing=killFixture.size-savedReceived;std::cout<<"  resume served "<<served<<" of "<<missing<<" missing bytes, splits "<<num(job->data,"DynamicSplits")<<"\n";
   // A dynamic split discards the bytes in flight past the new boundary, so the origin may serve slightly more than the gap; never less, and never the saved bytes again.
   check(served>=missing&&served<missing+4*1048576,"Resume fetches the bytes the saved catalog was missing and no saved bytes again");
   check(fileHash(job->target())==hash&&num(job->data,"Received")==killFixture.size&&str(job->data,"Status")=="Complete","Kill-and-resume on the single output file completes with the exact SHA-256");
  }
 }
 {
  Manager manager(testRoot/L"http-overlap-mismatch");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","http-overlap-mismatch.bin","Main queue",true,{},hash);
  constexpr i64 retained=262144;
  Bytes saved((size_t)retained);
  for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);
  saved.back()^=1;
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);
  auto part=folder/(wide(job->id())+L".data");writeBytes(part,saved);
  {std::fstream dataFile(part,std::ios::in|std::ios::out|std::ios::binary);dataFile.seekp(fixture.size-1);dataFile.put('\0');if(!dataFile)throw std::runtime_error("Cannot size the overlap fixture.");}
  const auto oldHash=fileHash(part);
  job->data["Size"]=fixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["Received"]=retained;
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",fixture.size-1},{"Done",retained}}});
  job->data["RefreshPendingValidation"]=true;
  rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"An overlap mismatch after address refresh stops the transfer");
  check(fileHash(part)==oldHash&&!fs::exists(job->target())&&yes(job->data,"RefreshPendingValidation"),"A strict refresh preserves the old file and validation flag on mismatch");
  job->data["RefreshPendingValidation"]=false;
  transfer(manager,job,std::make_shared<Cancel>());
  check(job->workers.size()==1&&fileHash(job->target())==hash,"An ordinary overlap mismatch restarts with one connection and publishes exact bytes");
 }
 {
  Manager manager(testRoot/L"http-overlap-part-repair");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","http-overlap-part-repair.bin","Main queue",true,{},hash);
  const i64 middle=fixture.size/2,retained=262144;
  Bytes saved((size_t)(middle+retained));
  for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);
  saved.back()^=1;
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);
  writeBytes(folder/(wide(job->id())+L".data"),saved);
  job->data["Connections"]=1;job->data["Size"]=fixture.size;job->data["ETag"]="\"persistent-fixture-v1\"";job->data["Received"]=middle+retained;
  job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",middle-1},{"Done",middle}},{{"Index",1},{"Start",middle},{"End",fixture.size-1},{"Done",retained}}});
  const auto before=fixture.rangeStarts().size();
  transfer(manager,job,std::make_shared<Cancel>());
  const auto starts=fixture.rangeStarts();
  check(starts.size()==before+2&&starts[before]==middle+retained-64&&starts[before+1]==middle-64,"A mismatched resumed range is fetched again without requesting the completed preceding range");
  check(fileHash(job->target())==hash,"Repairing one mismatched range publishes the exact file");
 }
 {
  // Reproduce a 0.8.0 fixed-chunk layout with one partial file already present.
  Manager manager(testRoot/L"legacy-state");configure(manager);
  auto job=manager.add(fixture.url("/steady"),"","legacy-resume.bin","Main queue",true,{},hash);job->data["Connections"]=4;
  auto& data=job->data;data["Size"]=fixture.size;data["ETag"]="\"persistent-fixture-v1\"";data["RangeSupported"]=true;data["Segments"]=Json::array();
  constexpr i64 chunk=2*1024*1024;int id=0;
  for(i64 begin=0;begin<fixture.size;begin+=chunk)data["Segments"].push_back({{"Index",id++},{"Start",begin},{"End",std::min(begin+chunk-1,fixture.size-1)},{"Done",0}});
  auto folder=manager.root/L"parts"/wide(job->id());fs::create_directories(folder);Bytes saved(256*1024);for(size_t i=0;i<saved.size();++i)saved[i]=(unsigned char)TransferFixture::value((i64)i);
  const auto part=folder/L"0000.part";writeBytes(part,saved);const auto partHash=fileHash(part);fixture.resumedPrefix=false;
  {
   Handle locked(CreateFileW((manager.root/L"state.json.tmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));
   if(!locked)throw std::runtime_error("Cannot block the migration checkpoint.");
   rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"A failed catalog commit stops legacy migration before the workers start");
   check(fileHash(part)==partHash&&num(job->data,"TransferredBytes")==0&&!fs::exists(job->target()),"A failed catalog commit preserves the original legacy part bytes");
  }
  fixture.resumedPrefix=false;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&fixture.resumedPrefix,"Legacy fixed-chunk resume retains existing bytes");
  bool legacyLeft=false;for(const auto& entry:fs::directory_iterator(folder))legacyLeft|=entry.path().extension()==L".part";
  check(!legacyLeft,"Legacy part files are folded into the single output file and removed");
 }
 {
  const auto catalog=testRoot/L"interrupted-range-reset";
  fs::path part;
  {
   Manager manager(catalog);configure(manager);
   auto job=manager.add(fixture.url("/steady"),"","range-reset.bin","Main queue",true,{},hash);
   const auto parts=manager.root/L"parts"/wide(job->id());fs::create_directories(parts);
   part=parts/L"0000.part";writeBytes(part,Bytes(262144,'x'));const auto oldHash=fileHash(part);
   job->data["Size"]=fixture.size;job->data["ETag"]="\"old-resource\"";job->data["Received"]=262144;
   job->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",fixture.size-1},{"Done",262144}}});
   manager.save();
   {
    Handle locked(CreateFileW((catalog/L"state.json.tmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr));
    if(!locked)throw std::runtime_error("Cannot block the range reset checkpoint.");
    rejects([&]{transfer(manager,job,std::make_shared<Cancel>());},"A failed range reset checkpoint stops the transfer");
    check(fileHash(part)==oldHash&&num(job->data,"Received")==262144,"A failed range reset preserves old bytes and offsets");
   }
   // Simulate closing just after the reset commits, before old files are deleted.
   job->data["Received"]=0;job->data["Segments"][0]["Done"]=0;
   job->data["ETag"]="\"persistent-fixture-v1\"";job->data["HttpPartsResetPending"]=true;
   manager.save();
  }
  {
   Manager manager(catalog);
   auto job=manager.jobs.front();check(yes(job->data,"HttpPartsResetPending")&&fs::exists(part),"Reopen sees the pending reset alongside the obsolete part");
   transfer(manager,job,std::make_shared<Cancel>());
   check(!yes(job->data,"HttpPartsResetPending")&&!fs::exists(part)&&fileHash(job->target())==hash,"Pending reset discards obsolete parts before rebuilding the exact file");
  }
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
  // Count accepted sockets before their headers arrive so setup latency
  // cannot accidentally give extra workers a slot in the capped origin.
  TransferFixture limitFixture(32*1024*1024,240,1);limitFixture.maxConnections=2;
  Manager manager(testRoot/L"limit-state");configure(manager);
  auto job=manager.add(limitFixture.url("/steady"),"","limited.bin","Main queue",true,{},hash);job->data["Connections"]=8;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&str(job->data,"Status")=="Complete","A two-connection server limit completes the exact file with eight workers requested");
  const auto refused=limitFixture.rejected.load();
  check(refused<=2&&num(job->data,"Received")==limitFixture.size,"A refused extra connection does not trigger a wave of opens ("+std::to_string(refused)+" refused)");
  check(refused>0&&limitFixture.requests==2,"Refused extra split leaves both original responses covering the file ("+std::to_string(limitFixture.requests.load())+" HTTP requests)");
 }
 {
  TransferFixture forbiddenFixture(32*1024*1024,240,4);
  Manager manager(testRoot/L"forbidden-range-state");configure(manager);
  auto job=manager.add(forbiddenFixture.url("/range-forbidden"),"","forbidden-range.bin","Main queue",true,{},hash);
  job->data["Connections"]=8;
  transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&num(job->data,"Received")==forbiddenFixture.size,"An HTTP 403 on an extra range preserves the exact file");
  const auto requests=forbiddenFixture.requests.load();
  check(forbiddenFixture.forbiddenRanges==1&&requests==3,"An HTTP 403 returns the untouched split to the original response ("+std::to_string(requests)+" requests including the rejection)");
 }
 {
  TransferFixture recoveryFixture(32*1024*1024,240,4);
  recoveryFixture.dropEvery=6*1024*1024;
  Manager manager(testRoot/L"forbidden-range-recovery-state");configure(manager);
  auto job=manager.add(recoveryFixture.url("/range-forbidden"),"","forbidden-range-recovery.bin","Main queue",true,{},hash);
  job->data["Connections"]=8;
  transfer(manager,job,std::make_shared<Cancel>());
  check(recoveryFixture.forbiddenRanges>0&&recoveryFixture.drops>0&&fileHash(job->target())==hash,
   "An HTTP 403 on expansion still permits reconnecting ranges after a body reset");
 }
}
