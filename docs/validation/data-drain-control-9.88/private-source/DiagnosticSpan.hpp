#pragma once
#include "Core.hpp"
#include "ProductionUiTiming.hpp"
namespace udm {struct DiagnosticSpan {const char* end;DiagnosticSpan(const char* begin,const char* finish)noexcept:end(finish){const auto error=GetLastError();nativeUiTraceRecord(begin);SetLastError(error);}~DiagnosticSpan(){const auto error=GetLastError();nativeUiTraceRecord(end);SetLastError(error);}};}
