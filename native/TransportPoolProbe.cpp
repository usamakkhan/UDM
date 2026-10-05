#include "Core.hpp"
#include "CurlHttp.hpp"
#include <iostream>
#include <regex>
#include <future>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=3)return 2;
 WSADATA ws{};if(WSAStartup(MAKEWORD(2,2),&ws))return 3;
 CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 try{
  const auto config=Json::parse(readText(fs::path(argv[1])));Json results={{"http2Available",CurlHttp::supportsHttp2()},{"cases",Json::array()}};
  std::map<std::string,std::shared_ptr<HttpSession>> pools;
  std::map<std::string,Json> poolSettings;
  for(const auto& group:config.value("groups",Json::array()))for(const auto& item:group.at("cases")){auto key=str(item,"session");if(!key.empty()){auto prefs=item.value("settings",defaultSettings());if(pools.count(key)&&poolSettings.at(key)!=prefs)throw std::runtime_error("Fixture session settings differ.");if(!pools.count(key)){pools[key]=std::make_shared<HttpSession>(prefs);poolSettings[key]=prefs;}}}
  auto runCase=[&](const Json& item){
   const auto id=str(item,"id");if(!std::regex_match(id,std::regex("[a-z0-9_-]{1,64}")))throw std::runtime_error("Invalid test identifier.");
   Json result={{"id",id}};auto started=GetTickCount64();
   try{
    auto prefs=item.value("settings",defaultSettings());Headers headers=item.value("headers",Headers{});Cancel cancel;
    if(num(item,"cancelAfterMs")>0)cancel.deadline=started+(ULONGLONG)num(item,"cancelAfterMs");
    auto body=unb64(str(item,"body"));std::optional<i64> first,last;if(item.contains("start"))first=num(item,"start");if(item.contains("end"))last=num(item,"end");
    Http response(str(item,"url"),headers,prefs,cancel,first,last,"",item.contains("body")?&body:nullptr,yes(item,"redirects",true),str(item,"session").empty()?std::shared_ptr<HttpSession>{}:pools.at(str(item,"session")),yes(item,"head"));
    result["status"]=response.status;result["protocol"]=response.protocol();result["explicitProxyTransport"]=response.request==nullptr;result["finalUrl"]=response.finalUrl;
    result["contentRange"]=response.header(L"Content-Range");result["cookies"]=response.headers(L"Set-Cookie");
    if(num(item,"delayReadMs")>0)Sleep((DWORD)num(item,"delayReadMs"));
    auto bytes=response.all(4*1024*1024,cancel);auto file=fs::path(argv[2]).parent_path()/wide(id+".bin");writeBytes(file,bytes);
    result["bytes"]=bytes.size();result["sha256"]=fileHash(file);result["output"]=utf8(file.wstring());result["ok"]=true;
   }catch(const Cancelled& e){result["cancelled"]=true;result["error"]=e.what();result["ok"]=false;}
   catch(const std::exception& e){result["error"]=e.what();result["ok"]=false;}
   result["elapsedMs"]=GetTickCount64()-started;
   std::cout<<id<<": "<<(yes(result,"ok")?str(result,"protocol"):str(result,"error"))<<std::endl;
   return result;
  };
  for(const auto& item:config.value("cases",Json::array()))results["cases"].push_back(runCase(item));
  for(const auto& group:config.value("groups",Json::array())){
   auto start=GetTickCount64();
   if(yes(group,"parallel")){
    std::atomic<bool> go=false;std::vector<std::future<Json>> pending;
    for(const auto& item:group.at("cases"))pending.push_back(std::async(std::launch::async,[&,item]{while(!go.load())std::this_thread::yield();return runCase(item);}));
    go=true;for(auto& task:pending)results["cases"].push_back(task.get());
   }else for(const auto& item:group.at("cases"))results["cases"].push_back(runCase(item));
   results["groups"].push_back(Json{{"id",str(group,"id")},{"elapsedMs",GetTickCount64()-start}});
  }
  pools.clear();
  atomicText(fs::path(argv[2]),results.dump(2),false);CoUninitialize();WSACleanup();return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;CoUninitialize();WSACleanup();return 1;}
}
