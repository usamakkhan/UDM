#include "Core.hpp"
#include "CurlHttp.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=4||fs::exists(argv[1]))return 2;const auto root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array(),requests=Json::array();std::string error;
 WSADATA sockets{};WSAStartup(MAKEWORD(2,2),&sockets);CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  for(int backend=0;backend<2;++backend){
   const auto base=utf8(argv[backend+2]);const Json prefs={{"ProxyMode","Connect directly"}};
   for(const std::string name:{"default","identity","lowercase","mixed-case","conflicting","post","head","range"}){
    Headers headers;if(name!="default")headers["Accept-Encoding"]="identity";
    if(name=="lowercase"){headers.clear();headers["accept-encoding"]="identity";}
    if(name=="mixed-case")headers["accept-encoding"]="identity";
    if(name=="conflicting")headers["accept-encoding"]="gzip";
    auto cancel=std::make_shared<Cancel>();cancel->deadline=GetTickCount64()+10000;
    const bool post=name=="post",head=name=="head",range=name=="range";const Bytes body={0,1,2,255};
    Http response(base+"/"+name,headers,prefs,*cancel,range?std::optional<i64>{0}:std::nullopt,range?std::optional<i64>{4}:std::nullopt,range?"\"owned\"":"",post?&body:nullptr,false,{},head);
    check(needsExplicitProxyTransport(Url(base),prefs)==bool(backend),"Live request URL selects the intended WinHTTP or curl route");
    check(response.status==(range?206u:200u),"Identity policy preserves the response status");
    if(!head){std::string text;char buffer[32];for(size_t bytes;(bytes=response.read(buffer,sizeof(buffer),*cancel))!=0;)text.append(buffer,bytes);check(text=="owned","The complete unencoded response bytes remain exact");}
    requests.push_back({{"backend",backend?"curl":"winhttp"},{"case",name}});
   }
   auto cancel=std::make_shared<Cancel>();bool rejected=false;
   try{Http response(base+"/invalid",{{"Accept-Encoding","identity\r\nX-Injected: bad"}},prefs,*cancel);}catch(const std::exception& e){rejected=std::string(e.what())=="Invalid HTTP header.";}
   check(rejected,"Reserved encoding normalization still rejects header injection");
  }
 }catch(const std::exception& e){error=e.what();}
 Json result={{"passed",error.empty()},{"error",error},{"checks",checks},{"requests",requests}};atomicText(root/L"results.json",result.dump(2),false);std::cout<<result.dump()<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;
}
