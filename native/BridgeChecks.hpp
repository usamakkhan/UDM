#pragma once
static void bridgeChecks(udm::Manager& manager){
 using namespace udm;
 auto tag=wide("tests-"+guid().substr(0,16));SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",tag.c_str());
 {
  PipeServer server(manager,[]{});
  check(yes(send({{"action","ping"}},3000),"ok"),"Native pipe ping round trip");
  auto diagnostic=send({{"action","diagnostics"}},3000);check(yes(diagnostic,"ok")&&str(diagnostic,"dataDirectory")==utf8(manager.root.wstring())&&num(diagnostic,"downloads")==static_cast<i64>(manager.jobs.size()),"Native diagnostics identify the active history without exposing download URLs");
  bool repeated=true;for(int i=0;i<25;++i)repeated&=yes(send({{"action","preferences"}},3000),"ok");
  check(repeated,"Repeated native pipe replies are retained");
  auto name=L"\\\\.\\pipe\\"+wide(pipeName());HANDLE raw=INVALID_HANDLE_VALUE;
  for(int i=0;i<100&&raw==INVALID_HANDLE_VALUE;++i){raw=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);if(raw==INVALID_HANDLE_VALUE)Sleep(10);}
  Handle client(raw);if(!client)throw std::runtime_error("Cannot open delayed pipe fixture.");
  std::string request="{\"action\":\"ping\"}";DWORD size=(DWORD)request.size(),written=0;
  if(!WriteFile(client.h,&size,4,&written,nullptr)||written!=4||!WriteFile(client.h,request.data(),size,&written,nullptr)||written!=size)throw std::runtime_error("Cannot write delayed pipe fixture.");
  Sleep(150);DWORD count=0;size=0;bool retained=ReadFile(client.h,&size,4,&count,nullptr)&&count==4&&size>0&&size<1024;
  if(retained){std::string reply(size,0);retained=ReadFile(client.h,reply.data(),size,&count,nullptr)&&count==size&&yes(Json::parse(reply),"ok");}
  check(retained,"Delayed host read keeps its acknowledgment");
 }
 auto start=GetTickCount64();
 {PipeServer server(manager,[]{});Sleep(30);}
 check(GetTickCount64()-start<1500,"Idle native pipe shutdown is cancellable");
 SetEnvironmentVariableW(L"UDM_INSTANCE_TAG",nullptr);
}
