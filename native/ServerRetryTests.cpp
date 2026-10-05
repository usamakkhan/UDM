#include "Core.hpp"
#include <ws2tcpip.h>
#include <iostream>
using namespace udm;

namespace {
class RetryServer {
 SOCKET listener=INVALID_SOCKET;std::thread worker;std::atomic_bool stopping{false};
 std::string retryAfter;
 static void sendAll(SOCKET socket,const std::string& bytes){
  size_t at=0;while(at<bytes.size()){int sent=send(socket,bytes.data()+at,(int)(bytes.size()-at),0);if(sent<=0)return;at+=sent;}
 }
 void serve(SOCKET socket){
  DWORD timeout=2000;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
  std::string request;char bytes[4096];
  while(request.find("\r\n\r\n")==std::string::npos&&request.size()<32768){int n=recv(socket,bytes,sizeof(bytes),0);if(n<=0)return;request.append(bytes,n);}
  ++requests;
  if(reject.exchange(false)){
   rejectedAt=epoch();
   sendAll(socket,"HTTP/1.1 503 Service Unavailable\r\nRetry-After: "+retryAfter+"\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
  }else{
   if(rejectedAt.load()&&!recoveredAt.load())recoveredAt=epoch();
   auto body=payload();sendAll(socket,"HTTP/1.1 200 OK\r\nETag: \"stable\"\r\nContent-Type: application/octet-stream\r\nContent-Length: "+std::to_string(body.size())+"\r\nConnection: close\r\n\r\n"+body);
  }
 }
public:
 int port=0;std::atomic_bool reject{false};std::atomic_int requests{0};std::atomic<i64> rejectedAt{0},recoveredAt{0};
 static std::string payload(){return std::string(8192,'R');}
 explicit RetryServer(std::string header):retryAfter(std::move(header)){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(listener==INVALID_SOCKET||bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,16))throw std::runtime_error("Cannot create local retry fixture");
  int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  worker=std::thread([this]{while(!stopping){fd_set readable;FD_ZERO(&readable);FD_SET(listener,&readable);timeval delay{0,100000};if(select(0,&readable,nullptr,nullptr,&delay)<=0)continue;auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)continue;try{serve(client);}catch(...){}shutdown(client,SD_BOTH);closesocket(client);}});
 }
 ~RetryServer(){stopping=true;if(worker.joinable())worker.join();closesocket(listener);}
 std::string url()const{return "http://127.0.0.1:"+std::to_string(port)+"/file.bin";}
};
}

int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 Json checks=Json::array(),timings=Json::array();std::string error;
 auto check=[&](bool passed,const std::string& name){checks.push_back({{"name",name},{"passed",passed}});};
 try{
  for(bool synchronize:{false,true})for(const auto& scenario:std::vector<std::string>{"delay","restart","long","stop","malformed"}){
   const auto label=std::string(synchronize?"sync ":"download ")+scenario;
   auto folder=root/wide(label);RetryServer server(scenario=="long"?"600":scenario=="malformed"?"invalid":"3");
   auto manager=std::make_unique<Manager>(folder/L"data");
   auto prefs=manager->state["Settings"];prefs["DownloadFolder"]=utf8((folder/L"files").wstring());prefs["CategoryFolders"]=false;prefs["Connections"]=1;prefs["Parallel"]=1;prefs["Retries"]=0;manager->setSettings(prefs);
   auto queue=defaultQueue("Server delay");queue["Enabled"]=false;queue["Synchronize"]=synchronize;queue["Scheduled"]=true;queue["StartMinute"]=0;queue["StopMinute"]=1440;queue["Days"]=127;queue["FileRetryLimitEnabled"]=scenario!="long";queue["FileRetries"]=1;queue["RetryDelaySeconds"]=1;queue["Retries"]=0;queue["FinishAction"]="Exit UDM";manager->setQueue(queue);
   auto job=manager->add(server.url(),"","file.bin","Server delay");
   auto pump=[&](auto ready){auto until=GetTickCount64()+12000;while(GetTickCount64()<until){manager->tick();{Lock lock(manager->mutex);if(ready())return;}Sleep(10);}throw std::runtime_error("Timed out: "+label);};
   if(synchronize){manager->queueRun("Server delay",true);pump([&]{return str(job->data,"Status")=="Complete"&&!manager->isActive(job);});manager->tick();manager->takeQueueCompletions();}
   server.reject=true;manager->queueRun("Server delay",true);
   pump([&]{return server.rejectedAt.load()!=0&&!manager->isActive(job);});
   manager->tick();
   const auto rejected=server.rejectedAt.load(),deadline=rejected+3000;
   const auto count=server.requests.load();
   auto waitUntil=[&](i64 until){while(epoch()<until){manager->tick();Sleep(10);}};
   if(scenario=="long"){
    check(yes(job->data,"ServerRetryRequiresReview"),label+": excessive wait requires an explicit retry");
    waitUntil(epoch()+1250);
    check(server.requests==count&&num(job->data,"QueueAttempts")==0,label+": unlimited queue does not bypass long server wait");
    check(manager->takeQueueCompletions().empty(),label+": blocked server wait emits no completion action");
    manager->stop();manager.reset();manager=std::make_unique<Manager>(folder/L"data");job=manager->jobs.at(0);
    check(yes(job->data,"ServerRetryRequiresReview"),label+": manual-review state survives restart");
    waitUntil(epoch()+150);
    check(server.requests==count,label+": scheduled startup cannot bypass manual review");
    manager->queueRun("Server delay",true);
   }else if(scenario=="stop"){
    manager->queueRun("Server delay",false);waitUntil(deadline+100);
    check(server.requests==count&&!manager->isActive(job),label+": Stop cancels deferred retry even after deadline");
    check(manager->takeQueueCompletions().empty(),label+": stopped retry emits no completion action");
    check(!synchronize||readText(job->target())==RetryServer::payload(),label+": Stop preserves saved file");
    manager->stop();continue;
   }else{
    if(scenario!="malformed"){
     check(parseDate(job->data.value("NotBefore",Json()))>=deadline-50,label+": queue deadline honors Retry-After");
     check(parseDate(job->data.value("ServerRetryAfterUtc",Json()))>=deadline-50,label+": server deadline is stored durably");
    }
    if(scenario=="restart"){
     auto stored=job->data.value("ServerRetryAfterUtc",Json());manager->stop();manager.reset();manager=std::make_unique<Manager>(folder/L"data");job=manager->jobs.at(0);
     check(!stored.is_null()&&job->data.value("ServerRetryAfterUtc",Json())==stored,label+": restart retains server deadline");
     // A new explicit queue cycle must not erase a still-pending short deadline.
     manager->queueRun("Server delay",true);
    }
    if(scenario!="malformed"){
     waitUntil(deadline-100);
     check(server.requests==count,label+": no request before server deadline");
     check(!synchronize||readText(job->target())==RetryServer::payload(),label+": waiting preserves saved bytes");
    }
   }
   pump([&]{return !manager->isActive(job)&&str(job->data,"Status")=="Complete"&&(!synchronize||str(job->data,"SyncStatus")=="Up to date");});
   manager->tick();
   auto elapsed=server.recoveredAt.load()-rejected;timings.push_back({{"case",label},{"retryElapsedMs",elapsed}});
   check(readText(job->target())==RetryServer::payload()&&manager->jobs.size()==1,label+": recovered result preserves exact bytes and identity");
   if(scenario=="delay"||scenario=="restart")check(elapsed>=2950,label+": actual request honors elapsed server wait");
   if(scenario=="malformed")check(elapsed>=950&&elapsed<5000,label+": malformed header falls back to queue delay");
   check(!yes(job->data,"ServerRetryRequiresReview")&&parseDate(job->data.value("ServerRetryAfterUtc",Json()))==0,label+": successful fresh attempt clears stale retry state");
   auto events=manager->takeQueueCompletions();check(events.size()==1&&num(events[0],"FailedFiles")==0,label+": recovery finishes queue exactly once");
   manager->stop();
  }
 }catch(const std::exception& e){error=e.what();}
 bool passed=error.empty();for(const auto& item:checks)passed=passed&&yes(item,"passed");
 atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks},{"timings",timings},{"clockAdvanced",false},{"systemActionsExecuted",false}}.dump(2),false);
 CoUninitialize();WSACleanup();return passed?0:1;
}
