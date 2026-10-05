#include "Core.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <regex>
using namespace udm;

namespace {
class ProbeServer {
 SOCKET listener=INVALID_SOCKET;
 std::thread worker;
 std::atomic_bool stopping{false};
 static void sendAll(SOCKET socket,const std::string& bytes){
  size_t offset=0;
  while(offset<bytes.size()){
   auto sent=send(socket,bytes.data()+offset,(int)(bytes.size()-offset),0);
   if(sent<=0)return;
   offset+=sent;
  }
 }
 void serve(SOCKET socket){
  DWORD timeout=2000;
  setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
  std::string request;char buffer[4096];
  while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){
   int count=recv(socket,buffer,sizeof(buffer),0);if(count<=0)return;request.append(buffer,count);
  }
  ++requests;
  const int behavior=mode.load();
  std::string headers="ETag: \"stable\"\r\nConnection: close\r\n",body="A",range="bytes 0-0/8192";
  if(behavior==1)body.clear();
  if(behavior==2)range="bytes 1-1/8192";
  if(behavior==3)range.clear();
  if(behavior==4)headers+="Content-Encoding: gzip\r\n";
  if(behavior==5)body="AA";
  if(behavior==6||behavior==12){
   sendAll(socket,"HTTP/1.1 206 Partial Content\r\n"+headers+"Content-Range: "+range+"\r\nTransfer-Encoding: chunked\r\n\r\n"+(behavior==12?"1\r\nA\r\n":"")+"0\r\n\r\n");return;
  }
  if(behavior==7||behavior==8){
   sendAll(socket,"HTTP/1.1 416 Range Not Satisfiable\r\n"+headers+"Content-Range: bytes */"+(behavior==7?"0":"8192")+"\r\nContent-Length: 0\r\n\r\n");return;
  }
  if(behavior==9)range="bytes 0-0/999999999999999999999999999999";
  if(behavior==10){
   sendAll(socket,"HTTP/1.1 200 OK\r\n"+headers+"Content-Length: 8192\r\n\r\n"+std::string(8192,'A'));return;
  }
  if(behavior==11)headers+="Content-Encoding: identity\r\n";
  if(behavior==13){
   sendAll(socket,"HTTP/1.1 206 Partial Content\r\n"+headers+"Content-Range: "+range+"\r\nContent-Length: 1\r\n\r\n");
   bodyPending=true;const auto deadline=GetTickCount64()+5000;
   while(!stopping&&GetTickCount64()<deadline){
    fd_set readable;FD_ZERO(&readable);FD_SET(socket,&readable);timeval delay{0,50000};
    if(select(0,&readable,nullptr,nullptr,&delay)>0)break;
   }
   bodyPending=false;return;
  }
  if(behavior==14)range="bytes 0-0/0";
  if(behavior==0){
   std::smatch match;
   if(!std::regex_search(request,match,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase)))throw std::runtime_error("Expected a range request");
   auto first=std::stoull(match[1]),last=match[2].length()?std::stoull(match[2]):8191;
   if(first>last||last>=8192)throw std::runtime_error("Unexpected range");
   body=std::string((size_t)(last-first+1),'A');
   range="bytes "+std::to_string(first)+"-"+std::to_string(last)+"/8192";
  }
  if(!range.empty())headers+="Content-Range: "+range+"\r\n";
  sendAll(socket,"HTTP/1.1 206 Partial Content\r\n"+headers+"Content-Length: "+std::to_string(behavior==1?1:body.size())+"\r\n\r\n"+body);
 }
public:
 int port=0;std::atomic_int mode{0},requests{0};std::atomic_bool bodyPending{false};
 ProbeServer(){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(listener==INVALID_SOCKET||bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("Cannot start local probe fixture");
  int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  worker=std::thread([this]{
   while(!stopping){
    fd_set readable;FD_ZERO(&readable);FD_SET(listener,&readable);timeval delay{0,100000};
    if(select(0,&readable,nullptr,nullptr,&delay)<=0)continue;
    auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)continue;
    try{serve(client);}catch(...){}
    shutdown(client,SD_BOTH);closesocket(client);
   }
  });
 }
 ~ProbeServer(){stopping=true;if(worker.joinable())worker.join();closesocket(listener);}
 std::string url()const{return "http://127.0.0.1:"+std::to_string(port)+"/saved.bin";}
};
}

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;
 auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Json checks=Json::array();std::string error;
 auto check=[&](bool passed,const std::string& name){checks.push_back({{"name",name},{"passed",passed}});};
 try{
  ProbeServer server;
  struct Scenario{int mode;const char* name;bool accepted;};
  for(const auto& scenario:std::vector<Scenario>{{1,"truncated range",false},{2,"wrong range",false},{3,"missing range",false},{4,"encoded range",false},{5,"oversized body",false},{6,"empty chunked body",false},{7,"empty resource",true},{8,"nonempty unsatisfied range",false},{9,"overflow size",false},{10,"range ignored",true},{11,"identity range",true},{12,"chunked range",true},{13,"cancel pending body",false},{14,"impossible range size",false}}){
   auto folder=root/wide(std::to_string(scenario.mode));Manager manager(folder/L"data");
   auto prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["Retries"]=0;manager.setSettings(prefs);
   auto queue=defaultQueue("Probe sync");queue["Enabled"]=false;queue["Synchronize"]=true;queue["FileRetryLimitEnabled"]=true;queue["FileRetries"]=0;queue["FinishAction"]="Exit UDM";manager.setQueue(queue);
   server.mode=scenario.mode==7?7:0;
   auto job=manager.add(server.url(),"","saved.bin","Probe sync");
   auto pump=[&](auto ready){
    auto deadline=GetTickCount64()+10000;
    while(GetTickCount64()<deadline){manager.tick();{Lock lock(manager.mutex);if(ready())return;}Sleep(10);}
    throw std::runtime_error(std::string("Timed out: ")+scenario.name);
   };
   manager.queueRun("Probe sync",true);
   pump([&]{return str(job->data,"Status")=="Complete"&&!manager.isActive(job);});
   manager.tick();manager.takeQueueCompletions();
   auto originalHash=fileHash(job->target());auto originalSize=fs::file_size(job->target());
   server.mode=scenario.mode;int before=server.requests;
   manager.queueRun("Probe sync",true);
   if(scenario.mode==13){
    pump([&]{return server.bodyPending.load()&&manager.isActive(job);});
    auto started=GetTickCount64();manager.queueRun("Probe sync",false);
    pump([&]{return !manager.isActive(job);});
    check(GetTickCount64()-started<2000,"Pending probe body is promptly cancellable");
    check(str(job->data,"SyncStatus")=="Check canceled"&&!yes(job->data,"SyncRetryFailed"),"Canceled probe is not recorded as a retry failure");
    check(manager.jobs.size()==1&&fileHash(job->target())==originalHash,"Canceled probe preserves saved file and catalog membership");
    check(manager.takeQueueCompletions().empty(),"Canceled probe emits no completion action");
    manager.stop();
    Manager reopened(folder/L"data");
    check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data,"SyncStatus")=="Check canceled"&&fileHash(reopened.jobs[0]->target())==originalHash,"Canceled probe result and file survive restart");
    continue;
   }
   pump([&]{for(auto item:manager.jobs)if(manager.isActive(item))return false;return !yes(job->data,"SyncPending")&&!str(job->data,"SyncStatus").empty();});
   manager.tick();
   const auto prefix=std::string(scenario.name)+": ";
   check((str(job->data,"SyncStatus")=="Up to date")==scenario.accepted,prefix+"truthful synchronization result");
   check(yes(job->data,"SyncRetryFailed")!=scenario.accepted,prefix+"correct retry outcome");
   check(manager.jobs.size()==1&&server.requests-before==1,prefix+"one probe without an unvalidated replacement");
   check(fileHash(job->target())==originalHash&&fs::file_size(job->target())==originalSize,prefix+"saved file remains byte-identical");
   auto events=manager.takeQueueCompletions();
   check(events.size()==1&&num(events[0],"FailedFiles")== (scenario.accepted?0:1),prefix+"completion event discloses failures");
   manager.stop();
   Manager reopened(folder/L"data");
   check(reopened.jobs.size()==1&&str(reopened.jobs[0]->data,"SyncStatus")==str(job->data,"SyncStatus")&&fileHash(reopened.jobs[0]->target())==originalHash,prefix+"result and file survive restart");
  }
 }catch(const std::exception& e){error=e.what();}
 bool passed=error.empty();for(const auto& check:checks)passed=passed&&yes(check,"passed");
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks},{"systemActionsExecuted",false}}.dump(2),false);
 CoUninitialize();WSACleanup();return passed?0:1;
}
