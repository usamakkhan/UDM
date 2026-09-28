#pragma once
#include "Core.hpp"
#include <sstream>
#include <algorithm>
namespace udm {
inline std::string asciiDomain(std::string host){
 while(!host.empty()&&host.back()=='.')host.pop_back();if(host.empty()||host.find(':')!=std::string::npos)return lower(host);
 auto input=wide(host);int count=IdnToAscii(IDN_USE_STD3_ASCII_RULES,input.data(),(int)input.size(),nullptr,0);if(!count)throw std::runtime_error("Invalid international domain name.");std::wstring output(count,0);if(!IdnToAscii(IDN_USE_STD3_ASCII_RULES,input.data(),(int)input.size(),output.data(),count))throw std::runtime_error("Cannot normalize domain name.");return lower(utf8(output));
}
class PublicSuffix {
 std::set<std::string> exact,wildcards,exceptions;
public:
 explicit PublicSuffix(const std::string& text){
  std::istringstream lines(text);std::string line;while(std::getline(lines,line)){line=trim(line);if(line.empty()||line.rfind("//",0)==0)continue;line=line.substr(0,line.find_first_of(" \t"));bool exception=line[0]=='!',wild=line.rfind("*.",0)==0;if(exception)line.erase(0,1);if(wild)line.erase(0,2);if(line=="*")continue;auto rule=asciiDomain(line);(exception?exceptions:wild?wildcards:exact).insert(rule);}
 }
 std::string domain(const std::string& value)const{
  auto host=asciiDomain(value);if(host.empty()||host.find(':')!=std::string::npos||host.find_first_not_of("0123456789.")==std::string::npos)return host;
  std::vector<size_t> offsets{0};for(size_t i=0;i<host.size();++i)if(host[i]=='.')offsets.push_back(i+1);size_t suffix=1;
  for(size_t i=0;i<offsets.size();++i){auto part=host.substr(offsets[i]);if(exceptions.count(part))return part;if(exact.count(part))suffix=std::max(suffix,offsets.size()-i);if(i>0&&wildcards.count(part))suffix=std::max(suffix,offsets.size()-i+1);}
  return suffix>=offsets.size()?"":host.substr(offsets[offsets.size()-suffix-1]);
 }
 static const PublicSuffix& installed(){static const auto list=[] {auto text=readText(appDir()/L"assets/public_suffix_list.dat",2*1024*1024);if(text.find("===BEGIN ICANN DOMAINS===")==std::string::npos||text.find("===BEGIN PRIVATE DOMAINS===")==std::string::npos)throw std::runtime_error("The domain-boundary list is missing or invalid. Repair UDM before exploring a whole domain.");return PublicSuffix(text);}();return list;}
};
}
