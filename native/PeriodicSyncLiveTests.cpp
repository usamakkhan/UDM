#include "Core.hpp"
#include <ws2tcpip.h>
#include <iostream>
#include <regex>
using namespace udm;
class Server{
 SOCKET listener=INVALID_SOCKET;std::thread worker;std::atomic_bool stopping{false};
 static void sendAll(SOCKET s,const std::string& v){size_t n=0;while(n<v.size()){int k=send(s,v.data()+n,(int)std::min<size_t>(65536,v.size()-n),0);if(k<=0)return;n+=k;}}
 void serve(SOCKET s){DWORD timeout=3000;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));std::string req;char buf[4096];while(req.find("\r\n\r\n")==std::string::npos&&req.size()<32768){int n=recv(s,buf,sizeof(buf),0);if(n<=0)return;req.append(buf,n);}
 auto body=payload(version.load());std::smatch m;size_t first=0,last=body.size()-1;bool ranged=std::regex_search(req,m,std::regex("Range: bytes=([0-9]+)-([0-9]*)",std::regex::icase));if(ranged){first=std::stoull(m[1]);if(m[2].length())last=std::min(last,(size_t)std::stoull(m[2]));}
 if(first>last||last>=body.size()){sendAll(s,"HTTP/1.1 416 Range Not Satisfiable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");return;}
 bool head=req.rfind("HEAD ",0)==0;auto length=last-first+1;int v=body[0]=='A'?1:2;
 {std::lock_guard<std::mutex> l(mutex);requests.push_back({{"at",epoch()},{"version",v},{"first",first},{"last",last},{"head",head}});}
 std::string h=std::string("HTTP/1.1 ")+(ranged?"206 Partial Content":"200 OK")+"\r\nContent-Type: application/octet-stream\r\nAccept-Ranges: bytes\r\nETag: \"version-"+std::to_string(v)+"\"\r\nContent-Length: "+std::to_string(length)+"\r\nConnection: close\r\n";
 if(ranged)h+="Content-Range: bytes "+std::to_string(first)+"-"+std::to_string(last)+"/"+std::to_string(body.size())+"\r\n";
 sendAll(s,h+"\r\n");if(!head)sendAll(s,body.substr(first,length));}
public:
 int port=0;std::atomic_int version{1};std::mutex mutex;Json requests=Json::array();
 static std::string payload(int v){return std::string(262147,v==1?'A':'B');}
 Server(){listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);if(listener==INVALID_SOCKET||bind(listener,(sockaddr*)&a,sizeof(a))||listen(listener,16))throw std::runtime_error("Local server failed");int n=sizeof(a);getsockname(listener,(sockaddr*)&a,&n);port=ntohs(a.sin_port);worker=std::thread([this]{while(!stopping){fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval t{0,100000};if(select(0,&set,nullptr,nullptr,&t)>0){auto s=accept(listener,nullptr,nullptr);if(s!=INVALID_SOCKET){try{serve(s);}catch(...){}shutdown(s,SD_BOTH);closesocket(s);}}}});}
 ~Server(){stopping=true;if(worker.joinable())worker.join();closesocket(listener);}
 Json log(){std::lock_guard<std::mutex> l(mutex);return requests;}
};
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);Json checks=Json::array(),timings=Json::object();std::string failure;WSADATA w{};WSAStartup(MAKEWORD(2,2),&w);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto check=[&](bool b,const char* label){checks.push_back({{"name",label},{"passed",b}});if(!b)throw std::runtime_error(label);};
 try{Server server;Manager m(root/L"data");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((root/L"files").wstring());settings["CategoryFolders"]=false;settings["Connections"]=1;settings["Parallel"]=1;settings["Retries"]=0;settings["Proxy"]="";m.setSettings(settings);
 auto q=defaultQueue("Periodic fixture");q["Enabled"]=false;q["Synchronize"]=true;q["RepeatMinutes"]=1;m.setQueue(q);
 auto job=m.add("http://127.0.0.1:"+std::to_string(server.port)+"/payload.bin","","payload.bin","Periodic fixture");m.queueRun("Periodic fixture",true);
 auto queue=[&](){for(auto v:m.state["Queues"])if(str(v,"Name")=="Periodic fixture")return v;throw std::runtime_error("Queue missing");};
 auto pump=[&](auto predicate,int seconds,const char* phase){auto start=std::chrono::steady_clock::now();int last=-1;while(true){m.tick();{Lock l(m.mutex);if(predicate())return;}auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-start).count();if(elapsed>=seconds)throw std::runtime_error(std::string("Timed out: ")+phase);if((int)elapsed/10!=last){last=(int)elapsed/10;atomicText(root/L"progress.json",Json{{"phase",phase},{"elapsedSeconds",elapsed},{"checks",checks}}.dump(2),false);std::cout<<phase<<" "<<elapsed<<"s"<<std::endl;}Sleep(50);}};
 pump([&]{return str(job->data,"Status")=="Complete"&&!m.isActive(job)&&parseDate(queue().value("NextRunUtc",Json()))>epoch();},20,"initial download");
 {Lock l(m.mutex);check(readText(job->target())==Server::payload(1),"Initial HTTP download has exact version-one bytes");check(yes(job->data,"QueueMember"),"Completed synchronization file remains a queue member");}
 auto firstDone=std::chrono::steady_clock::now();server.version=2;JobPtr updated;
 pump([&]{for(auto j:m.jobs)if(j!=job&&str(j->data,"Status")=="Complete"&&str(j->data,"SyncStatus")=="Updated; previous version retained"&&!m.isActive(j)){updated=j;return parseDate(queue().value("NextRunUtc",Json()))>epoch();}return false;},80,"waiting for scheduled changed-file check");
 timings["changedCycleSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-firstDone).count();
 {Lock l(m.mutex);check(readText(updated->target())==Server::payload(2),"Scheduled synchronization publishes exact changed bytes");check(readText(job->target())==Server::payload(1),"Previous version bytes are retained");check(yes(updated->data,"QueueMember"),"Replacement remains enrolled for next synchronization");check(timings["changedCycleSeconds"].get<double>()>=59,"Repeat used real one-minute timing without advancing clock");}
 auto requestsBefore=server.log().size();auto secondDone=std::chrono::steady_clock::now();
 pump([&]{return str(updated->data,"SyncStatus")=="Up to date"&&!m.isActive(updated)&&parseDate(queue().value("NextRunUtc",Json()))>epoch();},80,"waiting for scheduled unchanged-file check");
 timings["unchangedCycleSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-secondDone).count();
 {Lock l(m.mutex);check(m.jobs.size()==2,"Unchanged synchronization does not create another version");check(readText(updated->target())==Server::payload(2),"Unchanged cycle preserves current bytes");check(timings["unchangedCycleSeconds"].get<double>()>=59,"Unchanged check also waited the real minute");}
 auto requests=server.log();check(requests.size()>requestsBefore,"Unchanged cycle contacts the server");bool onlyProbe=true;for(size_t i=requestsBefore;i<requests.size();++i)onlyProbe&=yes(requests[i],"head")||num(requests[i],"last")-num(requests[i],"first")+1<=1;check(onlyProbe,"Unchanged cycle transfers only metadata or one-byte probes");
 m.queueRun("Periodic fixture",false);m.stop();atomicText(root/L"requests.json",requests.dump(2),false);
 {Manager reopened(root/L"data");check(reopened.jobs.size()==2,"Both versions survive catalog reopen");check(readText(reopened.jobs.back()->target())==Server::payload(2),"Current synchronized file survives reopen");reopened.stop();}
 }catch(const std::exception&e){failure=e.what();}
 bool passed=failure.empty();atomicText(root/L"results.json",Json{{"passed",passed},{"error",failure},{"checks",checks},{"timings",timings},{"clockAdvanced",false}}.dump(2),false);CoUninitialize();WSACleanup();return passed?0:1;
}
