#pragma once
#include "Core.hpp"
#include "DiagnosticSpan.hpp"
#include <winioctl.h>
namespace udm {
struct PublishedWritebackResult {
 double openSeconds=0,normalizeSeconds=0,flushSeconds=0,closeSeconds=0;
 DWORD openError=0,normalizeError=0,flushError=0,closeError=0;
 bool confirmed=false;
};
struct PublishedWritebackOps {
 HANDLE open(const fs::path& path,DWORD access){return CreateFileW(path.c_str(),access,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);}
 bool normalize(HANDLE file){FILE_SET_SPARSE_BUFFER request{};DWORD returned=0;return DeviceIoControl(file,FSCTL_SET_SPARSE,&request,sizeof(request),nullptr,0,&returned,nullptr)!=FALSE;}
 bool flush(HANDLE file){DiagnosticSpan flush("body-flush-begin","body-flush-end");return FlushFileBuffers(file)!=FALSE;}
 bool close(HANDLE file){DiagnosticSpan close("body-close-begin","body-close-end");return CloseHandle(file)!=FALSE;}
};
template<class Ops> PublishedWritebackResult publishedWriteback(const fs::path& path,bool normalize,Ops& ops){
 PublishedWritebackResult result;const auto elapsed=[](auto start){return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
 auto start=std::chrono::steady_clock::now();auto file=ops.open(path,GENERIC_WRITE);const auto openError=GetLastError();result.openSeconds=elapsed(start);
 if(file==INVALID_HANDLE_VALUE||!file){result.openError=openError;return result;}
 struct Owned {Ops& ops;HANDLE file;~Owned(){if(file!=INVALID_HANDLE_VALUE)try{ops.close(file);}catch(...){}}} owner{ops,file};
 bool normalized=true;if(normalize){start=std::chrono::steady_clock::now();normalized=ops.normalize(file);const auto error=GetLastError();result.normalizeSeconds=elapsed(start);if(!normalized)result.normalizeError=error;}
 start=std::chrono::steady_clock::now();const bool flushed=ops.flush(file);const auto flushError=GetLastError();result.flushSeconds=elapsed(start);if(!flushed)result.flushError=flushError;
 start=std::chrono::steady_clock::now();const bool closed=ops.close(file);const auto closeError=GetLastError();result.closeSeconds=elapsed(start);owner.file=INVALID_HANDLE_VALUE;if(!closed)result.closeError=closeError;
 result.confirmed=normalized&&flushed&&closed;return result;
}
inline PublishedWritebackResult publishedWriteback(const fs::path& path,bool normalize=false){PublishedWritebackOps ops;return publishedWriteback(path,normalize,ops);}
}
