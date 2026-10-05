#include "Version.hpp"
#pragma once
#include "Core.hpp"
#include <array>
#include <functional>
namespace udm {
inline constexpr const char* currentProductVersion=UDM_VERSION_STRING;
inline constexpr const char* releaseEndpoint="https://api.github.com/repos/usamakkhan/UDM/releases/latest";
inline constexpr const char* releasesPage="https://github.com/usamakkhan/UDM/releases";
inline std::array<unsigned,4> releaseVersion(std::string value){
 if(value.rfind("UDM-",0)==0)value.erase(0,4);else if(!value.empty()&&(value[0]=='v'||value[0]=='V'))value.erase(0,1);
 std::array<unsigned,4> result{};size_t part=0,digits=0;
 for(char ch:value){if(ch=='.'){if(!digits||part==3)throw std::runtime_error("The published release has an unsupported version number.");++part;digits=0;}else if(ch>='0'&&ch<='9'){if(++digits>6)throw std::runtime_error("The published release has an unsupported version number.");result[part]=result[part]*10+(ch-'0');}else throw std::runtime_error("The published release has an unsupported version number.");}
 if(!digits||part<1)throw std::runtime_error("The published release has an unsupported version number.");return result;
}
struct PublishedRelease{std::string version,page;bool newer=false;};
inline PublishedRelease publishedRelease(const Json& value,const std::string& current=currentProductVersion){
 if(!value.is_object()||!value.contains("draft")||!value["draft"].is_boolean()||!value.contains("prerelease")||!value["prerelease"].is_boolean()||yes(value,"draft")||yes(value,"prerelease"))throw std::runtime_error("No supported stable release was returned.");
 auto tag=str(value,"tag_name"),page=str(value,"html_url");if(tag.size()>64||page.size()>2048)throw std::runtime_error("Invalid published release metadata.");
 Url url(page);const std::string prefix="/usamakkhan/UDM/releases/tag/";
 if(url.scheme!="https"||url.host!="github.com"||url.port!=443||url.path.rfind(prefix,0)!=0||url.path.size()==prefix.size()||!url.query.empty())throw std::runtime_error("The release page is outside the UDM repository.");
 return {tag,page,releaseVersion(tag)>releaseVersion(current)};
}
inline PublishedRelease fetchPublishedRelease(const Json& settings,const Cancel& cancel){
 Headers headers={{"Accept","application/vnd.github+json"},{"User-Agent",std::string("UDM/")+currentProductVersion}};
 Http request(releaseEndpoint,headers,settings,cancel,{}, {},"",nullptr,false);
 if(request.status==404)throw std::runtime_error("No public release is available, or the UDM repository is private.");
 if(request.status==403||request.status==429)throw std::runtime_error("The release service refused this check or reached its request limit. Try again later.");
 if(request.status!=200)throw std::runtime_error("The release service returned HTTP "+std::to_string(request.status)+".");
 auto body=request.all(256*1024,cancel);cancel.check();return publishedRelease(Json::parse(body.begin(),body.end()));
}
using ReleaseLoader=std::function<PublishedRelease(const Cancel&)>;
}
