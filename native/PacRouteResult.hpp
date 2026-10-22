#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include "third_party/json.hpp"
namespace udm {
struct PacRoute {std::string kind,host;unsigned port=0;};
inline std::vector<PacRoute> parsePacRouteText(const std::string& text){
 auto invalid=[](){throw std::runtime_error("Proxy script returned an invalid route. No direct fallback was used.");};
 if(text.empty()||text.size()>8192||text.find('\0')!=std::string::npos)invalid();
 auto trimmed=[](const std::string& value){const auto first=value.find_first_not_of(" \t\r\n"),last=value.find_last_not_of(" \t\r\n");return first==std::string::npos?std::string():value.substr(first,last-first+1);};
 std::vector<PacRoute> routes;
 size_t begin=0;
 while(begin<text.size()){
  auto end=text.find(';',begin);if(end==std::string::npos)end=text.size();
  const auto item=trimmed(text.substr(begin,end-begin));begin=end+1;
  if(item.empty())continue;
  if(routes.size()==32)invalid();
  const auto split=item.find_first_of(" \t");const auto type=item.substr(0,split);
  if(type=="DIRECT"){
   if(split!=std::string::npos)invalid();
   routes.push_back({"direct","",0});continue;
  }
  std::string kind;
  if(type=="PROXY"||type=="HTTP")kind="http";
  else if(type=="HTTPS")kind="https";
  else if(type=="SOCKS"||type=="SOCKS4")kind="socks4";
  else if(type=="SOCKS5")kind="socks5";
  else invalid();
  if(split==std::string::npos)invalid();
  const auto endpoint=trimmed(item.substr(split));
  if(endpoint.empty()||endpoint.find_first_of(" \t\r\n/@?#\\\"<>|")!=std::string::npos)invalid();
  std::string host,port;
  if(endpoint.front()=='['){
   const auto close=endpoint.find(']');
   if(close==std::string::npos||close+1>=endpoint.size()||endpoint[close+1]!=':')invalid();
   host=endpoint.substr(1,close-1);port=endpoint.substr(close+2);
   IN6_ADDR address{};if(InetPtonA(AF_INET6,host.c_str(),&address)!=1)invalid();
  }else{
   const auto colon=endpoint.find(':');
   if(colon==std::string::npos||endpoint.find(':',colon+1)!=std::string::npos)invalid();
   host=endpoint.substr(0,colon);port=endpoint.substr(colon+1);
   if(host.find_first_of("[]")!=std::string::npos)invalid();
  }
  if(host.empty()||host.size()>255||port.empty()||port.size()>5)invalid();
  for(unsigned char c:host)if(c<33||c==127)invalid();
  unsigned number=0;for(char c:port){if(c<'0'||c>'9')invalid();number=number*10+static_cast<unsigned>(c-'0');}
  if(!number||number>65535)invalid();
  routes.push_back({kind,host,number});
 }
 if(routes.empty())invalid();
 return routes;
}
inline std::vector<PacRoute> parsePacWorkerReply(const std::string& text,unsigned exitCode){
 if(exitCode||text.size()>65536)throw std::runtime_error("PAC worker failed. No direct fallback was used.");
 std::set<std::string> keys;
 auto reply=nlohmann::json::parse(text,[&](int depth,nlohmann::json::parse_event_t event,nlohmann::json& value){
  if(depth==1&&event==nlohmann::json::parse_event_t::key&&!keys.insert(value.get<std::string>()).second)
   throw std::runtime_error("PAC worker response contains duplicate fields.");
  if(depth>4)throw std::runtime_error("PAC worker response is too deeply nested.");return true;
 },false);
 if(!reply.is_object()||!reply.contains("ok")||!reply["ok"].is_boolean()||!reply["ok"].get<bool>()||
    !reply.contains("result")||!reply["result"].is_string()||
    !reply.contains("interrupted")||!reply["interrupted"].is_boolean()||reply["interrupted"].get<bool>()||
    !reply.contains("error")||!reply["error"].is_string()||!reply["error"].get_ref<const std::string&>().empty())
  throw std::runtime_error("PAC worker returned an invalid response. No direct fallback was used.");
 return parsePacRouteText(reply["result"].get_ref<const std::string&>());
}
}
