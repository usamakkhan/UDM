#pragma once
#include "Core.hpp"
namespace udm {
// The same request is used on first launch and when forwarding to an open app.
struct LaunchOptions {
 fs::path data=defaultData();std::string address,folder,name,tag;
 bool background=false,paused=false,silent=false,recovery=false,startQueue=false;DWORD waitProcess=0;
 Json request()const{if(startQueue)return {{"action","cli-start-queue"},{"background",background}};return {{"action","cli-add"},{"url",address},{"folder",folder},{"filename",name},{"paused",paused},{"silent",silent},{"background",background}};}
};
inline LaunchOptions parseLaunch(const std::vector<std::wstring>& args){
 LaunchOptions options;
 for(size_t i=0;i<args.size();++i){const auto& arg=args[i];
  auto value=[&]()->const std::wstring&{if(i+1>=args.size())throw std::runtime_error("Missing value for "+utf8(arg)+".");return args[++i];};
  if(arg==L"--recovery")options.recovery=true;
  else if(arg==L"--wait-process"){auto v=utf8(value());if(v.empty()||v.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid process identifier.");auto pid=std::stoull(v);if(!pid||pid>MAXDWORD)throw std::runtime_error("Invalid process identifier.");options.waitProcess=(DWORD)pid;}
  else if(arg==L"--data-dir")options.data=fs::absolute(value());
  else if(arg==L"--instance-tag")options.tag=utf8(value());
  else if(arg==L"--background")options.background=true;
  else if(arg==L"/s"||arg==L"--start-queue")options.startQueue=true;
  else if(arg==L"--paused"||arg==L"/a")options.paused=true;
  else if(arg==L"--silent"||arg==L"/n")options.silent=true;
  else if(arg==L"--add"||arg==L"/d")options.address=utf8(value());
  else if(arg==L"--folder"||arg==L"/p")options.folder=utf8(fs::absolute(value()).lexically_normal().wstring());
  else if(arg==L"--name"||arg==L"/f")options.name=utf8(value());
  else if(arg.rfind(L"udm://",0)==0){auto text=utf8(arg);auto pos=text.find("url=");if(pos!=std::string::npos)options.address=unescape(text.substr(pos+4));}
  else throw std::runtime_error("Unknown UDM option: "+utf8(arg));
 }
 if(options.recovery&&!options.address.empty())throw std::runtime_error("Recovery cannot add a download.");
 if(options.startQueue&&(options.recovery||options.paused||!options.address.empty()))throw std::runtime_error("Start queue must be used separately from adding a download or recovery.");
 return options;
}
}
