#include "DashResourceTransport.hpp"
#include "DashMetadataTransport.hpp"
#include <future>
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;auto root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  const std::string url="http://127.0.0.1:19001/file";const Json prefs={{"ProxyMode","Connect directly"}};const Headers headers={{"Authorization","Bearer owned"},{"Cookie","owned=explicit"}};
  check(!dashResourceTransportContext()&&!dashResourceTransport(url,prefs,headers),"Unowned requests retain ordinary transport lifetime");std::weak_ptr<HttpSession> normal,nested,exceptional;
  {DashResourceTransportScope scope;auto a=dashResourceTransport(url,prefs,headers);normal=a;
   check(a==dashResourceTransport("http://127.0.0.1:19001/other",prefs,headers),"Independent resources reuse the same origin and complete identity");
   check(yes(a->settings(),"__DashBoundedRoutes"),"Resource sessions enable bounded resolved-route retention");
   check(a!=dashResourceTransport("http://127.0.0.1:19002/file",prefs,headers),"Different ports cannot share resource authentication state");
   check(a!=dashResourceTransport("https://127.0.0.1:19001/file",prefs,headers),"Different schemes remain isolated");
   auto changed=headers;changed["Authorization"]="Bearer rotated";check(a!=dashResourceTransport(url,prefs,changed),"Authorization rotation creates a separate pool");
   changed=headers;changed["Cookie"]="owned=rotated";check(a!=dashResourceTransport(url,prefs,changed),"Cookie rotation creates a separate pool");
   changed=headers;changed["User-Agent"]="Owned other agent";check(a!=dashResourceTransport(url,prefs,changed),"Request identity headers participate in the transport key");
   auto preferences=prefs;preferences["UserAgent"]="Other default agent";check(a!=dashResourceTransport(url,preferences,headers),"Complete transport preferences remain isolated");
   {DashMetadataTransportScope metadata;check(a!=dashMetadataTransport(url,prefs),"Body transports never borrow metadata transports");}
   const auto owner=dashResourceTransportContext();auto other=std::async(std::launch::async,[&]{if(dashResourceTransportContext()||dashResourceTransport(url,prefs,headers))return false;{DashResourceTransportBinding binding(owner);if(dashResourceTransport(url,prefs,headers)!=a)return false;}return !dashResourceTransportContext();});check(other.get(),"A joined worker borrows only an explicitly bound owner and restores its context");
   std::vector<std::future<bool>> concurrent;for(int n=0;n<8;++n)concurrent.push_back(std::async(std::launch::async,[&]{DashResourceTransportBinding binding(owner);return dashResourceTransport(url,prefs,headers)==a;}));bool same=true;for(auto& task:concurrent)same&=task.get();check(same,"Concurrent workers reuse one session without duplicate cache insertion");
   {DashResourceTransportScope inner;auto b=dashResourceTransport(url,prefs,headers);nested=b;check(a!=b,"Nested recordings have separate transport ownership");{DashResourceTransportBinding binding(owner);check(dashResourceTransport(url,prefs,headers)==a,"An explicit binding selects its captured recording inside a nested scope");}check(dashResourceTransport(url,prefs,headers)==b,"Binding teardown restores the nested recording");}
   check(nested.expired()&&dashResourceTransport(url,prefs,headers)==a,"Nested teardown releases its pools and restores the parent recording");
   auto huge=prefs;huge["OwnedLargeKey"]=std::string(1024*1024,'x');check(!dashResourceTransport(url,huge,headers),"Oversized complete identities use ordinary request ownership");
  }
  check(normal.expired()&&!dashResourceTransportContext(),"Normal exit releases all pools after joined workers and request owners end");
  try{DashResourceTransportScope scope;exceptional=dashResourceTransport(url,prefs,headers);throw std::runtime_error("owned interruption");}catch(const std::runtime_error&){}
  check(exceptional.expired()&&!dashResourceTransportContext(),"Exceptional exit releases pools and clears the active owner");
  {DashResourceTransportScope scope;std::vector<std::shared_ptr<HttpSession>> held;for(int n=0;n<16;++n)held.push_back(dashResourceTransport("http://127.0.0.1:"+std::to_string(19100+n)+"/",prefs,headers));
   check(std::all_of(held.begin(),held.end(),[](const auto& p){return bool(p);}),"Sixteen complete identities may be retained");
   check(!dashResourceTransport("http://127.0.0.1:19116/",prefs,headers),"Capacity overflow falls back without retaining a seventeenth session");
   check(held[0]==dashResourceTransport("http://127.0.0.1:19100/again",prefs,headers),"Capacity fallback never evicts an in-use recording transport");
  }
  std::shared_ptr<HttpSession> request;std::weak_ptr<HttpSession> remaining;
  {DashResourceTransportScope scope;request=dashResourceTransport(url,prefs,headers);remaining=request;}
  check(!remaining.expired(),"A request's shared ownership survives cache-scope teardown");request.reset();check(remaining.expired(),"The final request release closes its remaining transport");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";return error.empty()?0:1;
}
