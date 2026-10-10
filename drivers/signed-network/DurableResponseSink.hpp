#pragma once
#include "HttpStream.hpp"
#include <windows.h>
#include <array>
#include <stdexcept>
namespace udmnet {
// Handles must be freshly created by the recipient in its own security context.
// The elevated broker never chooses a destination path or opens a user file.
class DurableResponseSink final:public ResponseSink {
    struct File {
        HANDLE h;
        explicit File(HANDLE value):h(value){}
        ~File(){if(h&&h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
        File(const File&)=delete;
    };
    using Record=std::array<uint64_t,8>;
    File body,journal;
    uint64_t received=0,expected=0;bool known=false,terminal=false;
    static constexpr uint64_t Magic=0x31504c53524d4455ull;
    static std::runtime_error failure(){return std::runtime_error("Cannot persist original response (Windows error "+std::to_string(GetLastError())+").");}
    static uint64_t size(HANDLE h){LARGE_INTEGER n{};if(GetFileType(h)!=FILE_TYPE_DISK||!GetFileSizeEx(h,&n)||n.QuadPart<0)throw failure();return static_cast<uint64_t>(n.QuadPart);}
    static uint64_t checksum(const Record& r){uint64_t value=14695981039346656037ull;for(size_t i=0;i<6;++i)for(unsigned shift=0;shift<64;shift+=8){value^=(r[i]>>shift)&255;value*=1099511628211ull;}return value;}
    static Record record(uint64_t state,uint64_t bytes,bool lengthKnown,uint64_t length){Record r{Magic,1,state,bytes,lengthKnown?1ull:0ull,length,0,0};r[6]=checksum(r);r[7]=~r[6];return r;}
    static bool valid(const Record& r){return r[0]==Magic&&r[1]==1&&r[4]<=1&&r[6]==checksum(r)&&r[7]==~r[6];}
    static void seek(HANDLE h,uint64_t offset){LARGE_INTEGER n{};n.QuadPart=static_cast<LONGLONG>(offset);if(!SetFilePointerEx(h,n,nullptr,FILE_BEGIN))throw failure();}
    static void writeAll(HANDLE h,const void* bytes,size_t length){auto p=static_cast<const char*>(bytes);while(length){DWORD n=0;auto part=static_cast<DWORD>(std::min<size_t>(length,1024*1024));if(!WriteFile(h,p,part,&n,nullptr)||!n)throw failure();p+=n;length-=n;}}
public:
    struct Recovery {bool complete=false;uint64_t received=0;bool lengthKnown=false;uint64_t expected=0;};
    // Ownership of both handles transfers even if construction fails. A caller
    // must persist its job-to-handle/file mapping before returning this sink.
    DurableResponseSink(HANDLE bodyHandle,HANDLE journalHandle,bool lengthKnown,uint64_t length)
        :body(bodyHandle),journal(journalHandle),expected(length),known(lengthKnown){
        if(bodyHandle==journalHandle){journal.h=INVALID_HANDLE_VALUE;throw std::runtime_error("Response body and receipt must be separate files.");}
        BY_HANDLE_FILE_INFORMATION a{},b{};
        if(!GetFileInformationByHandle(body.h,&a)||!GetFileInformationByHandle(journal.h,&b))throw failure();
        if(a.dwVolumeSerialNumber==b.dwVolumeSerialNumber&&a.nFileIndexHigh==b.nFileIndexHigh&&a.nFileIndexLow==b.nFileIndexLow)throw std::runtime_error("Response body aliases its receipt.");
        if(size(body.h)||size(journal.h)||length>static_cast<uint64_t>(INT64_MAX))throw std::runtime_error("Response reservation requires new empty files and a valid length.");
        seek(body.h,0);seek(journal.h,0);auto r=record(1,0,known,expected);writeAll(journal.h,r.data(),sizeof(r));
        if(!FlushFileBuffers(journal.h))throw failure();
    }
    ~DurableResponseSink(){abort();}
    void write(std::string_view bytes)override{
        if(terminal)throw std::runtime_error("Response ownership is closed.");
        if(bytes.size()>static_cast<uint64_t>(INT64_MAX)-received||(known&&(received>expected||bytes.size()>expected-received)))throw std::runtime_error("Original response exceeds its declared length.");
        writeAll(body.h,bytes.data(),bytes.size());received+=bytes.size();
    }
    void complete()override{
        if(terminal)throw std::runtime_error("Response ownership is closed.");
        if(known&&received!=expected)throw std::runtime_error("Original response is truncated.");
        if(!FlushFileBuffers(body.h))throw failure();
        auto r=record(2,received,known,expected);seek(journal.h,sizeof(Record));writeAll(journal.h,r.data(),sizeof(r));
        if(!FlushFileBuffers(journal.h))throw failure();terminal=true;
    }
    void abort()noexcept override{if(!terminal){terminal=true;FlushFileBuffers(body.h);}}
    static Recovery recover(HANDLE bodyHandle,HANDLE journalHandle){
        auto bodySize=size(bodyHandle),journalSize=size(journalHandle);
        if(journalSize<sizeof(Record)||journalSize>2*sizeof(Record))throw std::runtime_error("Invalid response ownership receipt.");
        Record first{};DWORD n=0;seek(journalHandle,0);
        if(!ReadFile(journalHandle,first.data(),sizeof(first),&n,nullptr)||n!=sizeof(first)||!valid(first)||first[2]!=1||first[3]!=0)throw std::runtime_error("Invalid response reservation.");
        Recovery result{false,bodySize,first[4]!=0,first[5]};
        if(result.lengthKnown&&bodySize>result.expected)throw std::runtime_error("Recovered response exceeds its expected size.");
        if(journalSize==2*sizeof(Record)){
            Record last{};if(!ReadFile(journalHandle,last.data(),sizeof(last),&n,nullptr)||n!=sizeof(last))throw failure();
            result.complete=valid(last)&&last[2]==2&&last[3]==bodySize&&last[4]==first[4]&&last[5]==first[5]&&(!result.lengthKnown||bodySize==result.expected);
        }
        return result;
    }
};
}
