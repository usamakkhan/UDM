#pragma once
static void reliabilityChecks(const udm::fs::path& root,Fixture& fixture){
 using namespace udm;
 check(retryAfterDelay(" 2 ",0)==2000&&retryAfterDelay("0",0)==0,"Retry-After accepts nonnegative seconds and whitespace");
 check(retryAfterDelay("Wed, 21 Oct 2015 07:28:02 GMT",1445412480000LL)==2000,"Retry-After HTTP-date uses UTC");
 check(retryAfterDelay("Sunday, 06-Nov-94 08:49:39 GMT",784111777000LL)==2000&&retryAfterDelay("Sun Nov  6 08:49:39 1994",784111777000LL)==2000,"Legacy HTTP-date forms preserve server wait times");
 check(retryAfterDelay("Wed, 21 Oct 2015 07:28:00 GMT",1445412482000LL)==0,"Past Retry-After dates do not delay");
 check(retryAfterDelay("garbage",0)==-1&&retryAfterDelay("-1",0)==-1&&retryAfterDelay("1.5",0)==-1,"Malformed Retry-After falls back to normal backoff");
 check(retryAfterDelay("999999999999999999999999",0)==-2&&!HttpRejected(429,"301").retryable(),"Excessive server waits stop automatic retries without integer overflow");
 check(!HttpRejected(401).retryable()&&!HttpRejected(403).retryable()&&!HttpRejected(404).retryable()&&!HttpRejected(410).retryable()&&HttpRejected(429).retryable()&&HttpRejected(503).retryable(),"Permanent HTTP failures and temporary throttles have distinct policies");
 Manager manager(root/L"reliability");manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"reliability-files").wstring());manager.state["Settings"]["Connections"]=1;manager.state["Settings"]["Retries"]=2;manager.state["Queues"][0]["Retries"]=2;
 auto expected=root/L"retry-expected.bin";writeBytes(expected,Bytes(fixture.payload.begin(),fixture.payload.end()));auto hash=fileHash(expected);
 for(auto route:{"/busy-probe","/busy-worker"}){
  auto job=manager.add(fixture.url(route),"",std::string(route+1)+".bin");auto begin=GetTickCount64();transfer(manager,job,std::make_shared<Cancel>());
  check(fileHash(job->target())==hash&&GetTickCount64()-begin>=1000,route==std::string("/busy-probe")?"Transient probe retries honor server delay and download exact bytes":"Transient range retries honor server delay and download exact bytes");
 }
 auto denied=manager.add(fixture.url("/forbidden-worker"),"","denied.bin");bool rejected=false;try{transfer(manager,denied,std::make_shared<Cancel>());}catch(const HttpRejected& e){rejected=e.status==403;}
 check(rejected&&fixture.forbiddenWorker==1&&!fs::exists(denied->target()),"Forbidden range stops immediately and never publishes a file");
 auto busy=manager.add(fixture.url("/always-busy"),"","busy.bin");rejects([&]{transfer(manager,busy,std::make_shared<Cancel>());},"Busy probe eventually reports failure");check(fixture.alwaysBusy==3,"Probe retries obey the configured attempt budget");
 auto longWait=manager.add(fixture.url("/long-wait"),"","long.bin");rejects([&]{transfer(manager,longWait,std::make_shared<Cancel>());},"Excessive Retry-After returns a recoverable error");check(fixture.longWait==1,"A long server wait never causes an early automatic retry");
 for(auto route:{"/cancel-retry","/cancel-worker"}){
  auto job=manager.add(fixture.url(route),"",std::string(route+1)+".bin");auto cancel=std::make_shared<Cancel>();
  auto task=std::async(std::launch::async,[&]{try{transfer(manager,job,cancel);return false;}catch(const Cancelled&){return true;}});
  auto& seen=route==std::string("/cancel-retry")?fixture.cancelRetry:fixture.cancelWorker;for(int i=0;i<200&&seen==0;++i)Sleep(5);
  auto begin=GetTickCount64();cancel->stop=true;bool paused=task.get();check(paused&&seen==1&&GetTickCount64()-begin<1500,route==std::string("/cancel-retry")?"Pause interrupts a long probe cooldown promptly":"Pause interrupts a long range cooldown promptly");
 }
 auto options=parseLaunch({L"/d",L"https://example.test/file",L"/p",(root/L"explicit folder").wstring(),L"/f",L"chosen.bin",L"/n",L"--paused"});
 check(options.silent&&options.paused&&options.folder==utf8((root/L"explicit folder").wstring())&&options.name=="chosen.bin","Command-line aliases preserve folder, name and independent pause/silent flags");
 rejects([]{parseLaunch({L"--folder"});},"Missing command-line values report an error");
 auto paused=manager.receive(options.request());check(str(paused->data,"Folder")==options.folder&&str(paused->data,"FileName")=="chosen.bin"&&str(paused->data,"Status")=="Paused","Paused command-line request preserves its exact destination");
 options.address="https://example.test/silent";options.name="silent.bin";options.paused=false;
 auto silent=manager.receive(options.request());check(str(silent->data,"Status")=="Queued","Silent command-line request queues without File Info confirmation");
 auto before=silent->data;check(manager.receive(options.request())==silent&&silent->data==before,"Repeated command-line handoff never resets queued progress");
 options.address="https://example.test/confirm";options.silent=false;manager.state["Settings"]["SkipBrowserFileInfo"]=true;
 check(str(manager.receive(options.request())->data,"Status")=="Awaiting confirmation","Ordinary command-line launch retains File Info regardless of browser setting");
}
