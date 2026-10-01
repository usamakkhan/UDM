#pragma once
#include "GrabberDestinations.hpp"
namespace udm {
struct OfflinePathLess {
 bool operator()(const std::wstring& a,const std::wstring& b)const{
  return CompareStringOrdinal(a.c_str(),(int)a.size(),b.c_str(),(int)b.size(),TRUE)==CSTR_LESS_THAN;
 }
};
inline void validateOfflineArchivePath(const std::string& name){
 auto value=wide(name);
 if(name.empty()||name.size()>65535||value.size()>1800||utf8(value)!=name||name.front()=='/'||name.back()=='/'||name.find_first_of("\\:")!=std::string::npos)throw std::runtime_error("Invalid offline archive path.");
 size_t start=0;
 for(;;){auto end=name.find('/',start);auto part=name.substr(start,end==std::string::npos?end:end-start);
  if(part.empty()||part=="."||part==".."||safeName(part)!=part||std::any_of(part.begin(),part.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Invalid offline archive component.");
  if(end==std::string::npos)break;start=end+1;
 }
}
inline std::string offlineArchiveReference(const std::string& from,const std::string& destination){
 validateOfflineArchivePath(from);validateOfflineArchivePath(destination);
 auto relative=fs::path(wide(destination)).lexically_relative(fs::path(wide(from)).parent_path());
 if(relative.empty()||relative.is_absolute())throw std::runtime_error("Cannot make a local website reference.");
 auto value=utf8(relative.generic_wstring());std::string encoded;const char* hex="0123456789ABCDEF";
 for(unsigned char c:value){if((c<128&&isalnum(c))||c=='/'||c=='-'||c=='_'||c=='.'||c=='~')encoded+=(char)c;else{encoded+='%';encoded+=hex[c>>4];encoded+=hex[c&15];}}
 return encoded;
}
class OfflineArchiveLayout {
 std::string origin,externalRoot;
 std::set<std::wstring,OfflinePathLess> occupied;
 std::map<std::string,std::string> directories;
 static std::string join(const std::string& parent,const std::string& name){return parent.empty()?name:parent+"/"+name;}
 std::string reserve(const std::string& parent,const std::string& suggested,bool directory){
  auto original=fs::path(wide(safeName(suggested)));auto candidate=utf8(original.wstring());
  for(int suffix=0;;++suffix){
   if(suffix)candidate=directory?utf8(original.wstring())+" ("+std::to_string(suffix)+")":utf8(original.stem().wstring())+" ("+std::to_string(suffix)+")"+utf8(original.extension().wstring());
   auto path=join(parent,candidate);validateOfflineArchivePath(path);
   if(occupied.insert(wide(path)).second)return path;
  }
 }
 std::string directory(const std::string& parent,const std::string& identity,const std::string& name){
  auto found=directories.find(identity);if(found!=directories.end())return found->second;
  auto path=reserve(parent,name,true);directories.emplace(identity,path);return path;
 }
public:
 explicit OfflineArchiveLayout(const std::string& start):origin(Url(start).origin){
  occupied.insert(L"index.html");occupied.insert(L"UDM-offline-report.json");
  externalRoot=directory("","@external","_external");
 }
 std::string file(const std::string& address,const std::string& extension,bool first=false){
  Url url(address);auto relative=grabberRelativeFile(address);std::vector<std::string> parts;
  for(const auto& part:relative)parts.push_back(utf8(part.wstring()));
  std::vector<std::string> identities;size_t begin=0;
  while(begin<url.path.size()){auto end=url.path.find('/',begin);if(end==std::string::npos)end=url.path.size();if(end>begin)identities.push_back(url.path.substr(0,end));begin=end+1;}
  std::string parent;
  if(url.origin!=origin)parent=directory(externalRoot,"@origin:"+url.origin,safeName(url.scheme+"_"+url.host+"_"+std::to_string(url.port)));
  for(size_t i=0;i+1<parts.size();++i)parent=directory(parent,url.origin+"|"+identities.at(i),parts[i]);
  auto name=parts.back(),ext=lower(utf8(fs::path(wide(name)).extension().wstring()));
  if(extension==".html"){if(ext!=".html"&&ext!=".htm"&&ext!=".xhtml")name+=".html";}
  else if(extension!=".bin"&&ext!=extension)name+=extension;
  else if(ext.empty())name+=extension;
  if(first&&parent.empty()&&lower(name)=="index.html")return "index.html";
  return reserve(parent,name,false);
 }
};
}

