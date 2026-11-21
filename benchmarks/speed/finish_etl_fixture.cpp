// Tiny, process-local test producer. No kernel provider or elevation request.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <evntrace.h>
#include <TraceLoggingProvider.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
namespace fs=std::filesystem;
TRACELOGGING_DEFINE_PROVIDER(readerFixture,"UDM.FinishReader.Smoke",(0x4c78d6ef,0x7bb7,0x4ea4,0xa3,0xa8,0x4d,0x95,0x12,0xcc,0x69,0x5e));
static const GUID providerGuid={0x4c78d6ef,0x7bb7,0x4ea4,{0xa3,0xa8,0x4d,0x95,0x12,0xcc,0x69,0x5e}};
int wmain(int argc,wchar_t** argv){
 if(argc!=2||IsUserAnAdmin())return 2;fs::path file=argv[1];if(!file.is_absolute()||fs::exists(file)||!fs::is_directory(file.parent_path()))return 2;
 auto registered=TraceLoggingRegister(readerFixture);if(registered!=ERROR_SUCCESS)return 2;
 struct ProviderOwner {~ProviderOwner(){TraceLoggingUnregister(readerFixture);}} provider;
 auto name=L"UDMReaderFixture-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());auto path=file.wstring();
 size_t nameBytes=(name.size()+1)*sizeof(wchar_t),pathBytes=(path.size()+1)*sizeof(wchar_t);std::vector<BYTE> buffer(sizeof(EVENT_TRACE_PROPERTIES)+nameBytes+pathBytes);
 auto props=reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.data());props->Wnode.BufferSize=static_cast<ULONG>(buffer.size());props->Wnode.Guid=providerGuid;props->Wnode.Flags=WNODE_FLAG_TRACED_GUID;props->Wnode.ClientContext=1;
 props->BufferSize=4;props->MinimumBuffers=2;props->MaximumBuffers=2;props->MaximumFileSize=1;
 props->LogFileMode=EVENT_TRACE_FILE_MODE_SEQUENTIAL|EVENT_TRACE_PRIVATE_LOGGER_MODE|EVENT_TRACE_PRIVATE_IN_PROC;
 props->LoggerNameOffset=sizeof(EVENT_TRACE_PROPERTIES);props->LogFileNameOffset=static_cast<ULONG>(sizeof(EVENT_TRACE_PROPERTIES)+nameBytes);
 memcpy(buffer.data()+props->LoggerNameOffset,name.c_str(),nameBytes);memcpy(buffer.data()+props->LogFileNameOffset,path.c_str(),pathBytes);
 TRACEHANDLE session=0;auto started=StartTraceW(&session,name.c_str(),props);
 if(started!=ERROR_SUCCESS){std::cout<<"{\"started\":false,\"status\":"<<started<<",\"elevationRequested\":false}\n";return started==ERROR_ACCESS_DENIED?77:2;}
 struct SessionOwner {TRACEHANDLE handle;EVENT_TRACE_PROPERTIES* props;std::wstring name;bool stopped=false;~SessionOwner(){if(!stopped)ControlTraceW(handle,name.c_str(),props,EVENT_TRACE_CONTROL_STOP);}} owner{session,props,name};
 auto enabled=EnableTraceEx2(session,&providerGuid,EVENT_CONTROL_CODE_ENABLE_PROVIDER,TRACE_LEVEL_VERBOSE,~0ULL,0,1000,nullptr);
 if(enabled!=ERROR_SUCCESS){std::cout<<"{\"started\":true,\"enableStatus\":"<<enabled<<",\"elevationRequested\":false}\n";return 2;}
 for(ULONG sequence=1;sequence<=3;++sequence)TraceLoggingWrite(readerFixture,"DecoderSmoke",TraceLoggingUInt32(GetCurrentProcessId(),"ProducerPID"),TraceLoggingUInt32(sequence,"Sequence"),TraceLoggingUInt64(0x1122334455667788ULL,"Sentinel"));
 auto stopped=ControlTraceW(session,name.c_str(),props,EVENT_TRACE_CONTROL_STOP);owner.stopped=stopped==ERROR_SUCCESS;
 std::cout<<"{\"started\":true,\"stopStatus\":"<<stopped<<",\"pid\":"<<GetCurrentProcessId()<<",\"eventsEmitted\":3,\"kernelProvidersEnabled\":0,\"processLocalOnly\":true,\"elevationRequested\":false}\n";
 return stopped==ERROR_SUCCESS?0:2;
}
