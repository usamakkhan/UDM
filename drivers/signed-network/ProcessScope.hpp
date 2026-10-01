#pragma once
#include <tlhelp32.h>
#include <map>
#include <mutex>
namespace udmnet {
inline uint64_t fileTimeValue(FILETIME t){ULARGE_INTEGER u{};u.LowPart=t.dwLowDateTime;u.HighPart=t.dwHighDateTime;return u.QuadPart;}
inline DWORD processParent(HANDLE handle,DWORD pid) {
    // Resolve from a held process object first, avoiding a snapshot per socket event.
    struct BasicInfo {LONG exitStatus;PVOID peb;ULONG_PTR affinity;LONG priority;ULONG_PTR process,parent;};
    using Query=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);
    static auto query=reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"NtQueryInformationProcess"));
    BasicInfo info{};ULONG size=0;
    if(query&&query(handle,0,&info,sizeof(info),&size)==0&&size==sizeof(info)&&info.process==pid&&info.parent<=MAXDWORD)return (DWORD)info.parent;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return 0;
    PROCESSENTRY32W entry{sizeof(entry)};DWORD parent=0;
    if(Process32FirstW(snapshot,&entry))do{if(entry.th32ProcessID==pid){parent=entry.th32ParentProcessID;break;}}while(Process32NextW(snapshot,&entry));
    CloseHandle(snapshot);return parent;
}
class ProcessScope {
    struct Identity {
        HANDLE h=nullptr;uint64_t created=0;DWORD parent=0;
        explicit Identity(DWORD pid,DWORD parentPid=0):parent(parentPid) {
            h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid);
            if(!h)throw error("Cannot identify scoped process");
            FILETIME c{},e{},k{},u{};
            if(!GetProcessTimes(h,&c,&e,&k,&u)||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){CloseHandle(h);h=nullptr;throw std::runtime_error("Scoped process has exited.");}
            created=fileTimeValue(c);if(!parent)parent=processParent(h,pid);
        }
        ~Identity(){if(h)CloseHandle(h);}
        bool alive()const{return WaitForSingleObject(h,0)==WAIT_TIMEOUT;}
    };
    std::map<DWORD,std::unique_ptr<Identity>> known;std::set<DWORD> roots;
    bool children;mutable std::mutex mutex;
    uint64_t failures=0;
    bool resolve(DWORD pid,unsigned depth) {
        if(pid<=4||(pid==GetCurrentProcessId()&&roots.count(pid)==0)||depth>32)return false;
        auto existing=known.find(pid);
        if(existing!=known.end()) {
            if(existing->second->alive())return true;
            // An exited root never grants scope to a reused PID.
            if(roots.count(pid))return false;known.erase(existing);
        }
        std::unique_ptr<Identity> identity;
        try{identity=std::make_unique<Identity>(pid);}catch(...){++failures;return false;}
        DWORD parent=identity->parent;if(parent==pid||!parent)return false;
        if(!resolve(parent,depth+1))return false;
        auto p=known.find(parent);if(p==known.end()||!p->second->alive()||identity->created<p->second->created)return false;
        for(auto it=known.begin();it!=known.end();)if(!roots.count(it->first)&&!it->second->alive())it=known.erase(it);else ++it;
        if(known.size()>=256){++failures;return false;}
        known.emplace(pid,std::move(identity));return true;
    }
public:
    explicit ProcessScope(const std::vector<DWORD>& pids,bool includeChildren):children(includeChildren) {
        pidFilter(pids);
        for(auto pid:pids){known.emplace(pid,std::make_unique<Identity>(pid));roots.insert(pid);}
    }
    bool matches(DWORD pid) {
        std::lock_guard<std::mutex> lock(mutex);
        auto found=known.find(pid);if(found!=known.end()&&found->second->alive())return true;
        if(!children||roots.count(pid))return false;
        return resolve(pid,0);
    }
    uint64_t creation(DWORD pid)const {
        std::lock_guard<std::mutex> lock(mutex);auto it=known.find(pid);return it==known.end()?0:it->second->created;
    }
    DWORD parent(DWORD pid)const {
        std::lock_guard<std::mutex> lock(mutex);auto it=known.find(pid);return it==known.end()?0:it->second->parent;
    }
    uint64_t missed()const{std::lock_guard<std::mutex> lock(mutex);return failures;}
};
}
