#include "../../native/Core.hpp"
#include <iostream>
using namespace udm;
static DWORD handles(){DWORD n=0;GetProcessHandleCount(GetCurrentProcess(),&n);return n;}
struct HandleEntry {HANDLE value;ULONG_PTR handles,pointers;ULONG access,type,attributes,reserved;};
struct HandleSnapshot {ULONG_PTR count,reserved;HandleEntry entries[1];};
struct TypeName {USHORT length,maximum;PWSTR buffer;};
static Json handleTypes(){using QueryProcess=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);using QueryObject=LONG(NTAPI*)(HANDLE,ULONG,PVOID,ULONG,PULONG);auto module=GetModuleHandleW(L"ntdll.dll");auto process=(QueryProcess)GetProcAddress(module,"NtQueryInformationProcess");auto object=(QueryObject)GetProcAddress(module,"NtQueryObject");Bytes data(1024*1024);ULONG required=0;if(process(GetCurrentProcess(),51,data.data(),(ULONG)data.size(),&required)<0)return {{"unavailable",true}};Json counts=Json::object();auto snapshot=(HandleSnapshot*)data.data();for(ULONG_PTR i=0;i<snapshot->count&&i<4096;++i){Bytes type(4096);if(object(snapshot->entries[i].value,2,type.data(),(ULONG)type.size(),&required)>=0){auto name=(TypeName*)type.data();auto key=utf8(std::wstring(name->buffer,name->length/2));counts[key]=counts.value(key,0)+1;}}return counts;}
int wmain(int argc,wchar_t** argv){if(argc<3)return 2;try{auto url=utf8(argv[1]);fs::path root=argv[2];auto prefs=defaultSettings();Cancel cancel;Json report=Json::array();auto run=[&](const char* name,auto operation){Json samples=Json::array();for(int i=0;i<40;i++){operation(i);if(i%10==9)samples.push_back({{"iteration",i+1},{"handles",handles()}});}Sleep(1000);report.push_back({{"name",name},{"samples",samples},{"settled",handles()},{"types",handleTypes()}});std::cout<<report.back().dump()<<std::endl;};
 run("session only",[&](int){HttpSession session(prefs);});
 run("session and connection",[&](int){HttpSession session(prefs);session.connect(Url(url));});
 run("session-local pools",[&](int){auto pool=std::make_shared<HttpSession>(prefs);if(!WinHttpSetOption(pool->handle(),WINHTTP_OPTION_DISABLE_GLOBAL_POOLING,nullptr,0))throw std::runtime_error("disable pooling error "+std::to_string(GetLastError()));Http response(url,{},prefs,cancel,0,0,"",nullptr,true,pool);response.all(1,cancel);});
 run("wait-session-unload",[&](int){Handle unloaded(CreateEventW(nullptr,TRUE,FALSE,nullptr));auto pool=std::make_shared<HttpSession>(prefs);if(!WinHttpSetOption(pool->handle(),WINHTTP_OPTION_UNLOAD_NOTIFY_EVENT,&unloaded.h,sizeof(unloaded.h)))throw std::runtime_error("unload event error");{Http response(url,{},prefs,cancel,0,0,"",nullptr,true,pool);response.all(1,cancel);}pool.reset();if(WaitForSingleObject(unloaded.h,30000)!=WAIT_OBJECT_0)throw std::runtime_error("session unload timed out");});
 run("per-session requests",[&](int){Http response(url,{},prefs,cancel,0,0);response.all(1,cancel);});
 {auto pool=std::make_shared<HttpSession>(prefs);run("shared-session requests",[&](int){Http response(url,{},prefs,cancel,0,0,"",nullptr,true,pool);response.all(1,cancel);});}
 prefs["Proxy"]=Url(url).host+":"+std::to_string(Url(url).port);run("explicit-proxy sessions",[&](int){Http response(url,{},prefs,cancel,0,0);response.all(1,cancel);});auto file=root/L"handle-fixture.bin";writeBytes(file,Bytes(100,42));
 run("hash and zone marking",[&](int){fileHash(file);markZone(file);});
 run("atomic state writes",[&](int i){atomicText(root/L"handle-fixture.json",Json{{"i",i}}.dump());});
 for(int i=0;i<6;++i){Sleep(10000);std::cout<<Json{{"cooldownSeconds",(i+1)*10},{"handles",handles()}}.dump()<<std::endl;} atomicText(root/L"handle-diagnosis.json",report.dump(2));
 }catch(const std::exception& e){std::cerr<<e.what();return 1;}return 0;}
