#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {
struct DiagnosticSpan {
 const char* end;
 DiagnosticSpan(const char* begin,const char* finished)noexcept:end(finished){auto saved=GetLastError();nativeUiTraceRecord(begin);SetLastError(saved);}
 ~DiagnosticSpan(){auto saved=GetLastError();nativeUiTraceRecord(end);SetLastError(saved);}
};
}
