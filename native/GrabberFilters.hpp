#pragma once
#include "Core.hpp"
#include "PublicSuffix.hpp"
#include <sstream>
#include <algorithm>
namespace udm {
inline std::vector<std::string> grabberPatterns(const std::string& input){
 if(input.size()>8192)throw std::runtime_error("A Grabber filter list must be at most 8,192 bytes.");std::vector<std::string> result;size_t begin=0;
 while(begin<input.size()){auto end=input.find_first_of(",\r\n",begin);if(end==std::string::npos)end=input.size();auto item=trim(input.substr(begin,end-begin));begin=end+1;if(item.empty())continue;if(item.size()>2048||std::any_of(item.begin(),item.end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("Use comma-separated Grabber patterns without control characters.");result.push_back(lower(item));if(result.size()>128)throw std::runtime_error("Use at most 128 patterns per filter list.");}return result;
}
inline bool grabberGlob(const std::string& pattern,const std::string& value){
 const auto text=lower(value);size_t p=0,v=0,star=std::string::npos,retry=0;while(v<text.size()){if(p<pattern.size()&&pattern[p]==text[v]){++p;++v;}else if(p<pattern.size()&&pattern[p]=='*'){star=p++;retry=v;}else if(star!=std::string::npos){p=star+1;v=++retry;}else return false;}while(p<pattern.size()&&pattern[p]=='*')++p;return p==pattern.size();
}
inline std::string grabberCanonical(const std::string& value){Url url(value);if(url.scheme!="http"&&url.scheme!="https")throw std::runtime_error("Site exploration requires HTTP or HTTPS.");return url.origin+url.path+url.query;}
inline std::string grabberFileName(const std::string& address){auto path=Url(address).path;return path.empty()||path.back()=='/'?"index.html":safeName(unescape(path));}
inline std::string grabberLegacyPatterns(const Json& project){std::string value;for(auto ext:words(str(project,"Extensions","zip pdf jpg png mp4 mp3"))){if(!value.empty())value+=",";value+=ext=="*"?"*":"*."+ext;}return value;}
inline std::string grabberPath(const std::string& value){
 auto path=unescape(value);std::replace(path.begin(),path.end(),'\\','/');std::vector<std::string> parts;size_t begin=0;while(begin<path.size()){auto end=path.find('/',begin);if(end==std::string::npos)end=path.size();auto part=path.substr(begin,end-begin);begin=end+1;if(part.empty()||part==".")continue;if(part==".."){if(!parts.empty())parts.pop_back();}else parts.push_back(part);}std::string result="/";for(size_t i=0;i<parts.size();++i){if(i)result+="/";result+=parts[i];}if(!parts.empty()&&!path.empty()&&path.back()=='/')result+="/";return result;
}
inline Headers grabberHeaders(const std::string& referrer,const std::string& destination){
 if(referrer.empty())return {};Url source(referrer),target(destination);if(source.scheme=="https"&&target.scheme!="https")return {};return {{"Referer",source.origin==target.origin?source.origin+source.path+source.query:source.origin+"/"}};
}
struct GrabberFilters {
 Url source;std::vector<std::string> includeFiles,excludeFiles,pageInclude,pageExclude,fileInclude,fileExclude;bool legacy=false,filesSame=true,pagesSame=true,subdomains=false,noParents=false,hideDuplicates=false;int depth=1,externalDepth=0,maxPages=20;i64 minimum=0,maximum=0;std::string mainDomain,sourceDirectory;
 explicit GrabberFilters(const Json& project):source(str(project,"StartUrl")){
  if(source.scheme!="http"&&source.scheme!="https")throw std::runtime_error("Site exploration requires HTTP or HTTPS.");
  for(const char* key:{"FileInclude","FileExclude","PageIncludePaths","PageExcludePaths","FileIncludePaths","FileExcludePaths"})if(project.contains(key)&&!project[key].is_string())throw std::runtime_error("Invalid Grabber filter text.");
  for(const char* key:{"FilesSameSite","ExploreSameSite","ExploreSubdomains","NoParentDirectories","HideDuplicates"})if(project.contains(key)&&!project[key].is_boolean())throw std::runtime_error("Invalid Grabber filter switch.");
  for(const char* key:{"Depth","ExternalDepth","MaxPages","MinFileBytes","MaxFileBytes"})if(project.contains(key)&&!project[key].is_number_integer())throw std::runtime_error("Grabber limits must be whole numbers.");
  legacy=!project.contains("FileInclude");filesSame=yes(project,"FilesSameSite",true);pagesSame=yes(project,"ExploreSameSite",true);subdomains=yes(project,"ExploreSubdomains");noParents=yes(project,"NoParentDirectories");hideDuplicates=yes(project,"HideDuplicates")&&!yes(project,"OriginalSubfolders");
  auto levels=num(project,"Depth",1),other=num(project,"ExternalDepth"),pages=num(project,"MaxPages",20);minimum=num(project,"MinFileBytes");maximum=num(project,"MaxFileBytes");
  if(levels<0||levels>20||other<0||other>20||pages<1||pages>10000||minimum<0||maximum<0||minimum>(1LL<<50)||maximum>(1LL<<50)||(maximum&&minimum>maximum))throw std::runtime_error("Use depths from 0 to 20, 1 to 10,000 pages, and valid minimum/maximum file sizes.");depth=(int)levels;externalDepth=(int)other;maxPages=(int)pages;
  includeFiles=grabberPatterns(str(project,"FileInclude",grabberLegacyPatterns(project)));if(includeFiles.empty())includeFiles.push_back("*");excludeFiles=grabberPatterns(str(project,"FileExclude"));pageInclude=grabberPatterns(str(project,"PageIncludePaths"));pageExclude=grabberPatterns(str(project,"PageExcludePaths"));fileInclude=grabberPatterns(str(project,"FileIncludePaths"));fileExclude=grabberPatterns(str(project,"FileExcludePaths"));auto logout=grabberPatterns(str(project,"LogoutPages"));pageExclude.insert(pageExclude.end(),logout.begin(),logout.end());fileExclude.insert(fileExclude.end(),logout.begin(),logout.end());
  if(subdomains)mainDomain=PublicSuffix::installed().domain(source.host);auto path=grabberPath(source.path);sourceDirectory=path.substr(0,path.rfind('/')+1);
 }
 bool currentSite(const Url& url)const{if(asciiDomain(url.host)==asciiDomain(source.host))return true;return subdomains&&!mainDomain.empty()&&PublicSuffix::installed().domain(url.host)==mainDomain;}
 bool locationMatch(const std::vector<std::string>& rules,const Url& url)const{
  for(const auto& pattern:rules){if(pattern=="<start page>"){if(grabberCanonical(url.full)==grabberCanonical(source.full))return true;continue;}const bool full=pattern.find("://")!=std::string::npos,relative=!pattern.empty()&&pattern[0]=='/';auto path=relative?url.path+url.query:full?url.origin+url.path+url.query:url.origin.substr(url.scheme.size()+3)+url.path+url.query;if(grabberGlob(pattern,path)||grabberGlob(pattern,unescape(path)))return true;}return false;
 }
 bool pageAllowed(const std::string& address,bool first=false)const{
  Url url(address);if(url.scheme!="http"&&url.scheme!="https")return false;if(legacy&&pagesSame&&url.origin!=source.origin)return false;if(pagesSame&&!currentSite(url))return false;
  if(noParents&&currentSite(url)&&grabberPath(url.path).rfind(sourceDirectory,0)!=0)return false;
  return !locationMatch(pageExclude,url)&&(first||pageInclude.empty()||locationMatch(pageInclude,url));
 }
 bool fileLocationAllowed(const std::string& address)const{
  Url url(address);if(url.scheme!="http"&&url.scheme!="https")return false;if(filesSame&&(legacy?url.origin!=source.origin:asciiDomain(url.host)!=asciiDomain(source.host)))return false;
  return !locationMatch(fileExclude,url)&&(fileInclude.empty()||locationMatch(fileInclude,url));
 }
 bool nameAllowed(const std::string& address,const std::string& filename,bool page=false)const{
  if(legacy&&page)return false;auto matches=[&](const std::vector<std::string>& patterns){for(const auto& pattern:patterns){if(pattern=="<start page>"){if(grabberCanonical(address)==grabberCanonical(source.full))return true;}else if(grabberGlob(pattern,filename))return true;}return false;};return matches(includeFiles)&&!matches(excludeFiles);
 }
 bool sizeAllowed(i64 size)const{return !(minimum||maximum)||(size>=0&&size>=minimum&&(!maximum||size<=maximum));}
 std::string signature()const{Json value={{"StartUrl",grabberCanonical(source.full)},{"Legacy",legacy},{"Files",includeFiles},{"Exclude",excludeFiles},{"PageInclude",pageInclude},{"PageExclude",pageExclude},{"FileInclude",fileInclude},{"FileExclude",fileExclude},{"FilesSame",filesSame},{"PagesSame",pagesSame},{"Subdomains",subdomains},{"NoParents",noParents},{"HideDuplicates",hideDuplicates},{"Depth",depth},{"ExternalDepth",externalDepth},{"Minimum",minimum},{"Maximum",maximum}};return value.dump();}
};
}
