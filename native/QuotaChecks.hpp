#pragma once
static void quotaChecks(const fs::path& root,Fixture& fixture){
 Manager m(root/L"quota-state");auto p=m.state["Settings"];p["DownloadFolder"]=utf8((root/L"quota-files").wstring());p["CategoryFolders"]=false;p["QuotaHours"]=1;p["QuotaMb"]=0;m.setSettings(p);
 auto a=m.add(fixture.url("/quota-a")),b=m.add(fixture.url("/quota-b"));Rate rate;Cancel c;const i64 mb=1024*1024;
 auto wait=[&](auto condition){for(int i=0;i<250;++i){if(condition())return true;Sleep(10);}return false;};
 auto cancel=[]{auto value=std::make_shared<Cancel>();value->deadline=GetTickCount64()+10000;return value;};
 auto charge=[&](JobPtr job,size_t count,std::shared_ptr<Cancel> token){return std::async(std::launch::async,[&,job,count,token]{Rate local;try{m.charge(count,*token,local,job);return true;}catch(const Cancelled&){return false;}});};
 auto reset=[&](i64 used=0){Lock l(m.mutex);m.state["QuotaStart"]=date();m.state["QuotaBytes"]=used;};
 m.charge(2*mb,c,rate,a);check(num(m.state,"QuotaBytes")==2*mb&&!yes(m.quotaStatus(),"Waiting")&&m.takeQuotaWarning().empty(),"Disabled quota accounts bytes without blocking or warning");
 p["QuotaMb"]=1;m.setSettings(p);reset(mb-100);auto ca=cancel(),cb=cancel();auto fa=charge(a,1000,ca),fb=charge(b,1000,cb);
 check(wait([&]{return yes(m.quotaStatus(a),"Waiting")&&yes(m.quotaStatus(b),"Waiting");}),"Concurrent downloads both report quota waiting");
 check(num(m.quotaStatus(),"Bytes")==mb,"Concurrent reservations fill the remaining quota without overshooting");
 auto warning=m.takeQuotaWarning();check(yes(warning,"Waiting")&&num(warning,"LimitBytes")==mb&&num(warning,"Seconds")>3500,"Quota warning reports the actual shared limit and resumption time");
 check(m.takeQuotaWarning().empty()&&m.takeQuotaWarning().empty(),"One quota period emits one warning across multiple workers");
 check(quotaWaitText(warning).find("Resuming automatically")!=std::string::npos,"Quota progress explains automatic resumption");
 auto began=GetTickCount64();ca->stop=true;check(!fa.get()&&GetTickCount64()-began<1000,"Cancel interrupts a quota wait promptly");
 check(!yes(m.quotaStatus(a),"Waiting")&&yes(m.quotaStatus(b),"Waiting"),"Cancel clears only the waiting job and leaves other workers blocked");
 p["QuotaMb"]=2;m.setSettings(p);check(fb.get()&&!yes(m.quotaStatus(),"Waiting"),"Increasing a quota releases waiting downloads without a restart");
 check(num(m.state,"QuotaBytes")==mb+900||num(m.state,"QuotaBytes")==mb+1000,"Quota release charges outstanding bytes exactly once regardless of which worker reserved first");
 p["QuotaMb"]=1;p["WarnQuota"]=false;m.setSettings(p);reset(mb);ca=cancel();fa=charge(a,7,ca);check(wait([&]{return yes(m.quotaStatus(),"Waiting");})&&m.takeQuotaWarning().empty(),"Disabled warnings still enforce the limit silently");
 p["WarnQuota"]=true;m.setSettings(p);check(!m.takeQuotaWarning().empty(),"Enabling warnings during an unannounced wait presents the current limit");
 p["QuotaMb"]=0;m.setSettings(p);check(fa.get()&&m.takeQuotaWarning().empty()&&!yes(m.quotaStatus(a),"Waiting"),"Disabling the quota releases a wait and removes its warning state");
 p["QuotaMb"]=1;m.setSettings(p);reset(mb);ca=cancel();fa=charge(a,23,ca);check(wait([&]{return yes(m.quotaStatus(a),"Waiting");}),"Exhausted quota blocks the next reservation");m.takeQuotaWarning();
 {Lock l(m.mutex);m.state["QuotaStart"]=date(epoch()-3600001);}
 check(fa.get()&&num(m.state,"QuotaBytes")==23&&!yes(m.quotaStatus(),"Waiting"),"Elapsed quota period automatically releases reserved data and resets accounting");
 {Lock l(m.mutex);m.state["QuotaBytes"]=mb;}
 ca=cancel();fa=charge(a,1,ca);check(wait([&]{return yes(m.quotaStatus(),"Waiting");})&&!m.takeQuotaWarning().empty(),"A new quota period can warn again");ca->stop=true;fa.get();
 reset();ca=cancel();fa=charge(a,mb+101,ca);check(wait([&]{return yes(m.quotaStatus(),"Waiting");})&&num(m.state,"QuotaBytes")==mb,"A chunk larger than the quota fills this period instead of deadlocking");
 {Lock l(m.mutex);m.state["QuotaStart"]=date(epoch()-3600001);}
 check(fa.get()&&num(m.state,"QuotaBytes")==101,"Large reservation completes its remainder in the following period");
 {Lock l(m.mutex);m.state["QuotaStart"]="invalid";m.state["QuotaBytes"]=mb;}
 m.charge(11,c,rate,a);check(num(m.state,"QuotaBytes")==11,"Malformed quota timestamps recover without an indefinite wait");
 {Lock l(m.mutex);m.state["QuotaStart"]=date(epoch()+3600000);m.state["QuotaBytes"]=mb;}
 m.charge(13,c,rate,a);check(num(m.state,"QuotaBytes")==13,"Clock rollback cannot strand a future quota period");
 auto invalid=p;invalid["WarnQuota"]="yes";rejects([&]{m.setSettings(invalid);},"Quota warning setting rejects invalid types");
 reset(mb);m.save();{Manager restored(m.root);auto job=restored.jobs[0];auto token=cancel();auto future=std::async(std::launch::async,[&]{Rate local;try{restored.charge(1,*token,local,job);return true;}catch(const Cancelled&){return false;}});check(wait([&]{return yes(restored.quotaStatus(job),"Waiting");})&&!restored.takeQuotaWarning().empty(),"Restart retains consumed quota and reports a new waiting session");token->stop=true;check(!future.get(),"Restored quota wait remains cancellable");}
 // Exercise the actual segmented HTTP path, not only reservation helpers.
 Manager live(root/L"quota-live");auto settings=live.state["Settings"];settings["DownloadFolder"]=utf8((root/L"quota-live-files").wstring());settings["CategoryFolders"]=false;settings["QuotaMb"]=1;settings["Connections"]=4;settings["Retries"]=0;live.setSettings(settings);auto job=live.add(fixture.url("/range"));live.resume(job);live.tick();
 check(wait([&]{return yes(live.quotaStatus(job),"Waiting");}),"Real segmented HTTP transfer enters quota wait");
 i64 received;{Lock l(live.mutex);received=num(job->data,"Received");}check(received<=mb&&!fs::exists(job->target()),"Quota-blocked transfer neither publishes a partial file nor exceeds its write budget");
 live.pause(job);check(wait([&]{return !live.isActive(job);})&&!yes(live.quotaStatus(job),"Waiting"),"Pause stops all quota-blocked HTTP workers");
 check(num(job->data,"Received")>0&&str(job->data,"Status")=="Paused","Quota pause retains downloaded segments for resumption");
 settings["QuotaMb"]=0;live.setSettings(settings);live.resume(job);live.tick();bool complete=wait([&]{return !live.isActive(job);});if(!complete)live.pause(job);live.stop();
 auto expected=root/L"quota-expected.bin";writeBytes(expected,Bytes(fixture.payload.begin(),fixture.payload.end()));check(complete&&str(job->data,"Status")=="Complete"&&fileHash(job->target())==fileHash(expected),"Resuming a quota-paused parallel transfer yields the exact original bytes");
 Manager automatic(root/L"quota-automatic");settings["QuotaMb"]=1;automatic.setSettings(settings);auto autoJob=automatic.add(fixture.url("/range"));automatic.resume(autoJob);automatic.tick();check(wait([&]{return yes(automatic.quotaStatus(autoJob),"Waiting");}),"Automatic rollover test reaches its first quota limit");
 int periods=0;for(int i=0;i<600&&automatic.isActive(autoJob);++i){if(yes(automatic.quotaStatus(autoJob),"Waiting")){Lock l(automatic.mutex);automatic.state["QuotaStart"]=date(epoch()-3600001);++periods;}Sleep(10);}
 bool finished=!automatic.isActive(autoJob);if(!finished)automatic.pause(autoJob);automatic.stop();check(finished&&periods>=3&&str(autoJob->data,"Status")=="Complete"&&fileHash(autoJob->target())==fileHash(expected),"Real parallel download resumes across multiple quota periods with exact bytes");
 m.stop();
}
