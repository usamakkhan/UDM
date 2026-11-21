#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {
inline bool diagnosticSerialFinalBody(){wchar_t value[8]{};return GetEnvironmentVariableW(L"UDM_TEST_SERIAL_FINAL_BODY",value,8)==1&&value[0]==L'1';}
struct DiagnosticOperation {
 const char* label;LONGLONG start=0,minimum=0;
 explicit DiagnosticOperation(const char* stage,int milliseconds=1):label(stage){LARGE_INTEGER t{},f{};QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);start=t.QuadPart;minimum=f.QuadPart*milliseconds/1000;}
 ~DiagnosticOperation()noexcept{LARGE_INTEGER end{};QueryPerformanceCounter(&end);if(end.QuadPart-start<minimum)return;auto view=nativeUiTraceSink.view;if(!view)return;const auto index=InterlockedIncrement(&view->header.next)-1;if(index>=nativeUiTraceCapacity){InterlockedIncrement(&view->header.dropped);return;}auto& event=view->events[index];event.threadId=GetCurrentThreadId();event.counter=start;event.finished=end.QuadPart;const auto length=std::min<size_t>(strlen(label),39);memcpy(event.stage,label,length);event.stage[length]=0;InterlockedExchange(&event.ready,1);}
};
struct DiagnosticReceiverExit {~DiagnosticReceiverExit(){nativeUiTraceRecord("receiver-return");}};
}
