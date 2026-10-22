#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>
#include <stdexcept>
namespace udm {
struct PacWorkerReply {std::string text;DWORD exitCode=0;};
namespace pac_worker_detail {
struct Handle {
 HANDLE value=nullptr;
 ~Handle(){reset();}
 void reset(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);value=nullptr;}
 Handle()=default;Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
struct Worker {
 Handle job,process,thread,inRead,inWrite,outRead,outWrite;
 std::thread writer;
 ~Worker(){
  job.reset();
  if(process.value)WaitForSingleObject(process.value,5000);
  if(writer.joinable()){CancelSynchronousIo(writer.native_handle());writer.join();}
 }
};
}
// The caller supplies an absolute trusted helper path and an already bounded
// request frame. Only the child's pipe handles are inherited, never the job.
inline PacWorkerReply runPacWorker(const std::wstring& executable,const std::string& frame,
 DWORD timeoutMs,const std::function<bool()>& cancelled){
 using namespace pac_worker_detail;
 if(frame.size()>74003||!timeoutMs||timeoutMs>30000)throw std::runtime_error("Invalid PAC worker request.");
 if(cancelled())throw std::runtime_error("PAC evaluation cancelled.");
 std::atomic<bool> writeFailed{false};
 Worker worker;
 worker.job.value=CreateJobObjectW(nullptr,nullptr);
 JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
 if(!worker.job.value||!SetInformationJobObject(worker.job.value,JobObjectExtendedLimitInformation,&limits,sizeof(limits)))throw std::runtime_error("Cannot own PAC worker lifetime.");
 SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
 if(!CreatePipe(&worker.inRead.value,&worker.inWrite.value,&security,4096)||
    !CreatePipe(&worker.outRead.value,&worker.outWrite.value,&security,4096)||
    !SetHandleInformation(worker.inWrite.value,HANDLE_FLAG_INHERIT,0)||
    !SetHandleInformation(worker.outRead.value,HANDLE_FLAG_INHERIT,0))throw std::runtime_error("Cannot create PAC worker pipes.");
 SIZE_T size=0;InitializeProcThreadAttributeList(nullptr,1,0,&size);
 std::vector<unsigned char> storage(size);
 auto* attributes=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
 if(!InitializeProcThreadAttributeList(attributes,1,0,&size))throw std::runtime_error("Cannot restrict PAC handle inheritance.");
 struct AttributeCleanup {LPPROC_THREAD_ATTRIBUTE_LIST value;~AttributeCleanup(){DeleteProcThreadAttributeList(value);}} cleanup{attributes};
 HANDLE inherited[]={worker.inRead.value,worker.outWrite.value};
 if(!UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr))throw std::runtime_error("Cannot restrict PAC handle inheritance.");
 STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.lpAttributeList=attributes;
 startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;startup.StartupInfo.hStdInput=worker.inRead.value;
 startup.StartupInfo.hStdOutput=worker.outWrite.value;startup.StartupInfo.hStdError=worker.outWrite.value;
 PROCESS_INFORMATION process{};std::wstring command=L"\""+executable+L"\" --request";
 if(!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,nullptr,&startup.StartupInfo,&process))throw std::runtime_error("Cannot start PAC worker.");
 worker.process.value=process.hProcess;worker.thread.value=process.hThread;
 if(!AssignProcessToJobObject(worker.job.value,worker.process.value)){
  TerminateProcess(worker.process.value,1);throw std::runtime_error("Cannot attach PAC worker to its lifetime owner.");
 }
 if(ResumeThread(worker.thread.value)==static_cast<DWORD>(-1))throw std::runtime_error("Cannot resume PAC worker.");
 worker.thread.reset();worker.inRead.reset();worker.outWrite.reset();
 worker.writer=std::thread([&]{
  size_t offset=0;
  while(offset<frame.size()){
   DWORD written=0;
   if(!WriteFile(worker.inWrite.value,frame.data()+offset,static_cast<DWORD>(frame.size()-offset),&written,nullptr)||!written){writeFailed=true;break;}
   offset+=written;
  }
  worker.inWrite.reset();
 });
 const auto deadline=GetTickCount64()+timeoutMs;PacWorkerReply reply;
 for(;;){
  if(cancelled())throw std::runtime_error("PAC evaluation cancelled.");
  if(GetTickCount64()>=deadline)throw std::runtime_error("PAC evaluation timed out.");
  DWORD available=0;
  if(PeekNamedPipe(worker.outRead.value,nullptr,0,nullptr,&available,nullptr)){
   if(available){
    char buffer[4096];DWORD read=0;
    if(!ReadFile(worker.outRead.value,buffer,available<sizeof(buffer)?available:sizeof(buffer),&read,nullptr))throw std::runtime_error("Cannot read PAC worker result.");
    reply.text.append(buffer,read);if(reply.text.size()>65536)throw std::runtime_error("PAC worker result is too large.");
    continue;
   }
  }else if(GetLastError()!=ERROR_BROKEN_PIPE)throw std::runtime_error("Cannot inspect PAC worker result.");
  const auto status=WaitForSingleObject(worker.process.value,5);
  if(status==WAIT_FAILED)throw std::runtime_error("Cannot wait for PAC worker.");
  if(status==WAIT_OBJECT_0){
   // A final pass drains bytes written immediately before process exit.
   DWORD remaining=0;
   if(PeekNamedPipe(worker.outRead.value,nullptr,0,nullptr,&remaining,nullptr)&&remaining)continue;
   break;
  }
 }
 if(worker.writer.joinable())worker.writer.join();
 if(writeFailed||!GetExitCodeProcess(worker.process.value,&reply.exitCode)||reply.text.empty())throw std::runtime_error("PAC worker did not return a complete response.");
 return reply;
}
}
