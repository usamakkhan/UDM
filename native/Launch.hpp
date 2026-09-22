#pragma once
#include "Core.hpp"
namespace udm {
// The same request is used on first launch and when forwarding to an open app.
struct LaunchOptions {
 fs::path data=defaultData();std::string address,folder,name,tag;
 bool background=false,paused=false,silent=false;
 Json request()const{return {{"action","cli-add"},{"url",address},{"folder",folder},{"filename",name},{"paused",paused},{"silent",silent},{"background",background}};}
};
inline LaunchOptions parseLaunch(const std::vector<std::wstring>& args){
 LaunchOptions options;
 for(size_t i=0;i<args.size();++i){const auto& arg=args[i];
  auto value=[&]()->const std::wstring&{if(i+1>=args.size())throw std::runtime_error("Missing value for "+utf8(arg)+".");return args[++i];};
  if(arg==L"--data-dir")options.data=fs::absolute(value());
  else if(arg==L"--instance-tag")options.tag=utf8(value());
  else if(arg==L"--background")options.background=true;
  else if(arg==L"--paused")options.paused=true;
  else if(arg==L"--silent"||arg==L"/n")options.silent=true;
  else if(arg==L"--add"||arg==L"/d")options.address=utf8(value());
  else if(arg==L"--folder"||arg==L"/p")options.folder=utf8(fs::absolute(value()).lexically_normal().wstring());
  else if(arg==L"--name"||arg==L"/f")options.name=utf8(value());
  else if(arg.rfind(L"udm://",0)==0){auto text=utf8(arg);auto pos=text.find("url=");if(pos!=std::string::npos)options.address=unescape(text.substr(pos+4));}
  else throw std::runtime_error("Unknown UDM option: "+utf8(arg));
 }
 return options;
}
}
