#include "Scanner.hpp"
#include <shellapi.h>
#include <algorithm>
namespace udm {
Json scannerPreset(const std::string& program){
 auto name=lower(utf8(fs::path(wide(program)).filename().wstring()));
 if(name=="mpcmdrun.exe")return {{"Name","Microsoft Defender"},{"Arguments","-Scan -ScanType 3 -File \"{file}\""}};
 if(name=="clamscan.exe"||name=="clamdscan.exe")return {{"Name","ClamAV"},{"Arguments","--no-summary -- \"{file}\""}};
 return Json::object();
}

void validateScannerSettings(const Json& prefs,bool requireExecutable){
 auto program=str(prefs,"ScanProgram"),args=str(prefs,"ScanArguments","\"{file}\"");
 if(program.size()>32700||args.size()>16000||program.find('\0')!=std::string::npos||args.find('\0')!=std::string::npos||program.find_first_of("\r\n")!=std::string::npos||args.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Scanner program and arguments must be single lines without null characters.");
 if(num(prefs,"ScanTimeoutSeconds",300)<1||num(prefs,"ScanTimeoutSeconds",300)>3600)throw std::runtime_error("Scanner wait timeout must be 1 to 3600 seconds.");
 if(!program.empty()){
  fs::path exe(wide(program));if(!exe.is_absolute()||lower(utf8(exe.extension().wstring()))!=".exe")throw std::runtime_error("Choose the full path to a scanner .exe file.");
  if(requireExecutable&&!fs::is_regular_file(exe))throw std::runtime_error("The scanner executable is missing. Choose an existing .exe file in Virus checking settings.");
 }else if(requireExecutable)throw std::runtime_error("Configure a scanner in Options > Downloads > Virus checking settings first.");
}
std::wstring scannerCommand(const Json& prefs,const fs::path& file){
 validateScannerSettings(prefs);if(!file.is_absolute())throw std::runtime_error("Scanning requires an absolute file path.");
 auto input=L"scanner "+wide(str(prefs,"ScanArguments","\"{file}\""));int count=0;auto argv=CommandLineToArgvW(input.c_str(),&count);if(!argv)throw std::runtime_error("Cannot parse scanner arguments.");
 struct Free{LPWSTR* p;~Free(){LocalFree(p);}} guard{argv};
 auto command=quote(wide(str(prefs,"ScanProgram")));bool inserted=false;
 for(int i=1;i<count;++i){std::wstring arg=argv[i],expanded;size_t pos=0;
  // Replace tokens in the original argument only: a filename may itself contain a token.
  while(pos<arg.size()){auto a=arg.find(L"{file}",pos),b=arg.find(L"[File]",pos);auto at=std::min(a,b);if(at==std::wstring::npos){expanded+=arg.substr(pos);break;}expanded+=arg.substr(pos,at-pos);expanded+=file.wstring();pos=at+6;inserted=true;}
  command+=L" "+quote(expanded);
 }
 // IDM-compatible empty/no-placeholder arguments: append exactly one quoted path.
 if(!inserted)command+=L" "+quote(file.wstring());
 if(command.size()>32766)throw std::runtime_error("The scanner command is too long.");return command;
}
static Json scanResult(std::string status,std::string message){return {{"Status",status},{"Message",message},{"FinishedUtc",date()}};}
Json scannerExitResult(const Json& prefs,DWORD code){
 auto result=scanResult(code?"Attention":"Finished","Scanner exited with code "+std::to_string(code)+". Consult your scanner's documentation for its meaning.");
 auto preset=scannerPreset(str(prefs,"ScanProgram"));
 if(!preset.empty()&&str(prefs,"ScanArguments")==str(preset,"Arguments")){
  result["Scanner"]=str(preset,"Name");
  if(str(preset,"Name")=="ClamAV"){
   if(code==0)result["Message"]="ClamAV reports no virus found.";
   else if(code==1)result["Message"]="ClamAV reports a virus found. Do not open the file; review your scanner.";
   else {result["Status"]="Failed";result["Message"]="ClamAV could not complete the scan (exit "+std::to_string(code)+").";}
  }else if(code==0)result["Message"]="Microsoft Defender completed the scan: no malware found or detected malware remediated. Review Windows Security for details.";
  else if(code==2)result["Message"]="Microsoft Defender reports an unresolved detection or a scanning error. Review Windows Security before opening the file.";
  else {result["Status"]="Failed";result["Message"]="Microsoft Defender could not complete the scan (exit "+std::to_string(code)+"). Check its service and permissions.";}
 }
 result["ExitCode"]=(i64)code;return result;
}
Json runScanner(const Json& prefs,const fs::path& file,const Cancel& cancel){
 try{
  validateScannerSettings(prefs,true);if(!fs::is_regular_file(file))return scanResult("Failed","The downloaded file is missing; the scanner was not started.");
  if(cancel.cancelled())return scanResult("Interrupted","Scanner launch canceled.");
  auto cmd=scannerCommand(prefs,file);fs::path exe(wide(str(prefs,"ScanProgram")));auto cwd=exe.parent_path();STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};
  // Launch this exact executable without a shell or inherited handles. Do not hide a scanner's own UI.
  if(!CreateProcessW(exe.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,cwd.c_str(),&si,&pi)){
   auto code=GetLastError();return scanResult("Failed","Scanner could not start (Windows error "+std::to_string(code)+"). Check its path, permissions and command-line options.");
  }
  Handle process(pi.hProcess),thread(pi.hThread);auto began=GetTickCount64();auto budget=(ULONGLONG)num(prefs,"ScanTimeoutSeconds",300)*1000;
  for(;;){
   auto waited=WaitForSingleObject(process.h,50);
   if(waited==WAIT_OBJECT_0){DWORD code=0;if(!GetExitCodeProcess(process.h,&code))return scanResult("Failed","Cannot read the scanner's exit code.");
    auto result=scannerExitResult(prefs,code);
    if(!fs::is_regular_file(file)){result["Status"]="Attention";result["Message"]="The downloaded file is no longer present after the scanner ran. Check your antivirus history.";}
    result["ElapsedMs"]=(i64)(GetTickCount64()-began);return result;
   }
   if(waited==WAIT_FAILED)return scanResult("Failed","Cannot monitor the scanner process; it may still be running.");
   if(cancel.cancelled())return scanResult("Interrupted","Stopped waiting for the scanner; it may still be running. UDM did not terminate your antivirus.");
   if(GetTickCount64()-began>=budget)return scanResult("Timed out","Scanner wait timed out; it may still be running. Review its result before opening the file.");
  }
 }catch(const std::exception& e){return scanResult("Failed",e.what());}
}
std::string scannerSummary(const Json& record){
 auto found=record.find("ScanResult");if(found==record.end()||!found->is_object())return "Not scanned by UDM";
 return str(*found,"Status","Unknown")+": "+str(*found,"Message");
}
bool scannerAllowsCompletion(const Json& record){
 auto found=record.find("ScanResult");return found==record.end()||(found->is_object()&&str(*found,"Status")=="Finished"&&num(*found,"ExitCode",-1)==0);
}
void recoverScanner(Json& record){
 auto it=record.find("ScanResult");if(it!=record.end()&&it->is_object()&&str(*it,"Status")=="Running"){
  (*it)["Status"]="Interrupted";(*it)["Message"]="UDM closed before the scanner result was recorded. Review your antivirus or run a new check.";(*it)["FinishedUtc"]=date();record["CompletionActionArmed"]=false;
 }
}
void Manager::scanCompleted(JobPtr job,const Cancel& cancel){
 Json prefs;fs::path file;
 {Lock lock(mutex);prefs=state["Settings"];if(str(prefs,"ScanProgram").empty())return;file=job->target();job->speed=0;job->data["ScanResult"]={{"Status","Running"},{"Message","Waiting for the scanner to finish."},{"StartedUtc",date()},{"Program",utf8(fs::path(wide(str(prefs,"ScanProgram"))).filename().wstring())}};try{save();}catch(const std::exception& e){job->data["ScanResult"]["Status"]="Failed";job->data["ScanResult"]["Message"]=std::string("Cannot save scanner state: ")+e.what();job->data["CompletionActionArmed"]=false;cycleFailed[str(job->data,"Queue")]=true;return;}}
 auto result=runScanner(prefs,file,cancel);
 {Lock lock(mutex);for(auto it=result.begin();it!=result.end();++it)job->data["ScanResult"][it.key()]=it.value();
  if(!scannerAllowsCompletion(job->data)){job->data["CompletionActionArmed"]=false;cycleFailed[str(job->data,"Queue")]=true;}
  try{save();}catch(const std::exception& e){job->data["ScanResult"]["Status"]="Failed";job->data["ScanResult"]["Message"]=std::string("Cannot save scanner result: ")+e.what();job->data["CompletionActionArmed"]=false;cycleFailed[str(job->data,"Queue")]=true;}
 }
}
void Manager::scanAgain(JobPtr job){
 Lock lock(mutex);if(stopping||!job||std::find(jobs.begin(),jobs.end(),job)==jobs.end()||isActive(job)||str(job->data,"Status")!="Complete")throw std::runtime_error("Choose an idle, completed download to scan.");
 validateScannerSettings(state["Settings"],true);if(!fs::is_regular_file(job->target()))throw std::runtime_error("The downloaded file is missing.");
 auto cancel=std::make_shared<Cancel>();active[job->id()]=cancel;
 try{threads.emplace_back([this,job,cancel]{scanCompleted(job,*cancel);Lock lock(mutex);active.erase(job->id());schedulePaused.erase(job->id());try{save();}catch(...) {}});}catch(...){active.erase(job->id());throw;}
}
}
