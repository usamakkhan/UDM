#pragma once
// Included only by the owned diagnostic build, never normal production targets.
namespace udm {
struct NativeUiTraceHeader {
 char magic[8];DWORD version,capacity;LONGLONG frequency;DWORD processId;
 volatile LONG next,dropped;DWORD eventSize;LONGLONG anchorQpc,anchorFileTime;char reserved[8];
};
struct NativeUiTraceEvent {volatile LONG ready;DWORD threadId;LONGLONG counter,finished;char stage[40];};
struct NativeUiTraceView {NativeUiTraceHeader header;NativeUiTraceEvent events[128];};
static_assert(sizeof(NativeUiTraceHeader)==64&&sizeof(NativeUiTraceEvent)==64);
struct NativeUiTraceSink {
 Handle file,mapping;NativeUiTraceView* view=nullptr;
 ~NativeUiTraceSink(){if(view)UnmapViewOfFile(view);}
};
inline NativeUiTraceSink nativeUiTraceSink;
// Catalog ownership observation ends at the first File Info paint, keeping
// later transfer checkpoints out of this bounded startup/capture trace.
inline volatile LONG nativeUiTraceSaveWindow=1;
inline thread_local const char* nativeUiTraceSaveKind=nullptr;
struct NativeUiTraceSaveScope {
 const char* previous;
 explicit NativeUiTraceSaveScope(bool automatic):previous(nativeUiTraceSaveKind){nativeUiTraceSaveKind=automatic?"auto-":"forced-";}
 ~NativeUiTraceSaveScope(){nativeUiTraceSaveKind=previous;}
};
inline bool nativeUiTraceInitialize(){
 try{
  wchar_t pathText[32768]{};const auto length=GetEnvironmentVariableW(L"UDM_TEST_UI_TRACE_FILE",pathText,32768);
  if(!length)return true;
  if(length>=32768)return false;
  const fs::path path(pathText);wchar_t image[32768]{};
  const auto imageLength=GetModuleFileNameW(nullptr,image,32768);
  if(!imageLength||imageLength>=32768||!path.is_absolute()||path.filename()!=L"ui-timing.bin"||
     !fs::is_regular_file(path.parent_path()/L".udm-live-file-info-fixture")||
     fs::path(image).parent_path()!=path.parent_path()/L"native")return false;
  auto& sink=nativeUiTraceSink;
  sink.file.h=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
  if(!sink.file)return false;
  sink.mapping.h=CreateFileMappingW(sink.file.h,nullptr,PAGE_READWRITE,0,sizeof(NativeUiTraceView),nullptr);
  if(!sink.mapping)return false;
  sink.view=static_cast<NativeUiTraceView*>(MapViewOfFile(sink.mapping.h,FILE_MAP_READ|FILE_MAP_WRITE,0,0,sizeof(NativeUiTraceView)));
  if(!sink.view)return false;
  ZeroMemory(sink.view,sizeof(NativeUiTraceView));auto& header=sink.view->header;
  memcpy(header.magic,"UDMUIT1",8);header.version=1;header.capacity=128;header.processId=GetCurrentProcessId();header.eventSize=sizeof(NativeUiTraceEvent);
  LARGE_INTEGER frequency{},anchor{};FILETIME wall{};if(!QueryPerformanceFrequency(&frequency)||!QueryPerformanceCounter(&anchor))return false;
  GetSystemTimeAsFileTime(&wall);header.frequency=frequency.QuadPart;header.anchorQpc=anchor.QuadPart;
  header.anchorFileTime=static_cast<LONGLONG>((static_cast<ULONGLONG>(wall.dwHighDateTime)<<32)|wall.dwLowDateTime);
  return true;
 }catch(...){return false;}
}
inline void nativeUiTraceRecord(const char* stage){
 auto view=nativeUiTraceSink.view;if(!view)return;
 const auto index=InterlockedIncrement(&view->header.next)-1;
 if(index>=128){InterlockedIncrement(&view->header.dropped);return;}
 auto& event=view->events[index];LARGE_INTEGER counter{};QueryPerformanceCounter(&counter);event.counter=counter.QuadPart;
 event.threadId=GetCurrentThreadId();const auto length=std::min<size_t>(strlen(stage),39);memcpy(event.stage,stage,length);event.stage[length]=0;
 QueryPerformanceCounter(&counter);event.finished=counter.QuadPart;InterlockedExchange(&event.ready,1);
 if(strcmp(stage,"owned-paint-returned")==0)InterlockedExchange(&nativeUiTraceSaveWindow,0);
}
inline void nativeUiTraceSaveRecord(const char* stage){
 const auto kind=nativeUiTraceSaveKind;
 if(!kind||!InterlockedCompareExchange(&nativeUiTraceSaveWindow,1,1))return;
 char label[40]{};const auto prefix=strlen(kind),length=std::min<size_t>(strlen(stage),39-prefix);
 memcpy(label,kind,prefix);memcpy(label+prefix,stage,length);nativeUiTraceRecord(label);
}
}
