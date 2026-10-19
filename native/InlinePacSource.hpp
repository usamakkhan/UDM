#pragma once
#include "Core.hpp"
namespace udm {
// WinHTTP's asynchronous, out-of-process PAC evaluator accepts HTTP(S) URLs.
// Supply inline bytes through a temporary loopback endpoint; never write scripts to disk.
class InlinePacSource {
 SOCKET listener=INVALID_SOCKET;std::thread worker;std::atomic_bool stopping{false};
 std::string path,script,address;
 bool ready(SOCKET socket,bool writing,ULONGLONG deadline){
  while(!stopping&&GetTickCount64()<deadline){fd_set set;FD_ZERO(&set);FD_SET(socket,&set);timeval timeout{0,25000};
   int n=select(0,writing?nullptr:&set,writing?&set:nullptr,nullptr,&timeout);if(n>0)return true;if(n<0)return false;
  }return false;
 }
 void serve(SOCKET socket){
  struct Close{SOCKET value;~Close(){closesocket(value);}} close{socket};
  u_long nonblocking=1;if(ioctlsocket(socket,FIONBIO,&nonblocking))return;
  const auto deadline=GetTickCount64()+3000;std::string request;char buffer[2048];
  while(request.find("\r\n\r\n")==std::string::npos&&request.size()<8192){
   if(!ready(socket,false,deadline))return;int n=recv(socket,buffer,sizeof(buffer),0);
   if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(n<=0)return;request.append(buffer,n);
  }
  if(request.size()>8192||request.find("\r\n\r\n")==std::string::npos)return;
  const bool match=request.rfind("GET "+path+" HTTP/1.1\r\n",0)==0||request.rfind("GET "+path+" HTTP/1.0\r\n",0)==0;
  std::string response=match?"HTTP/1.1 200 OK\r\nContent-Type: application/x-ns-proxy-autoconfig\r\nCache-Control: no-store\r\nContent-Length: "+std::to_string(script.size())+"\r\nConnection: close\r\n\r\n"+script:"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
  for(size_t offset=0;offset<response.size();){
   if(!ready(socket,true,deadline))return;int n=::send(socket,response.data()+offset,(int)(response.size()-offset),0);
   if(n==SOCKET_ERROR&&WSAGetLastError()==WSAEWOULDBLOCK)continue;if(n<=0)return;offset+=n;
  }
 }
public:
 explicit InlinePacSource(std::string text):path("/"+guid()+".pac"),script(std::move(text)){
  if(script.empty()||script.size()>65536||script.find('\0')!=std::string::npos)throw std::runtime_error("Invalid inline proxy script.");
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(listener==INVALID_SOCKET)throw std::runtime_error("Cannot initialize inline proxy script source.");
  try{
   BOOL exclusive=TRUE;u_long nonblocking=1;sockaddr_in local{};local.sin_family=AF_INET;local.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
   if(setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&exclusive,sizeof(exclusive))||bind(listener,(sockaddr*)&local,sizeof(local))||listen(listener,4)||ioctlsocket(listener,FIONBIO,&nonblocking))throw std::runtime_error("Cannot bind inline proxy script source.");
   int length=sizeof(local);if(getsockname(listener,(sockaddr*)&local,&length))throw std::runtime_error("Cannot read inline proxy script endpoint.");
   address="http://127.0.0.1:"+std::to_string(ntohs(local.sin_port))+path;
   worker=std::thread([this]{try{while(!stopping){if(!ready(listener,false,GetTickCount64()+100))continue;auto client=accept(listener,nullptr,nullptr);if(client!=INVALID_SOCKET)serve(client);}}catch(...){}});
  }catch(...){closesocket(listener);listener=INVALID_SOCKET;throw;}
 }
 InlinePacSource(const InlinePacSource&)=delete;InlinePacSource& operator=(const InlinePacSource&)=delete;
 ~InlinePacSource(){stopping=true;if(worker.joinable())worker.join();if(listener!=INVALID_SOCKET)closesocket(listener);}
 const std::string& url()const{return address;}
};
}
