#include "Core.hpp"
#include "HttpRouteRetention.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;auto root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  std::map<std::string,std::shared_ptr<int>> routes;int created=0;auto make=[&]{return std::make_shared<int>(++created);};
  auto first=retainHttpRoute(routes,"first",true,make);check(first==retainHttpRoute(routes,"first",true,make)&&created==1,"A known resolved route reuses its original owner");
  for(int n=1;n<16;++n)retainHttpRoute(routes,std::to_string(n),true,make);check(routes.size()==16&&created==16,"Bounded owners retain at most sixteen resolved routes");
  std::weak_ptr<int> overflow;{auto temporary=retainHttpRoute(routes,"overflow",true,make);overflow=temporary;check(routes.size()==16&&created==17,"New routes at capacity remain request-owned");}
  check(overflow.expired(),"The uncached resolved route is released with its request");
  check(first==retainHttpRoute(routes,"first",true,make)&&created==17,"Capacity fallback preserves cached owners without eviction");
  std::map<std::string,std::shared_ptr<int>> large;auto oversized=retainHttpRoute(large,std::string(1024*1024+1,'x'),true,make);check(large.empty()&&oversized,"Oversized route identities are usable without retention");
  auto exact=retainHttpRoute(large,std::string(1024*1024,'x'),true,make);check(large.size()==1&&exact,"An exactly bounded route key is retained");
  std::map<std::string,std::shared_ptr<int>> failures;bool rejected=false;try{retainHttpRoute(failures,"failed",true,[]()->std::shared_ptr<int>{throw std::runtime_error("owned factory failure");});}catch(const std::exception&){rejected=true;}
  check(rejected&&failures.empty(),"A construction failure cannot consume a cache slot or publish a null route");
  check(bool(retainHttpRoute(failures,"failed",true,make))&&failures.size()==1,"A failed route can be constructed on a subsequent request");
  std::map<std::string,std::shared_ptr<int>> ordinary;for(int n=0;n<20;++n)retainHttpRoute(ordinary,std::to_string(n),false,make);check(ordinary.size()==20,"Ordinary session retention remains unchanged outside resource owners");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";return error.empty()?0:1;
}
