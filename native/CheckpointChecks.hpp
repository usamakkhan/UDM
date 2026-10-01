#pragma once
static void checkpointChecks(const udm::fs::path& root){
 using namespace udm;
 Manager manager(root/L"checkpoint-state");manager.save();const auto file=manager.root/L"state.json";
 const auto stamp=fs::last_write_time(file)-std::chrono::hours(24);fs::last_write_time(file,stamp);
 for(int i=0;i<6;++i)manager.tick();
 check(fs::last_write_time(file)==stamp,"Idle checkpoints avoid unchanged state writes and flushes");
 manager.state["Settings"]["Retries"]=1;for(int i=0;i<3;++i)manager.tick();
 check(Json::parse(readText(file))["Settings"]["Retries"]==1&&fs::last_write_time(file)!=stamp,"Changed checkpoint is persisted atomically");
 Handle held(CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 manager.state["Settings"]["Retries"]=2;
 rejects([&]{for(int i=0;i<3;++i)manager.tick();},"Failed checkpoint is reported while prior state remains intact");
 check(Json::parse(readText(file))["Settings"]["Retries"]==1&&!manager.storageError.empty(),"Failed checkpoint does not advance the saved snapshot");
 CloseHandle(held.h);held.h=INVALID_HANDLE_VALUE;for(int i=0;i<3;++i)manager.tick();
 check(Json::parse(readText(file))["Settings"]["Retries"]==2&&manager.storageError.empty(),"Failed checkpoint is retried successfully after storage recovers");

 Handle brieflyHeld(CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
 check((bool)brieflyHeld,"Fixture holds a temporary reader without delete sharing");
 HANDLE handle=brieflyHeld.h;brieflyHeld.h=INVALID_HANDLE_VALUE;
 std::thread releaseReader([handle]{Sleep(100);CloseHandle(handle);});
 manager.state["Settings"]["Retries"]=3;bool recovered=false;
 try{manager.save();recovered=true;}catch(...){}releaseReader.join();
 check(recovered&&manager.storageError.empty()&&Json::parse(readText(file))["Settings"]["Retries"]==3,"Transient reader lock is retried without surfacing a state failure");
 check(Json::parse(readText(file.wstring()+L".bak"))["Settings"]["Retries"]==2,"Successful retry retains the previous valid checkpoint as backup");
}
