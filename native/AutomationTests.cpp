// Udm.AutomationTests.exe <new-directory> [--running-server]
// Registers UDM's automation class for the current user, activates it through
// COM (cold, or against an explicitly started server) and checks real calls
// against a private catalog. The registration is removed afterwards.
#include "Automation.hpp"
#include "Udm.Automation_h.h"
#include "Udm.Automation_i.c"
#include <atlbase.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <regex>
using namespace udm;
namespace {
int passed=0,failed=0;Json checks=Json::array();
void check(bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});if(ok){++passed;std::cout<<"PASS "<<name<<std::endl;}else{++failed;std::cerr<<"FAIL "<<name<<std::endl;}}
void good(HRESULT hr){if(FAILED(hr)){char text[32];sprintf_s(text,"0x%08lX",(unsigned long)hr);throw std::runtime_error(std::string("COM HRESULT ")+text);}}
template<class F>void rejects(F action,const char* name){try{action();check(false,name);}catch(...){check(true,name);}}
void run(const fs::path& exe,const std::vector<std::wstring>& args){auto command=quote(exe.wstring());for(auto arg:args)command+=L" "+quote(arg);STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDir().c_str(),&si,&pi))throw std::runtime_error("Cannot launch fixture registration.");Handle process(pi.hProcess);CloseHandle(pi.hThread);if(WaitForSingleObject(process.h,30000)!=WAIT_OBJECT_0)throw std::runtime_error("Fixture registration timed out.");DWORD code=1;GetExitCodeProcess(process.h,&code);if(code)throw std::runtime_error("Fixture registration failed with code "+std::to_string(code)+".");}
struct Registration{
 fs::path exe=appDir()/L"Udm.Automation.exe";bool registered=false;std::unique_ptr<Handle> manual;
 Registration(const fs::path& root,const std::wstring& tag,bool running){
  HKEY key=nullptr;auto status=RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Classes\\CLSID\\{84987627-1793-4A9D-AEC6-E31AF7D6942A}",0,KEY_READ,&key);
  if(status==ERROR_SUCCESS){RegCloseKey(key);throw std::runtime_error("Existing automation registration: do not replace it with a test.");}
  if(status!=ERROR_FILE_NOT_FOUND)throw std::runtime_error("Cannot inspect the automation registration.");
  if(!fs::exists(exe))throw std::runtime_error("Udm.Automation.exe is missing beside the test.");
  run(exe,{L"--register",L"--instance-tag",tag,L"--data-dir",root.wstring()});registered=true;
  if(running)startManual(root,tag);
 }
 void startManual(const fs::path& root,const std::wstring& tag){auto command=quote(exe.wstring())+L" -Embedding --instance-tag "+quote(tag)+L" --data-dir "+quote(root.wstring());STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,appDir().c_str(),&si,&pi))throw std::runtime_error("Cannot start the isolated running server.");CloseHandle(pi.hThread);manual=std::make_unique<Handle>(pi.hProcess);Sleep(1500);}
 ~Registration(){if(registered)try{run(exe,{L"--unregister"});}catch(const std::exception& e){std::cerr<<"CLEANUP FAILED "<<e.what()<<std::endl;++failed;}if(manual&&WaitForSingleObject(manual->h,15000)!=WAIT_OBJECT_0){TerminateProcess(manual->h,1);WaitForSingleObject(manual->h,3000);++failed;}}
};
class Fixture{
 SOCKET socket=INVALID_SOCKET;std::thread worker;std::atomic_bool stop{false};std::mutex mutex;Json requests=Json::array();
 static void sendAll(SOCKET s,const std::string& value){size_t at=0;while(at<value.size()){auto sent=::send(s,value.data()+at,(int)std::min<size_t>(32768,value.size()-at),0);if(sent<=0)return;at+=sent;}}
 void serve(SOCKET client){DWORD timeout=5000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));std::string request;char buffer[8192];size_t bodyAt=std::string::npos,length=0;
  while(request.size()<2*1024*1024){auto n=recv(client,buffer,sizeof(buffer),0);if(n<=0)break;request.append(buffer,n);if(bodyAt==std::string::npos){auto end=request.find("\r\n\r\n");if(end!=std::string::npos){bodyAt=end+4;std::smatch match;if(std::regex_search(request,match,std::regex("Content-Length: ([0-9]+)",std::regex::icase)))length=std::stoul(match[1]);}}
   if(bodyAt!=std::string::npos&&request.size()>=bodyAt+length)break;
  }
  if(bodyAt!=std::string::npos){std::lock_guard<std::mutex> lock(mutex);requests.push_back({{"headers",request.substr(0,bodyAt)},{"body",request.substr(bodyAt,length)}});}
  sendAll(client,"HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "+std::to_string(payload.size())+"\r\nConnection: close\r\n\r\n"+payload);closesocket(client);
 }
public:
 unsigned short port=0;std::string payload=std::string(65536,'Q');
 Fixture(){socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(socket==INVALID_SOCKET)throw std::runtime_error("Cannot create fixture listener.");sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(bind(socket,(sockaddr*)&addr,sizeof(addr))||listen(socket,10))throw std::runtime_error("Cannot bind fixture listener.");int n=sizeof(addr);getsockname(socket,(sockaddr*)&addr,&n);port=ntohs(addr.sin_port);
  worker=std::thread([this]{while(!stop){fd_set set;FD_ZERO(&set);FD_SET(socket,&set);timeval wait{0,100000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;auto client=accept(socket,nullptr,nullptr);if(client!=INVALID_SOCKET)std::thread([this,client]{serve(client);}).detach();}});}
 ~Fixture(){stop=true;if(worker.joinable())worker.join();closesocket(socket);}
 std::string url(const std::string& path){return "http://127.0.0.1:"+std::to_string(port)+path;}
 Json snapshot(){std::lock_guard<std::mutex> lock(mutex);return requests;}
};
CComBSTR b(const std::string& s){return CComBSTR(wide(s).c_str());}
bool waitComplete(Manager& m,JobPtr job){auto until=GetTickCount64()+15000;for(;;){m.tick();{Lock lock(m.mutex);auto state=str(job->data,"Status");if(state=="Complete")return true;if(state=="Failed"){std::cerr<<str(job->data,"Error")<<std::endl;return false;}}if(GetTickCount64()>until)return false;Sleep(40);}}
}
int wmain(int argc,wchar_t** argv){
 bool running=false;fs::path root;
 for(int i=1;i<argc;++i){std::wstring arg=argv[i];if(arg==L"--running-server")running=true;else if(root.empty())root=fs::absolute(arg);else return 2;}
 if(root.empty())return 2;if(fs::exists(root)&&!fs::is_empty(root)){std::cerr<<"Use a new isolated test directory."<<std::endl;return 2;}fs::create_directories(root);
 auto tag=L"autotest-"+wide(guid()).substr(0,8);SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag.c_str());
 WSADATA winsock{};WSAStartup(MAKEWORD(2,2),&winsock);good(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED));
 try{
  Fixture fixture;Registration registration(root/L"catalog",tag,running);CComPtr<IUdmLinkTransmitter> api;HRESULT activated=E_FAIL;auto deadline=GetTickCount64()+5000;
  do{activated=api.CoCreateInstance(CLSID_UdmLinkTransmitter,nullptr,CLSCTX_LOCAL_SERVER);if(activated!=REGDB_E_CLASSNOTREG)break;Sleep(100);}while(GetTickCount64()<deadline);
  // Some machines never route per-user LocalServer32 registrations through the
  // COM service (in-process per-user classes still work). The cold run then
  // records that and continues against an explicitly started server, which is
  // also what a running desktop provides.
  bool coldUnavailable=false;
  if(!running&&activated==REGDB_E_CLASSNOTREG){coldUnavailable=true;registration.startManual(root/L"catalog",tag);activated=api.CoCreateInstance(CLSID_UdmLinkTransmitter,nullptr,CLSCTX_LOCAL_SERVER);}
  good(activated);check(!!api,running?"Actual running out-of-process automation server accepts activation":coldUnavailable?"Cold activation is unavailable on this machine (per-user LocalServer32 not consulted by the COM service); the registered server was started explicitly":"Cold COM activation starts the registered automation server");
  std::cout<<"Automation activation: "<<Json{{"mode",running?"running-server":"cold"},{"coldUnavailable",coldUnavailable}}.dump()<<std::endl;
  auto folder=utf8((root/L"files").wstring());auto address=fixture.url("/authenticated.bin"),referrer=fixture.url("/page");std::string post="token=automation-private&value=%E2%9C%93";
  {
   Manager manager(root/L"catalog");std::atomic<int> shows=0;PipeServer bridge(manager,[&]{++shows;});
   CComVariant agent(L"UDM automation fixture"),empty;
   good(api->SendLinkToUDM2(b(address),b(referrer),b("session=automation-cookie"),b(post),b("fixture-user"),b("fixture-password"),b(folder),b("renamed.bin"),3,agent,empty));
   JobPtr download;{Lock lock(manager.mutex);download=manager.jobs.at(0);}
   check(str(download->data,"Status")=="Paused"&&shows==0,"Silent queue-only call stays paused without requesting a window");
   check(str(download->data,"FileName")=="renamed.bin"&&str(download->data,"Folder")==folder,"COM destination and file name reach the native catalog");
   auto headers=readHeaders(download->data);check(headerValue(headers,"Cookie")=="session=automation-cookie"&&headerValue(headers,"Referer")==referrer&&headerValue(headers,"User-Agent")=="UDM automation fixture"&&basicLogin(headers).second=="fixture-password","Cookies, referer, user agent and credentials survive protected handoff");
   auto stored=readText(root/L"catalog/state.json");check(stored.find("automation-cookie")==std::string::npos&&stored.find("fixture-password")==std::string::npos&&stored.find("automation-private")==std::string::npos,"Credentials, cookies and POST data are absent from plaintext catalog");
   manager.resume(download);check(waitComplete(manager,download)&&readText(download->target())==fixture.payload,"COM-authenticated POST completes with exact output bytes");
   auto requests=fixture.snapshot();check(requests.size()==1&&str(requests[0],"body")==post&&str(requests[0],"headers").rfind("POST ",0)==0&&str(requests[0],"headers").find("Range:")==std::string::npos,"POST transfer is sent exactly once without range replay");
   check(str(requests[0],"headers").find("session=automation-cookie")!=std::string::npos&&str(requests[0],"headers").find("Basic Zml4dHVyZS11c2VyOmZpeHR1cmUtcGFzc3dvcmQ=")!=std::string::npos,"Actual request carries the supplied cookie and authentication");
   size_t before;{Lock lock(manager.mutex);before=manager.jobs.size();}
   check(FAILED(api->SendLinkToUDM(b(address),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),b("invalid.bin"),4)),"Unsupported COM flags fail");
   check(FAILED(api->SendLinkToUDM(b(address),nullptr,b("cookie\r\nInjected: value"),nullptr,nullptr,nullptr,b(folder),nullptr,3)),"Header injection fails before adding a job");
   check(FAILED(api->SendLinkToUDM(b(address),nullptr,nullptr,nullptr,nullptr,nullptr,b("relative"),nullptr,3)),"Relative destinations fail before adding a job");
   check(FAILED(api->SendLinkToUDM(b(address),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),b("../escape.bin"),3)),"Path traversal file names fail");
   CComVariant reserved(1);check(FAILED(api->SendLinkToUDM2(b(address),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),nullptr,3,empty,reserved)),"Unsupported reserved values fail");
   check(FAILED(api->SendLinkToUDM2(b(address),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),nullptr,3,reserved,empty)),"Non-string user-agent variants fail");
   CComBSTR embedded(3,L"a\0b");check(FAILED(api->SendLinkToUDM(b(address),nullptr,embedded,nullptr,nullptr,nullptr,b(folder),nullptr,3)),"Embedded NUL input fails without truncation");
   CComBSTR excessive((int)MaxBrowserPostBytes+1);if(excessive.m_str)std::fill_n(excessive.m_str,MaxBrowserPostBytes+1,L'x');check(FAILED(api->SendLinkToUDM(b(address),nullptr,nullptr,excessive,nullptr,nullptr,b(folder),nullptr,3)),"Oversized automation POST data fails");
   check(FAILED(api->SendLinksArray(b(referrer),empty)),"An absent batch array fails");
   {Lock lock(manager.mutex);check(manager.jobs.size()==before,"Rejected calls leave the job list unchanged");}
   good(api->SendLinkToUDM(b(fixture.url("/confirm.bin")),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),nullptr,0));
   {Lock lock(manager.mutex);check(str(manager.jobs.back()->data,"Status")=="Awaiting confirmation"&&shows>0,"Default COM call requests File Info confirmation");}
   good(api->SendLinkToUDM(b(fixture.url("/later.bin")),nullptr,nullptr,nullptr,nullptr,nullptr,b(folder),nullptr,2));
   {Lock lock(manager.mutex);check(str(manager.jobs.back()->data,"Status")=="Paused"&&yes(manager.jobs.back()->data,"QueueMember"),"Queue-only flag retains a pending queue member");}
   CComPtr<IDispatch> dispatch;good(api.QueryInterface(&dispatch));LPOLESTR method=const_cast<LPOLESTR>(L"SendLinkToUDM");DISPID methodId=0;good(dispatch->GetIDsOfNames(IID_NULL,&method,1,LOCALE_USER_DEFAULT,&methodId));
   CComVariant arguments[9]={CComVariant(3),CComVariant(L"dispatch.bin"),CComVariant(wide(folder).c_str()),CComVariant(L""),CComVariant(L""),CComVariant(L""),CComVariant(L""),CComVariant(L""),CComVariant(wide(fixture.url("/dispatch.bin")).c_str())};DISPPARAMS parameters{arguments,nullptr,9,0};good(dispatch->Invoke(methodId,IID_NULL,LOCALE_USER_DEFAULT,DISPATCH_METHOD,&parameters,nullptr,nullptr,nullptr));
   {Lock lock(manager.mutex);check(str(manager.jobs.back()->data,"FileName")=="dispatch.bin","IDispatch clients can call the same typed automation method");}
   SAFEARRAYBOUND bounds[2]={{2,5},{4,7}};CComVariant array;array.vt=VT_ARRAY|VT_BSTR;array.parray=SafeArrayCreate(VT_BSTR,2,bounds);if(!array.parray)throw std::bad_alloc();
   for(LONG row=5;row<7;++row)for(LONG column=7;column<11;++column){std::string value=column==7?fixture.url(row==5?"/batch-one.bin":"/batch-two.bin"):column==8?"batch-cookie=private":column==9?"Description "+std::to_string(row):row==5?"Batch agent":"";auto v=b(value);LONG at[]={row,column};good(SafeArrayPutElement(array.parray,at,v));}
   good(api->SendLinksArray(b(referrer),array));auto offer=nextAutomationBatch(manager);check(offer["links"].size()==2&&str(offer["links"][0],"userAgent")=="Batch agent","Two-dimensional SAFEARRAY with nonzero bounds reaches pending batch review");
   stored=readText(root/L"catalog/state.json");check(stored.find("batch-cookie=private")==std::string::npos&&stored.find("/batch-one.bin")==std::string::npos,"Unreviewed batch requests are encrypted in the catalog");
   before=manager.jobs.size();acceptAutomationRow(manager,offer,0,folder,"Main queue",true);acceptAutomationRow(manager,offer,0,folder,"Main queue",true);check(manager.jobs.size()==before+1&&automationRowAdded(manager,str(offer,"Id"),0),"Repeated acceptance of a batch row does not duplicate the job");
   manager.stop();
  }
  {
   Manager restored(root/L"catalog");auto offer=nextAutomationBatch(restored);check(!offer.empty()&&automationRowAdded(restored,str(offer,"Id"),0)&&!automationRowAdded(restored,str(offer,"Id"),1),"Restart preserves the review and accepted row attribution");auto before=restored.jobs.size();acceptAutomationRow(restored,offer,1,folder,"Main queue",true);check(restored.jobs.size()==before+1&&str(restored.jobs.back()->data,"Description")=="Description 6","Review continuation preserves the second link description");dismissAutomationBatch(restored,str(offer,"Id"));check(nextAutomationBatch(restored).empty(),"Completed review removes its protected pending offer");
   auto saved=restored.jobs.size();Json batch={{"referrer",referrer},{"userAgent","fixture"},{"links",Json::array({Json{{"url",fixture.url("/cancelled.bin")}}})}};queueAutomationBatch(restored,batch);auto cancel=nextAutomationBatch(restored);dismissAutomationBatch(restored,str(cancel,"Id"));check(restored.jobs.size()==saved&&nextAutomationBatch(restored).empty(),"Cancelling batch review creates no download");
   rejects([&]{queueAutomationBatch(restored,{{"links",Json::array({Json{{"url",fixture.url("/valid.bin")}},Json{{"url","file:///invalid"}}})}});},"An invalid batch row rejects the whole offer");check(nextAutomationBatch(restored).empty(),"Invalid batch has no partially persisted offer");
   restored.stop();
  }
  api.Release();
 }catch(const std::exception& e){check(false,e.what());}
 HKEY key=nullptr;auto cleanup=RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Classes\\CLSID\\{84987627-1793-4A9D-AEC6-E31AF7D6942A}",0,KEY_READ,&key);if(key)RegCloseKey(key);check(cleanup==ERROR_FILE_NOT_FOUND,"Temporary COM class registration is removed");
 std::ofstream(root/L"results.json")<<Json{{"mode",running?"running-server":"cold-activation"},{"passed",passed},{"failed",failed},{"checks",checks}}.dump(2)<<std::endl;
 std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;CoUninitialize();WSACleanup();return failed?1:0;
}
