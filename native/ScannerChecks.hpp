#pragma once
#include "Scanner.hpp"
#include <shellapi.h>
static int scannerFixture(){
 using namespace udm;int count=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&count);if(!argv||count<3)return 91;
 auto spec=Json::parse(readText(fs::path(argv[2])));Json args=Json::array();for(int i=3;i<count;++i)args.push_back(utf8(argv[i]));LocalFree(argv);
 if(spec.contains("receipt"))atomicText(fs::path(wide(str(spec,"receipt"))),Json{{"arguments",args},{"pid",GetCurrentProcessId()}}.dump(),false);
 Sleep((DWORD)num(spec,"delay"));if(spec.contains("finished"))atomicText(fs::path(wide(str(spec,"finished"))),"finished",false);
 return (int)num(spec,"code");
}
static void scannerChecks(const udm::fs::path& root,Fixture& fixture){
 using namespace udm;auto base=root/L"scanner";fs::create_directories(base);auto target=base/L"日本語 & a [File] {file}.bin";writeBytes(target,Bytes{'o','k'});
 wchar_t exe[MAX_PATH*4]{};GetModuleFileNameW(nullptr,exe,(DWORD)std::size(exe));Json prefs=defaultSettings();prefs["ScanProgram"]=utf8(exe);
 auto specification=[&](std::string name,int code,int delay){auto path=base/wide(name+".json");atomicText(path,Json{{"code",code},{"delay",delay},{"receipt",utf8((base/wide(name+"-receipt.json")).wstring())},{"finished",utf8((base/wide(name+"-finished.txt")).wstring())}}.dump());return "--scanner-fixture "+utf8(quote(path.wstring()));};
 auto parse=[](const std::wstring& command){int n=0;auto av=CommandLineToArgvW(command.c_str(),&n);std::vector<std::wstring> out;for(int i=0;i<n;++i)out.push_back(av[i]);LocalFree(av);return out;};
 auto preset=scannerPreset("C:\\Scanner\\MpCmdRun.exe");check(str(preset,"Name")=="Microsoft Defender"&&str(preset,"Arguments").find("-ScanType 3")!=std::string::npos,"Defender preset scans only the selected file");
 check(scannerPreset("C:\\Scanner\\unknown.exe").empty(),"Unknown scanner executables retain manual configuration");
 for(const char* name:{"CLAMSCAN.EXE","clamdscan.exe"})check(str(scannerPreset(std::string("C:\\Scanner\\")+name),"Name")=="ClamAV","ClamAV scanner names select documented parameters");
 Json configured={{"ScanProgram","C:\\Scanner\\clamscan.exe"},{"ScanArguments",str(scannerPreset("clamscan.exe"),"Arguments")}};
 auto clear=scannerExitResult(configured,0),found=scannerExitResult(configured,1),scanFailure=scannerExitResult(configured,2);
 check(str(clear,"Status")=="Finished"&&str(found,"Status")=="Attention"&&str(scanFailure,"Status")=="Failed","ClamAV results distinguish clean, detection and scan failure");
 configured["ScanArguments"]="custom";check(str(scannerExitResult(configured,1),"Message").find("Consult")!=std::string::npos,"Custom scanner parameters never inherit an unverified verdict mapping");
 configured={{"ScanProgram","C:\\Scanner\\MpCmdRun.exe"},{"ScanArguments",str(preset,"Arguments")}};
 check(str(scannerExitResult(configured,2),"Message").find("or a scanning error")!=std::string::npos,"Defender code two is not falsely classified as a definite malware detection");
 check(str(scannerExitResult(configured,5),"Status")=="Failed","Unexpected Defender errors are not reported as a completed check");
 prefs["ScanArguments"]="";auto args=parse(scannerCommand(prefs,target));check(args.size()==2&&args[1]==target.wstring(),"Empty scanner arguments append one quoted Unicode filename");
 prefs["ScanArguments"]="--scan --flag";args=parse(scannerCommand(prefs,target));check(args.size()==4&&args.back()==target.wstring(),"Scanner flags without a placeholder still receive the downloaded file");
 prefs["ScanArguments"]="--file={file} \"[File]\" {file}";args=parse(scannerCommand(prefs,target));check(args.size()==4&&args[1]==L"--file="+target.wstring()&&args[2]==target.wstring()&&args[3]==target.wstring(),"Both placeholder styles and repeated tokens remain individual arguments");
 prefs["ScanArguments"]="\"literal with spaces\" \"\" \"a\\\"b\" {file}";args=parse(scannerCommand(prefs,target));check(args.size()==5&&args[1]==L"literal with spaces"&&args[2].empty()&&args[3]==L"a\"b"&&args[4]==target.wstring(),"Scanner command preserves empty arguments, embedded quotes and spaces");
 auto invalid=prefs;invalid["ScanProgram"]="scanner.exe";rejects([&]{validateScannerSettings(invalid);},"Scanner rejects a relative executable path");
 invalid=prefs;invalid["ScanProgram"]=utf8((base/L"scanner.cmd").wstring());rejects([&]{validateScannerSettings(invalid);},"Scanner settings require an executable rather than a shell script");
 invalid=prefs;invalid["ScanArguments"]=std::string("abc\0def",7);rejects([&]{validateScannerSettings(invalid);},"Scanner rejects embedded command nulls");
 invalid=prefs;invalid["ScanArguments"]="--scan\n{file}";rejects([&]{validateScannerSettings(invalid);},"Scanner rejects multiline commands");
 for(auto seconds:{0,3601}){invalid=prefs;invalid["ScanTimeoutSeconds"]=seconds;rejects([&]{validateScannerSettings(invalid);},"Scanner wait timeout validates bounds");}
 Cancel cancel;prefs["ScanArguments"]=specification("ok",0,100)+" {file}";auto result=runScanner(prefs,target,cancel);auto receipt=Json::parse(readText(base/L"ok-receipt.json"));
 check(str(result,"Status")=="Finished"&&num(result,"ExitCode",-1)==0&&receipt["arguments"]==Json::array({utf8(target.wstring())}),"Real scanner process receives exact Unicode path and records exit zero");
 check(readText(target)=="ok","Scanner monitoring preserves the downloaded bytes");
 prefs["ScanArguments"]=specification("attention",2,0);result=runScanner(prefs,target,cancel);check(str(result,"Status")=="Attention"&&num(result,"ExitCode")==2,"Nonzero scanner code is attention, not a claimed malware verdict");
 prefs["ScanArguments"]=specification("259",259,0);result=runScanner(prefs,target,cancel);check(str(result,"Status")=="Attention"&&num(result,"ExitCode")==259,"A signaled process with exit 259 is finished, not still running");
 invalid=prefs;invalid["ScanProgram"]=utf8((base/L"missing.exe").wstring());result=runScanner(invalid,target,cancel);check(str(result,"Status")=="Failed","Missing scanner is reported without failing the downloaded file");
 writeBytes(base/L"invalid.exe",Bytes{'n','o'});invalid["ScanProgram"]=utf8((base/L"invalid.exe").wstring());result=runScanner(invalid,target,cancel);check(str(result,"Status")=="Failed"&&str(result,"Message").find("Windows error")!=std::string::npos,"Process launch failure includes a Windows error code");
 result=runScanner(prefs,base/L"absent.bin",cancel);check(str(result,"Status")=="Failed","A missing downloaded file is not passed to the scanner");
 Cancel stopped;stopped.stop=true;prefs["ScanArguments"]=specification("notstarted",0,0);result=runScanner(prefs,target,stopped);check(str(result,"Status")=="Interrupted"&&!fs::exists(base/L"notstarted-receipt.json"),"Cancellation before launch starts no scanner process");
 prefs["ScanTimeoutSeconds"]=1;prefs["ScanArguments"]=specification("timeout",0,1800);auto began=GetTickCount64();result=runScanner(prefs,target,cancel);check(str(result,"Status")=="Timed out"&&GetTickCount64()-began<1600,"Scanner monitoring has a bounded timeout");
 for(int i=0;i<250&&!fs::exists(base/L"timeout-finished.txt");++i)Sleep(10);check(fs::exists(base/L"timeout-finished.txt"),"Timeout does not terminate the user's antivirus process");
 prefs["ScanTimeoutSeconds"]=300;prefs["ScanArguments"]=specification("cancel",0,1800);Cancel interrupted;auto future=std::async(std::launch::async,[&]{return runScanner(prefs,target,interrupted);});
 for(int i=0;i<200&&!fs::exists(base/L"cancel-receipt.json");++i)Sleep(10);began=GetTickCount64();interrupted.stop=true;result=future.get();check(str(result,"Status")=="Interrupted"&&GetTickCount64()-began<750,"Stopping a scan wait is prompt");
 for(int i=0;i<250&&!fs::exists(base/L"cancel-finished.txt");++i)Sleep(10);check(fs::exists(base/L"cancel-finished.txt"),"Stopping monitoring leaves the scanner able to finish");
 Json record={{"Status","Complete"},{"ScanResult",{{"Status","Running"},{"StartedUtc",date()}}},{"CompletionActionArmed",true}};recoverScanner(record);check(str(record["ScanResult"],"Status")=="Interrupted"&&!yes(record,"CompletionActionArmed")&&str(record,"Status")=="Complete","Crash recovery preserves completed file status and marks the scan interrupted");
 check(scannerAllowsCompletion(Json::object())&&!scannerAllowsCompletion(record),"Completion actions allow no scanner and reject an incomplete check");
 record["ScanResult"]={{"Status","Finished"},{"ExitCode",0}};check(scannerAllowsCompletion(record),"Observed exit zero permits configured completion actions");
 for(auto state:{"Running","Failed","Attention","Timed out","Interrupted"}){record["ScanResult"]={{"Status",state},{"ExitCode",0}};check(!scannerAllowsCompletion(record),"Unsuccessful scanner states cannot trigger completion actions");}
 auto configure=[&](Manager& m,const char* scenario,int code,int delay){auto p=m.state["Settings"];p["DownloadFolder"]=utf8((base/wide(scenario)).wstring());p["CategoryFolders"]=false;p["ProxyMode"]="Connect directly";p["Connections"]=2;p["ScanProgram"]=utf8(exe);p["ScanArguments"]=specification(scenario,code,delay);m.setSettings(p);};
 auto wait=[&](Manager& m,JobPtr job){for(int i=0;i<1200;++i){m.tick();if(!m.isActive(job)&&str(job->data,"Status")=="Complete")return;Sleep(10);}throw std::runtime_error("Scanner manager fixture timed out.");};
 {
  Manager m(base/L"manager-success");configure(m,"managed",0,1200);auto q=defaultQueue();q["FinishAction"]="Exit UDM";m.setQueue(q);std::atomic_int completed{0};m.event=[&](JobPtr j,bool done){if(done&&str(j->data.value("ScanResult",Json::object()),"Status")=="Finished")++completed;};
  auto job=m.add(fixture.url("/range"),"","managed.bin","Main queue",false);m.setCompletionAction(job,"Open downloaded file",30,false);m.tick();
  for(int i=0;i<700&&!fs::exists(base/L"managed-receipt.json");++i)Sleep(10);
  check(m.isActive(job)&&str(job->data["ScanResult"],"Status")=="Running"&&completed==0&&m.takeDownloadCompletion(job).empty()&&m.takeQueueCompletions().empty(),"Downloads retain active ownership and defer events/actions while scanning");
  rejects([&]{m.relocate(job,base/L"moved.bin");},"Moving a file under an active scanner is rejected");
  wait(m,job);m.tick();check(readText(job->target())==fixture.payload&&completed==1&&!m.takeDownloadCompletion(job).empty(),"Completion event follows scanner exit and exact downloaded bytes survive");
  check(m.takeQueueCompletions().size()==1,"Queue completion is released after a successful scanner exit");
  auto before=fixture.requests.load();m.scanAgain(job);wait(m,job);check(fixture.requests==before&&completed==1,"Manual rescan makes no network request and does not replay download completion");
  m.stop();Manager restored(m.root);check(str(restored.jobs[0]->data["ScanResult"],"Status")=="Finished"&&str(restored.jobs[0]->data,"Status")=="Complete","Scanner result survives app restart");
 }
 {
  Manager m(base/L"manager-attention");configure(m,"managed-error",7,10);auto q=defaultQueue();q["FinishAction"]="Exit UDM";m.setQueue(q);auto job=m.add(fixture.url("/range"),"","attention.bin","Main queue",false);m.setCompletionAction(job,"Open downloaded file",30,false);wait(m,job);m.tick();
  check(str(job->data,"Status")=="Complete"&&num(job->data["ScanResult"],"ExitCode")==7&&readText(job->target())==fixture.payload,"Scanner errors are recorded separately from a successful download");
  check(!yes(job->data,"CompletionActionArmed")&&m.takeDownloadCompletion(job).empty()&&m.takeQueueCompletions().empty(),"Scanner attention cancels download and queue completion actions");
 }
 {
  Manager m(base/L"manager-stop");configure(m,"managed-stop",0,1800);auto job=m.add(fixture.url("/range"),"","stop.bin","Main queue",false);m.tick();for(int i=0;i<700&&!fs::exists(base/L"managed-stop-receipt.json");++i)Sleep(10);began=GetTickCount64();m.stop();
  check(GetTickCount64()-began<1000&&str(job->data,"Status")=="Complete"&&str(job->data["ScanResult"],"Status")=="Interrupted","App shutdown stops monitoring promptly without undoing a completed download");
  for(int i=0;i<250&&!fs::exists(base/L"managed-stop-finished.txt");++i)Sleep(10);
 }
 {
  Manager m(base/L"manager-sync");configure(m,"sync-first",0,0);auto job=m.add(fixture.url("/range"),"","sync.bin","Main queue",false);wait(m,job);{Lock lock(m.mutex);job->data["ETag"]="\"older\"";m.save();}
  auto p=m.state["Settings"];p["ScanArguments"]=specification("sync-replaced",0,0);m.setSettings(p);auto q=defaultQueue();q["Synchronize"]=true;m.setQueue(q);m.queueRun("Main queue",true);
  for(int i=0;i<1200;++i){m.tick();bool done=false;{Lock lock(m.mutex);for(auto j:m.jobs)if(j!=job&&!m.isActive(j)&&str(j->data,"Status")=="Complete"&&str(j->data.value("ScanResult",Json::object()),"Status")=="Finished")done=true;}if(done)break;Sleep(10);}
  check(fs::exists(base/L"sync-replaced-receipt.json")&&m.jobs.size()==2,"Synchronized replacement files also run the configured scanner");
 }
 {
  Manager m(base/L"manager-restore");auto job=m.add(fixture.url("/range"),utf8(base.wstring()),"restore.bin");job->data["Status"]="Complete";job->data["ScanResult"]={{"Status","Running"}};m.save();Manager reopened(m.root);check(str(reopened.jobs[0]->data["ScanResult"],"Status")=="Interrupted","Persisted running scan is recovered without relaunching an external process");
 }
}
