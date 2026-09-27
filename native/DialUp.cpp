#include "DialUp.hpp"
#include <ras.h>
#include <raserror.h>
#include <algorithm>
namespace udm {
namespace {
std::runtime_error rasError(DWORD code){wchar_t buffer[1024]{};RasGetErrorStringW(code,buffer,1024);return std::runtime_error("Windows dial-up / VPN: "+(buffer[0]?utf8(buffer):"error "+std::to_string(code))+" ("+std::to_string(code)+").");}
void hangup(HRASCONN handle)noexcept{if(!handle)return;RasHangUpW(handle);auto until=GetTickCount64()+3000;while(GetTickCount64()<until){RASCONNSTATUSW status{};status.dwSize=sizeof(status);if(RasGetConnectStatusW(handle,&status)==ERROR_INVALID_HANDLE)return;Sleep(25);}}
// No callback context survives the polling worker. RAS stops notifications at
// Connected/error/hangup, and this callback never dereferences client memory.
DWORD CALLBACK dialEvent(ULONG_PTR,DWORD,HRASCONN,UINT,RASCONNSTATE,DWORD,DWORD){return 1;}
class WindowsDial:public DialBackend {
 std::map<HRASCONN,GUID> owned;
 static std::vector<RASCONNW> connections(){DWORD bytes=sizeof(RASCONNW),count=0;std::vector<RASCONNW> entries(1);DWORD result;for(int tries=0;;++tries){entries[0].dwSize=sizeof(RASCONNW);result=RasEnumConnectionsW(entries.data(),&bytes,&count);if(result!=ERROR_BUFFER_TOO_SMALL||tries==3)break;entries.resize((bytes+sizeof(RASCONNW)-1)/sizeof(RASCONNW));}if(result)throw rasError(result);entries.resize(count);return entries;}
public:
 ~WindowsDial(){try{for(const auto& c:connections()){auto item=owned.find(c.hrasconn);if(item!=owned.end()&&memcmp(&item->second,&c.guidCorrelationId,sizeof(GUID))==0)hangup(c.hrasconn);}}catch(...) {}}
 bool connected(const Json& p)override{
  DWORD bytes=sizeof(RASCONNW),count=0;std::vector<RASCONNW> entries(1);DWORD result;
  for(int tries=0;;++tries){entries[0].dwSize=sizeof(RASCONNW);result=RasEnumConnectionsW(entries.data(),&bytes,&count);if(result!=ERROR_BUFFER_TOO_SMALL||tries==3)break;entries.resize((bytes+sizeof(RASCONNW)-1)/sizeof(RASCONNW));}
  if(result)throw rasError(result);auto name=wide(str(p,"DialEntry")),book=wide(str(p,"DialPhonebook"));
  for(DWORD i=0;i<count;++i)if(_wcsicmp(entries[i].szEntryName,name.c_str())==0&&(book.empty()||_wcsicmp(entries[i].szPhonebook,book.c_str())==0)){RASCONNSTATUSW status{};status.dwSize=sizeof(status);if(RasGetConnectStatusW(entries[i].hrasconn,&status)==0&&status.rasconnstate==RASCS_Connected)return true;}return false;
 }
 void connect(const Json& p,const Cancel& cancel)override{
  cancel.check();auto name=wide(str(p,"DialEntry")),book=wide(str(p,"DialPhonebook"));auto records=dialEntries();std::wstring resolvedBook;int matches=0;for(auto& e:records)if(_wcsicmp(wide(e.name).c_str(),name.c_str())==0&&(book.empty()||_wcsicmp(wide(e.phonebook).c_str(),book.c_str())==0)){++matches;resolvedBook=wide(e.phonebook);}
  if(matches!=1)throw std::runtime_error("Choose an existing Windows dial-up / VPN connection in Options > Dial-up / VPN.");
  if(book.empty())book=resolvedBook;
  struct Params {RASDIALPARAMSW value{};~Params(){SecureZeroMemory(&value,sizeof(value));}} params;auto& args=params.value;args.dwSize=sizeof(args);wcscpy_s(args.szEntryName,name.c_str());BOOL password=FALSE;auto pb=book.empty()?nullptr:book.c_str();DWORD result=RasGetEntryDialParamsW(pb,&args,&password);if(result)throw rasError(result);
  struct Identity {RASEAPUSERIDENTITYW* value=nullptr;~Identity(){if(value)RasFreeEapUserIdentityW(value);}} identity;
  result=RasGetEapUserIdentityW(pb,name.c_str(),RASEAPF_NonInteractive,nullptr,&identity.value);
  if(result!=ERROR_SUCCESS&&result!=ERROR_INVALID_FUNCTION_FOR_ENTRY)throw rasError(result);
  RASDIALEXTENSIONS extensions{};extensions.dwSize=sizeof(extensions);
  if(identity.value){wcscpy_s(args.szUserName,identity.value->szUserName);extensions.RasEapInfo.dwSizeofEapInfo=identity.value->dwSizeofEapInfo;extensions.RasEapInfo.pbEapInfo=identity.value->pbEapInfo;}
  HRASCONN handle=nullptr;try{
   cancel.check();result=RasDialW(&extensions,pb,&args,2,(LPVOID)dialEvent,&handle);if(result)throw rasError(result);
   auto until=GetTickCount64()+(ULONGLONG)num(p,"DialTimeoutSeconds",120)*1000;
   for(;;){cancel.check();RASCONNSTATUSW status{};status.dwSize=sizeof(status);result=RasGetConnectStatusW(handle,&status);if(result)throw rasError(result);if(status.dwError)throw rasError(status.dwError);if(status.rasconnstate==RASCS_Connected){bool tracked=false;for(const auto& connection:connections())if(connection.hrasconn==handle){owned[handle]=connection.guidCorrelationId;tracked=true;break;}if(!tracked)throw std::runtime_error("Windows connection vanished before download start.");return;}if(status.rasconnstate==RASCS_Disconnected)throw rasError(ERROR_CONNECTION_ABORTED);if(GetTickCount64()>=until)throw std::runtime_error("Windows dial-up / VPN connection timed out.");cancel.wait(50);}
  }catch(...){hangup(handle);throw;}
 }
};
}
std::vector<DialEntry> dialEntries(){
 DWORD bytes=sizeof(RASENTRYNAMEW),count=0;std::vector<RASENTRYNAMEW> entries(1);DWORD result;
 for(int tries=0;;++tries){entries[0].dwSize=sizeof(RASENTRYNAMEW);result=RasEnumEntriesW(nullptr,nullptr,entries.data(),&bytes,&count);if(result!=ERROR_BUFFER_TOO_SMALL||tries==3)break;entries.resize((bytes+sizeof(RASENTRYNAMEW)-1)/sizeof(RASENTRYNAMEW));}
 if(result)throw rasError(result);std::vector<DialEntry> output;for(DWORD i=0;i<count;++i)output.push_back({utf8(entries[i].szEntryName),utf8(entries[i].szPhonebookPath)});return output;
}
void validateDialSettings(const Json& p){
 auto name=str(p,"DialEntry"),book=str(p,"DialPhonebook");if(wide(name).size()>RAS_MaxEntryName||name.find_first_of("\r\n")!=std::string::npos||name.find('\0')!=std::string::npos)throw std::runtime_error("Choose a valid Windows connection name.");
 if(yes(p,"DialEnabled")&&name.empty())throw std::runtime_error("Select a connection before enabling automatic dial-up / VPN.");
 if(!book.empty()&&(!fs::path(wide(book)).is_absolute()||book.find('\0')!=std::string::npos||book.find_first_of("\r\n")!=std::string::npos))throw std::runtime_error("The connection phone book must have an absolute path.");
 if(num(p,"DialAttempts",3)<0||num(p,"DialAttempts",3)>100||num(p,"DialRetrySeconds",10)<1||num(p,"DialRetrySeconds",10)>3600||num(p,"DialTimeoutSeconds",120)<10||num(p,"DialTimeoutSeconds",120)>600)throw std::runtime_error("Use 0-100 dial attempts (0 = until stopped), 1-3600 seconds between attempts, and a 10-600 second connection timeout.");
}
DialUp::DialUp(std::unique_ptr<DialBackend> implementation):backend(implementation?std::move(implementation):std::make_unique<WindowsDial>()){}
DialUp::~DialUp()=default;
void ensureDialConnection(const Json& prefs,const Cancel& cancel,std::function<void(const std::string&)> report){if(!yes(prefs,"DialEnabled"))return;static DialUp connection;connection.ensure(prefs,cancel,std::move(report));}
void DialUp::ensure(const Json& p,const Cancel& cancel,std::function<void(const std::string&)> report){
 if(!yes(p,"DialEnabled"))return;validateDialSettings(p);cancel.check();if(report)report("Waiting for Windows connection...");
 std::unique_lock<std::timed_mutex> lock(mutex,std::defer_lock);while(!lock.try_lock_for(std::chrono::milliseconds(50)))cancel.check();cancel.check();
 if(backend->connected(p)){if(report)report("");return;}
 auto key=str(p,"DialEntry")+"\n"+str(p,"DialPhonebook");if(key==failedKey&&GetTickCount64()<failedUntil)throw std::runtime_error(failedMessage);
 for(i64 attempt=1;;++attempt){cancel.check();try{if(report)report("Connecting to "+str(p,"DialEntry")+" (attempt "+std::to_string(attempt)+")...");backend->connect(p,cancel);failedKey.clear();failedMessage.clear();if(report)report("");return;}
  catch(const Cancelled&){throw;}catch(const std::exception& e){if(num(p,"DialAttempts",3)>0&&attempt>=num(p,"DialAttempts",3)){failedKey=key;failedMessage=e.what();failedUntil=GetTickCount64()+2000;throw;}if(report)report("Connection failed; waiting to retry...");cancel.wait((int)num(p,"DialRetrySeconds",10)*1000);if(backend->connected(p)){if(report)report("");return;}}
 }
}
}
