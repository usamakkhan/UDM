#pragma once
#include "QueueWake.hpp"
#include "SchedulerTime.hpp"
#include <cstdlib>
#include <ctime>
struct TestTimeZone {
 std::string previous;bool existed=false;
 explicit TestTimeZone(const char* value){char* old=nullptr;size_t size=0;_dupenv_s(&old,&size,"TZ");if(old){existed=true;previous=old;free(old);}_putenv_s("TZ",value);_tzset();}
 ~TestTimeZone(){_putenv_s("TZ",existed?previous.c_str():"");_tzset();}
};
static void wakeChecks(const fs::path& root){
 auto utc=[](const char* s){return parseDate(Json(s));};
 {TestTimeZone zone("UTC0");auto q=defaultQueue();const auto monday=utc("2026-09-28T08:00:00.000Z");
  check(!yes(q,"WakeComputer"),"Existing/default queues do not opt into waking the computer");
  q["Scheduled"]=true;q["StartMinute"]=9*60;q["StopMinute"]=10*60;
  check(nextQueueWake(q,monday)==0,"Wake planner ignores a queue without explicit opt-in");q["WakeComputer"]=true;
  check(nextQueueWake(q,monday)==monday+3600000,"Daily wake matches the next scheduler window");
  q["Enabled"]=false;check(nextQueueWake(q,monday)==0,"Disabled queue cannot wake the computer");q["Enabled"]=true;
  q["Days"]=1<<2;check(nextQueueWake(q,monday)==monday+25*3600000LL,"Weekday mask skips a disabled day");
  q["Days"]=0;check(nextQueueWake(q,monday)==0,"Empty weekday mask has no wake deadline");q["Days"]=1<<1;
  q["StartMinute"]=23*60;q["StopMinute"]=2*60;
  check(nextQueueWake(q,monday)==monday+15*3600000LL,"Overnight queue wakes on the selected start day");
  check(nextQueueWake(q,utc("2026-09-29T01:00:00.000Z"))==utc("2026-10-05T23:00:00.000Z"),"Overnight continuation does not invent a second morning wake");
  q["Days"]=127;q["StartMinute"]=0;q["StopMinute"]=1440;
  check(nextQueueWake(q,monday)==0,"Continuously open daily window has no future opening");
  q["StartMinute"]=9*60;q["StopMinute"]=10*60;
  check(nextQueueWake(q,monday+3600000)==monday+25*3600000LL,"An already open window schedules only its next opening");
  q["NextRunUtc"]=date(monday+90*60000);
  check(nextQueueWake(q,monday+65*60000)==monday+90*60000,"Queue repetition can wake inside its active daily window");
  q["NextRunUtc"]=date(monday+3*3600000LL);
  check(nextQueueWake(q,monday+65*60000)==monday+25*3600000LL,"Repeat outside its daily window waits for the next opening");
  q["Scheduled"]=false;
  check(nextQueueWake(q,monday)==monday+3*3600000LL,"Unscheduled repeating queue uses its saved repeat deadline");
  q["NextRunUtc"]=date(monday-1);check(nextQueueWake(q,monday)==0,"Overdue repeat does not register a timer in the past");
  q["RunOnce"]=true;q["StartOnceUtc"]=date(monday+12345);
  check(nextQueueWake(q,monday)==monday+12345,"Run-once wake preserves its exact UTC instant");
  q["OnceStarted"]=true;check(nextQueueWake(q,monday)==0,"Started run-once queue cannot rearm its start");q["OnceStarted"]=false;
  check(nextQueueWake(q,monday+12345)==0,"Due run-once queue is left to normal scheduling");
 }
 {TestTimeZone zone("PST8PDT");auto q=defaultQueue();q["WakeComputer"]=true;q["Scheduled"]=true;q["StartMinute"]=2*60+30;q["StopMinute"]=4*60;q["Days"]=1;
  check(nextQueueWake(q,utc("2026-03-08T09:00:00.000Z"))==utc("2026-03-08T10:00:00.000Z"),"Spring clock gap wakes at the first valid minute in the scheduled window");
  q["StopMinute"]=2*60+45;
  check(nextQueueWake(q,utc("2026-03-08T09:00:00.000Z"))==utc("2026-03-15T09:30:00.000Z"),"A completely skipped spring window matches normal scheduler behavior");
  q["StartMinute"]=90;q["StopMinute"]=105;
  check(nextQueueWake(q,utc("2026-11-01T08:00:00.000Z"))==utc("2026-11-01T08:30:00.000Z"),"Autumn repeated hour first opening is scheduled correctly");
  check(nextQueueWake(q,utc("2026-11-01T08:50:00.000Z"))==utc("2026-11-01T09:30:00.000Z"),"Autumn repeated hour later opening matches the live window predicate");
 }
 {DYNAMIC_TIME_ZONE_INFORMATION zone{};bool found=false;for(DWORD index=0;;++index){auto code=EnumDynamicTimeZoneInformation(index,&zone);if(code==ERROR_NO_MORE_ITEMS)break;if(code!=ERROR_SUCCESS)throw std::runtime_error("Cannot enumerate Windows time zones for the scheduler fixture.");if(std::wstring(zone.TimeZoneKeyName)==L"Pacific Standard Time"){found=true;break;}}if(!found)throw std::runtime_error("Pacific time-zone fixture is unavailable.");zone.DynamicDaylightTimeDisabled=FALSE;
  auto winter=localScheduleTime(utc("2026-12-01T12:00:00.000Z"),&zone),summer=localScheduleTime(utc("2026-07-01T12:00:00.000Z"),&zone);
  check(winter.wHour==4&&summer.wHour==5,"Date picker conversion uses the selected date's winter or summer offset");
  check(utcScheduleTime(winter,&zone)==utc("2026-12-01T12:00:00.000Z")&&utcScheduleTime(summer,&zone)==utc("2026-07-01T12:00:00.000Z"),"Date picker round trips across daylight-saving seasons");
  SYSTEMTIME gap{};gap.wYear=2026;gap.wMonth=3;gap.wDay=8;gap.wHour=2;gap.wMinute=30;
  rejects([&]{utcScheduleTime(gap,&zone);},"Date picker rejects a nonexistent spring clock-change time");
  SYSTEMTIME autumn{};autumn.wYear=2026;autumn.wMonth=11;autumn.wDay=1;autumn.wHour=1;autumn.wMinute=30;
  check(utcScheduleTime(autumn,&zone)==utc("2026-11-01T08:30:00.000Z"),"A new repeated autumn date selects its first occurrence");
  auto early=utc("2026-11-01T08:30:27.123Z"),late=utc("2026-11-01T09:30:15.000Z");
  check(editedScheduleTime(autumn,date(early),&zone)==early,"Applying an unchanged first autumn occurrence preserves its UTC instant and hidden seconds");
  check(editedScheduleTime(autumn,date(late),&zone)==late,"Applying an unchanged second autumn occurrence preserves its UTC instant");
  autumn.wMinute=40;check(editedScheduleTime(autumn,date(late),&zone)==utc("2026-11-01T08:40:00.000Z"),"Editing an ambiguous date applies the documented first-occurrence rule");
  check(localScheduleTime(utc("1965-12-01T12:00:00.000Z"),&zone).wYear==1965,"Historical dates before the Unix epoch remain displayable");
 }
 auto q=defaultQueue();q["WakeComputer"]=true;q["RunOnce"]=true;q["StartOnceUtc"]=date(epoch()+120000);
 auto job=std::make_shared<Job>(Json{{"Id",guid()},{"Url","https://example.invalid/file"},{"Queue","Main queue"},{"Status","Paused"},{"QueueMember",true}});std::vector<JobPtr> jobs{job};
 check(queueHasWakeWork(q,jobs),"Paused queue members are eligible for a scheduled wake");
 for(const char* field:{"RequiresRequestCapture","RequiresMediaCapture","ConfirmationPending"}){job->data[field]=true;check(!queueHasWakeWork(q,jobs),"Browser recapture or pending confirmation cannot trigger a wake");job->data.erase(field);}
 job->data["DuplicateOf"]="existing";check(!queueHasWakeWork(q,jobs),"Unresolved duplicate prompt cannot trigger a wake");job->data.erase("DuplicateOf");
 job->data["PostAttempted"]=true;check(!queueHasWakeWork(q,jobs),"Wake cannot automatically replay an already submitted form");job->data.erase("PostAttempted");
 job->data["QueueMember"]=false;check(!queueHasWakeWork(q,jobs),"Removed queue member cannot trigger a wake");job->data["QueueMember"]=true;
 job->data["Status"]="Complete";check(!queueHasWakeWork(q,jobs),"Completed file alone does not wake the computer");q["Synchronize"]=true;
 check(queueHasWakeWork(q,jobs),"Completed HTTP file can wake for requested synchronization");job->data["Url"]="ftp://example.invalid/file";
 check(!queueHasWakeWork(q,jobs),"Unsupported FTP synchronization cannot trigger a wake");job->data["Url"]="https://example.invalid/file";job->data["Status"]="Paused";
 {QueueWakeTimer timer;auto now=epoch();q["StartOnceUtc"]=date(now+800);timer.update(Json::array({q}),jobs,now);auto status=timer.snapshot();
  check(str(status,"Status")=="Armed"||str(status,"Status")=="Unsupported","Windows accepts the real absolute waitable timer request");
  auto until=GetTickCount64()+3000;while(!timer.signaled()&&GetTickCount64()<until)Sleep(10);
  check(timer.signaled(),"Real Windows wake timer signals at its UTC deadline while awake");
  timer.stop();check(str(timer.snapshot(),"Status")=="Off"&&!timer.signaled(),"Stopping releases an already signaled timer");
 }
 {QueueWakeTimer timer;auto now=epoch();q["StartOnceUtc"]=date(now+120000);auto other=q;other["Name"]="Other";other["StartOnceUtc"]=date(now+60000);auto b=std::make_shared<Job>(job->data);b->data["Queue"]="Other";jobs.push_back(b);
  timer.update(Json::array({q,other}),jobs,now);check(str(timer.snapshot(),"Queue")=="Other","Earliest eligible queue owns the one application wake timer");
  other["Enabled"]=false;timer.update(Json::array({q,other}),jobs,now+1);check(str(timer.snapshot(),"Queue")=="Main queue","Disabling earliest queue rearms for the next eligible one");
  q["StartOnceUtc"]=date(now+180000);timer.update(Json::array({q}),jobs,now+2);check(parseDate(timer.snapshot()["NextWakeUtc"])==now+180000,"Changing the scheduled date replaces the existing timer");
  timer.update(Json::array({q}),{},now+3);check(str(timer.snapshot(),"Status")=="Waiting"&&!timer.signaled(),"Removing all queue work cancels the Windows wake timer");
  q["WakeComputer"]=false;timer.update(Json::array({q}),jobs,now+4);check(str(timer.snapshot(),"Status")=="Off","Clearing wake opt-in cancels the timer immediately");q["WakeComputer"]=true;
 }
 {Manager m(root/L"wake-manager");auto bad=q;bad["WakeComputer"]="true";auto before=m.state["Queues"];rejects([&]{m.setQueue(bad);},"Queue validation rejects a non-boolean wake setting");check(m.state["Queues"]==before,"Invalid wake setting preserves saved queues");
  auto j=m.add("https://example.invalid/file","","wake.bin","Main queue",true);q["StartOnceUtc"]=date(epoch()+120000);m.setQueue(q);
  check(str(m.queueWakeStatus(),"Status")=="Armed"||str(m.queueWakeStatus(),"Status")=="Unsupported","Applying a queue registers its wake without a UI polling delay");
  auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);auto disabled=q;disabled["WakeComputer"]=false;rejects([&]{m.setQueue(disabled);},"Failed settings persistence does not apply a wake change");fs::remove(blocked);
  check(str(m.queueWakeStatus(),"Status")!="Off"&&yes(m.state["Queues"][0],"WakeComputer"),"Failed settings save preserves the applied timer and opt-in");
  m.stop();check(str(m.queueWakeStatus(),"Status")=="Off","Application shutdown cancels its wake timer");
 }
 {Manager m(root/L"wake-manager");check(yes(m.state["Queues"][0],"WakeComputer"),"Wake preference survives process-state reload");m.tick();check(str(m.queueWakeStatus(),"Status")=="Armed"||str(m.queueWakeStatus(),"Status")=="Unsupported","Reloaded application rearms a future eligible schedule");m.queueRun("Main queue",false);check(str(m.queueWakeStatus(),"Status")=="Off","Stopping a queue removes its wake request immediately");m.stop();}
}
