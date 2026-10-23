#pragma once
#include "Core.hpp"
#include <sstream>
namespace udm {
// The same request is used on first launch and when forwarding to an open app.
struct LaunchOptions {
 fs::path data=defaultData();std::string address,folder,name,tag;
 bool background=false,paused=false,silent=false,recovery=false,startQueue=false,quitAfterDownload=false,hangupAfterDownload=false;DWORD waitProcess=0;
 Json request()const{if(startQueue)return {{"action","cli-start-queue"},{"background",background}};return {{"action","cli-add"},{"url",address},{"folder",folder},{"filename",name},{"paused",paused},{"hangup",hangupAfterDownload},{"silent",silent},{"background",background}};}
};
// Explorer's "Download with UDM" verb on Internet Shortcut (.url) files runs UDM.exe --shortcut "%1".
// The file is an INI: the URL key of its [InternetShortcut] section is the address; only web and FTP addresses are accepted.
inline std::string readInternetShortcut(const fs::path& path){
 if(!fs::is_regular_file(path))throw std::runtime_error("The Internet Shortcut file was not found.");
 auto text=readText(path,64*1024);if(text.rfind("\xEF\xBB\xBF",0)==0)text.erase(0,3);
 std::istringstream lines(text);std::string line,address;bool section=false,found=false;
 while(std::getline(lines,line)){
  if(!line.empty()&&line.back()=='\r')line.pop_back();line=trim(line);if(line.empty()||line[0]==';'||line[0]=='#')continue;
  if(line.front()=='['){section=lower(line)=="[internetshortcut]";continue;}
  if(!section)continue;auto at=line.find('=');if(at==std::string::npos||lower(trim(line.substr(0,at)))!="url")continue;
  if(found)throw std::runtime_error("The Internet Shortcut names more than one address.");address=trim(line.substr(at+1));found=true;
 }
 if(!found||address.empty())throw std::runtime_error("The Internet Shortcut has no address.");
 if(address.size()>16000||std::any_of(address.begin(),address.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("The Internet Shortcut address is invalid.");
 Url url(address);if(url.scheme!="http"&&url.scheme!="https"&&url.scheme!="ftp")throw std::runtime_error("UDM downloads only web and FTP addresses from Internet Shortcuts.");
 return address;
}
inline LaunchOptions parseLaunch(const std::vector<std::wstring>& args){
 LaunchOptions options;
 for(size_t i=0;i<args.size();++i){const auto& arg=args[i];
  auto value=[&]()->const std::wstring&{if(i+1>=args.size())throw std::runtime_error("Missing value for "+utf8(arg)+".");return args[++i];};
  if(arg==L"--recovery")options.recovery=true;
  else if(arg==L"--wait-process"){auto v=utf8(value());if(v.empty()||v.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Invalid process identifier.");auto pid=std::stoull(v);if(!pid||pid>MAXDWORD)throw std::runtime_error("Invalid process identifier.");options.waitProcess=(DWORD)pid;}
  else if(arg==L"--data-dir")options.data=fs::absolute(value());
  else if(arg==L"--instance-tag")options.tag=utf8(value());
  else if(arg==L"--background")options.background=true;
  else if(arg==L"/h"||arg==L"--hangup-after-download")options.hangupAfterDownload=true;
  else if(arg==L"/q"||arg==L"--quit-after-download")options.quitAfterDownload=true;
  else if(arg==L"/s"||arg==L"--start-queue")options.startQueue=true;
  else if(arg==L"--paused"||arg==L"/a")options.paused=true;
  else if(arg==L"--silent"||arg==L"/n")options.silent=true;
  else if(arg==L"--add"||arg==L"/d")options.address=utf8(value());
  else if(arg==L"--shortcut"||arg==L"/u")options.address=readInternetShortcut(fs::absolute(value()));
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
