#pragma once
#include "Catalog.hpp"
#include <sstream>
#include <optional>
namespace udm {
struct IdmExportRecord { std::string url,referer,userAgent,cookie;std::optional<std::string> post; };
inline void validateIdmRecord(const IdmExportRecord& row){
 Url address(row.url);if(row.url.empty()||std::any_of(row.url.begin(),row.url.end(),[](unsigned char c){return c<33||c==127;}))throw std::runtime_error("Invalid address in the export file.");
 validateHeaders({{"Referer",row.referer},{"User-Agent",row.userAgent},{"Cookie",row.cookie}});
 if(!row.referer.empty()){Url ref(row.referer);if(ref.scheme!="http"&&ref.scheme!="https")throw std::runtime_error("The saved Referer must use HTTP or HTTPS.");}
 if(row.post){if(address.scheme!="http"&&address.scheme!="https")throw std::runtime_error("Saved form requests require HTTP or HTTPS.");if(row.post->size()>MaxBrowserPostBytes||std::any_of(row.post->begin(),row.post->end(),[](unsigned char c){return c<32||c==127;}))throw std::runtime_error("The export format supports a single-line form body only.");}
}
inline std::string decodeIdmExport(std::string input,bool legacy=false){
 if(input.size()>16*1024*1024)throw std::runtime_error("Import files must be at most 16 MiB.");
 if(input.rfind("\xef\xbb\xbf",0)==0)input.erase(0,3);
 if(input.size()>=2&&((unsigned char)input[0]==0xff&&(unsigned char)input[1]==0xfe||(unsigned char)input[0]==0xfe&&(unsigned char)input[1]==0xff)){
  bool le=(unsigned char)input[0]==0xff;if(input.size()%2)throw std::runtime_error("Truncated Unicode export file.");std::wstring value;
  for(size_t i=2;i<input.size();i+=2){unsigned a=(unsigned char)input[i],b=(unsigned char)input[i+1];value.push_back((wchar_t)(le?a|(b<<8):b|(a<<8)));}
  int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),(int)value.size(),nullptr,0,nullptr,nullptr);if(!bytes&&!value.empty())throw std::runtime_error("Invalid Unicode export file.");input.assign(bytes,0);if(bytes)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,value.data(),(int)value.size(),input.data(),bytes,nullptr,nullptr);
 }else if(!input.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),(int)input.size(),nullptr,0)){
  if(!legacy)throw std::runtime_error("The export file must contain valid UTF-8 text.");
  int size=MultiByteToWideChar(CP_ACP,0,input.data(),(int)input.size(),nullptr,0);if(!size)throw std::runtime_error("Cannot decode this legacy export file.");std::wstring value(size,0);MultiByteToWideChar(CP_ACP,0,input.data(),(int)input.size(),value.data(),size);input=utf8(value);
 }
 if(input.find('\0')!=std::string::npos)throw std::runtime_error("The export file contains a null character.");return input;
}
inline std::vector<std::string> importTextUrls(const std::string& bytes){
 auto text=decodeIdmExport(bytes);std::regex pattern(R"((?:https?|ftp)://[^\s<>"']+)",std::regex::icase);std::vector<std::string> urls;std::set<std::string> seen;
 for(auto it=std::sregex_iterator(text.begin(),text.end(),pattern);it!=std::sregex_iterator();++it){auto address=it->str();try{Url checked(address);}catch(...){continue;}if(seen.insert(address).second)urls.push_back(address);if(urls.size()>1000)throw std::runtime_error("Import at most 1,000 text links at a time.");}
 if(urls.empty())throw std::runtime_error("No supported download addresses were found in this text file.");return urls;
}
inline std::vector<IdmExportRecord> parseIdmExport(const std::string& bytes,bool legacy=false){
 auto input=decodeIdmExport(bytes,legacy);std::istringstream lines(input);std::string line;std::vector<IdmExportRecord> records;IdmExportRecord current;bool inside=false,haveUrl=false;std::set<std::string> fields;size_t lineNo=0;
 auto fail=[&](const std::string& reason){throw std::runtime_error("Export file, line "+std::to_string(lineNo)+": "+reason);};
 while(std::getline(lines,line)){
  ++lineNo;if(!line.empty()&&line.back()=='\r')line.pop_back();if(line.find('\r')!=std::string::npos)fail("Invalid line ending.");
  auto trimmed=trim(line);if(trimmed.empty()&&!inside)continue;
  if(trimmed=="<"){if(inside)fail("The previous download has no closing >.");inside=true;haveUrl=false;current={};fields.clear();continue;}
  if(trimmed==">"){if(!inside||!haveUrl)fail("A download address is missing.");validateIdmRecord(current);records.push_back(current);if(records.size()>10000)fail("Import at most 10,000 downloads.");inside=false;continue;}
  if(!inside)fail("Expected < before a download.");
  if(!haveUrl){current.url=trimmed;haveUrl=true;continue;}
  auto colon=line.find(':');if(colon==std::string::npos)fail("Expected a saved request field.");auto key=lower(trim(line.substr(0,colon)));auto value=line.substr(colon+1);if(!value.empty()&&value[0]==' ')value.erase(0,1);
  if(!fields.insert(key).second)fail("Duplicate saved request field.");
  if(key=="referer")current.referer=value;else if(key=="user-agent")current.userAgent=value;else if(key=="cookie")current.cookie=value;else if(key=="pd")current.post=value;else fail("Unsupported saved request field. Import was not started.");
 }
 if(inside)fail("Missing closing >. Import was not started.");if(records.empty())throw std::runtime_error("The export file contains no downloads.");return records;
}
inline std::string serializeIdmExport(const std::vector<IdmExportRecord>& records){
 if(records.empty()||records.size()>10000)throw std::runtime_error("Select 1 to 10,000 downloads.");std::string result;
 for(const auto& row:records){validateIdmRecord(row);result+="<\r\n"+row.url;if(!row.referer.empty())result+="\r\nreferer: "+row.referer;if(!row.cookie.empty())result+="\r\ncookie: "+row.cookie;if(row.post)result+="\r\npd: "+*row.post;if(!row.userAgent.empty())result+="\r\nUser-Agent: "+row.userAgent;result+="\r\n>\r\n";if(result.size()>16*1024*1024)throw std::runtime_error("The export exceeds 16 MiB. Select fewer downloads.");}return result;
}
inline std::string exportIdmFile(Manager& manager,const std::vector<JobPtr>& selected,bool session=false){
 Lock lock(manager.mutex);std::vector<IdmExportRecord> records;
 for(auto job:selected){const auto& d=job->data;
  if(capturedMedia(d)||!str(d,"ProtectedSabr").empty()||!str(d,"ProtectedBrowserProxy").empty()||!str(d,"ProtectedBrowserSession").empty()||yes(d,"RequiresRequestCapture")||yes(d,"RequiresMediaCapture")||yes(d,"RequiresBrowserSessionCapture")||yes(d,"RequiresBrowserProxyCapture")||d.contains("OfflineProject"))throw std::runtime_error("This download needs a UDM catalog to preserve its media, browser or project context.");
  auto h=readHeaders(d);for(auto& item:h){auto key=lower(item.first);if(key!="referer"&&key!="user-agent"&&key!="cookie"&&!item.second.empty())throw std::runtime_error("This request uses headers that the IDM export format cannot preserve. Choose UDM catalog.");}
  IdmExportRecord row;row.url=str(d,"Url");row.referer=headerValue(h,"Referer");row.userAgent=headerValue(h,"User-Agent");row.cookie=headerValue(h,"Cookie");auto request=readPostRequest(d);
  if(!request.empty()){if(lower(str(request,"contentType"))!="application/x-www-form-urlencoded")throw std::runtime_error("This form's content type requires a UDM catalog.");auto body=unb64(str(request,"body"));row.post=std::string(body.begin(),body.end());}
  if(!session&&(!row.cookie.empty()||row.post))throw std::runtime_error("Check Include cookies and form data to export this request, or choose UDM catalog for encrypted storage.");records.push_back(std::move(row));
 }return serializeIdmExport(records);
}
inline size_t importIdmFile(Manager& manager,const std::vector<IdmExportRecord>& selected,const std::string& destination,const std::string& queue,bool session){
 if(selected.empty()||selected.size()>10000)throw std::runtime_error("Select 1 to 10,000 downloads.");Lock lock(manager.mutex);Json catalog={{"Format","UDM catalog"},{"Version",1},{"Downloads",Json::array()},{"Queues",Json::array()}};
 auto folder=destination.empty()?str(manager.state["Settings"],"DownloadFolder"):destination;
 for(const auto& row:selected){validateIdmRecord(row);if(!session&&(!row.cookie.empty()||row.post))throw std::runtime_error("Check Use saved cookies and form data for the selected downloads.");Headers h;setHeader(h,"Referer",row.referer);setHeader(h,"User-Agent",row.userAgent);setHeader(h,"Cookie",row.cookie);
  Json record={{"Url",row.url},{"FileName",safeName(unescape(Url(row.url).path))},{"Folder",folder},{"Queue",queue},{"ProtectedHeaders",protect(legacyDictionary(Json(h)).dump())},{"DownloadPage",row.referer}};
  if(row.post){Bytes bytes(row.post->begin(),row.post->end());record["ProtectedRequest"]=protect(Json{{"method","POST"},{"url",row.url},{"contentType","application/x-www-form-urlencoded"},{"body",b64(bytes)}}.dump());}catalog["Downloads"].push_back(record);
 }return importCatalog(manager,catalog,destination,true,false);
}
}
