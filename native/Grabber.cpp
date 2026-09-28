#include "Core.hpp"
#include "GrabberFilters.hpp"
#include "GrabberProject.hpp"
#include "OfflineSite.hpp"
#include "DownloadPreview.hpp"
#include <future>
#include <regex>
#include <deque>
namespace udm {
namespace {
bool pageAddress(const std::string& address){auto path=lower(Url(address).path);auto ext=fs::path(wide(path)).extension().wstring();return ext.empty()||ext==L".html"||ext==L".htm"||ext==L".xhtml"||ext==L".php"||ext==L".asp"||ext==L".aspx"||ext==L".css";}
std::string responseName(const Http& response,const std::string& address){
 auto value=response.header(L"Content-Disposition");if(value.size()>8192)return grabberFileName(address);std::smatch match;
 if(std::regex_search(value,match,std::regex("filename\\*\\s*=\\s*UTF-8''([^;\\r\\n]+)",std::regex::icase)))return safeName(unescape(trim(match[1])));
 if(std::regex_search(value,match,std::regex("filename\\s*=\\s*\"([^\"]+)\"",std::regex::icase)))return safeName(match[1]);
 if(std::regex_search(value,match,std::regex("filename\\s*=\\s*([^;\\s]+)",std::regex::icase)))return safeName(match[1]);return grabberFileName(address);
}
i64 numberHeader(const std::string& text){if(text.empty())return -1;if(text.size()>19||text.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Server returned an invalid file size.");try{return std::stoll(text);}catch(...){throw std::runtime_error("Server file size exceeds the supported limit.");}}
Json metadata(const Http& response,const std::string& address,bool ranged){
 i64 size=-1;if(response.status==416&&ranged&&response.header(L"Content-Range")=="bytes */0")size=0;
 else if(response.status==206&&ranged){std::smatch match;auto range=response.header(L"Content-Range");if(!std::regex_match(range,match,std::regex("bytes 0-0/([0-9]+)")))throw std::runtime_error("Server returned invalid metadata byte ranges.");size=numberHeader(match[1]);auto length=numberHeader(response.header(L"Content-Length"));if(size<1||(length>=0&&length!=1))throw std::runtime_error("Server returned inconsistent metadata headers.");}
 else if(response.status==200)size=numberHeader(response.header(L"Content-Length"));else if(response.status==204)size=0;else throw HttpRejected(response.status,response.header(L"Retry-After"));
 const auto encoding=lower(trim(response.header(L"Content-Encoding")));if(!encoding.empty()&&encoding!="identity")size=-1;
 return {{"Url",address},{"FileName",responseName(response,address)},{"Size",size},{"ContentType",previewMimeType(response.header(L"Content-Type"))},{"Selected",true},{"DownloadId",nullptr}};
}
struct RequestResult {std::unique_ptr<Http> response;std::string address;};
RequestResult request(const std::string& initial,const std::string& referrer,const GrabberFilters& filters,const Json& project,const Json& prefs,const Cancel& cancel,const std::shared_ptr<HttpSession>& pool,bool file,bool head,bool ranged,bool first=false){
 auto address=grabberCanonical(initial);std::set<std::string> redirects;
 for(int hop=0;hop<10;++hop){cancel.check();if(file?!filters.fileLocationAllowed(address):!filters.pageAllowed(address,first))throw std::runtime_error("Redirect or address excluded by the project filters.");if(!redirects.insert(address).second)throw std::runtime_error("Redirect loop while exploring the site.");std::unique_ptr<Http> response;
  for(int attempt=0;;++attempt){auto headers=grabberRequestHeaders(project,referrer,address);headers["Accept-Encoding"]="identity";response=std::make_unique<Http>(address,headers,prefs,cancel,ranged?std::optional<i64>(0):std::nullopt,ranged?std::optional<i64>(0):std::nullopt,"",nullptr,false,pool,head);HttpRejected error(response->status,response->header(L"Retry-After"));if(!error.retryable()||attempt>=std::clamp<i64>(num(prefs,"Retries",3),0,10))break;response.reset();cancel.wait(error.delay(attempt));}
  if(response->status>=300&&response->status<400){auto location=response->header(L"Location");if(location.empty())throw std::runtime_error("The server redirected without a destination.");address=grabberCanonical(combineUrl(address,location));continue;}return {std::move(response),address};
 }throw std::runtime_error("Too many redirects while exploring the site.");
}
struct Found {Json link;std::string address,error;bool cancelled=false;};
Found probe(const std::string& address,const std::string& referrer,const GrabberFilters& filters,const Json& project,const Json& prefs,const Cancel& parent,const std::shared_ptr<HttpSession>& pool){
 Found result;result.address=address;Cancel cancel;cancel.parent=std::shared_ptr<Cancel>(const_cast<Cancel*>(&parent),[](Cancel*){});cancel.deadline=GetTickCount64()+15000;
 try{auto response=request(address,referrer,filters,project,prefs,cancel,pool,true,true,false);bool fallback=response.response->status==403||response.response->status==405||response.response->status==501;Json info;if(!fallback){info=metadata(*response.response,response.address,false);fallback=num(info,"Size",-1)<0;}
  if(fallback){
   auto initialInfo=info;response.response.reset();response=request(address,referrer,filters,project,prefs,cancel,pool,true,false,true);info=metadata(*response.response,response.address,true);
   if(initialInfo.is_object()&&str(initialInfo,"Url")==str(info,"Url")){if(response.response->header(L"Content-Disposition").empty())info["FileName"]=initialInfo["FileName"];if(str(info,"ContentType").empty())info["ContentType"]=initialInfo["ContentType"];}
  }info["Referrer"]=referrer;info["DiscoveredUrl"]=address;result.link=std::move(info);
 }catch(const Cancelled&){result.cancelled=parent.cancelled();if(!result.cancelled)result.error="File metadata lookup timed out.";}catch(const std::exception& e){result.error=e.what();}return result;
}
}
Json exploreWithUpdates(const Json& project,const Json& prefs,const Cancel& cancel,std::function<void(std::string)> report,GrabberCheckpoint checkpoint){
 validateGrabberActions(project);grabberLogin(project);grabberBrowserSession(project);auto sessionPrefs=browserSessionPreferences(prefs,project);
 GrabberFilters filters(project);auto signature=filters.signature()+(yes(project,"ConvertLinks")?"|links:1":"")+(yes(project,"UseLinkDescriptions",true)?"":"|descriptions:0")+(str(project,"ProtectedLogin").empty()?"":"|login:"+str(project,"ProtectedLogin")),browserSignature=str(project,"ProtectedBrowserSession");if(yes(project,"BrowserLogin"))signature+="|browser:"+browserSignature;auto start=grabberCanonical(filters.source.full);auto saved=project.value("ExploreState",Json::object());
 bool resume=str(saved,"Signature")==signature&&saved.contains("Pending")&&saved["Pending"].is_array()&&!saved["Pending"].empty();Json links=resume?project.value("Links",Json::array()):Json::array(),errors=resume?project.value("Errors",Json::array()):Json::array();
 std::map<std::string,std::string> parents;std::deque<Json> pending;std::set<std::string> visited,scheduled,considered,addresses,duplicateKeys;
 if(resume){
  auto invalid=[] {throw std::runtime_error("Saved exploration state is invalid. Start a new exploration.");};
  if(!links.is_array()||links.size()>2000||!errors.is_array()||errors.size()>1000)invalid();
  for(const char* key:{"Pending","Visited","Considered"})if(!saved.contains(key)||!saved[key].is_array()||saved[key].size()>10000)invalid();
  for(const auto& entry:saved["Pending"]){
   if(!entry.is_object()||!entry.contains("Url")||!entry["Url"].is_string()||!entry.contains("Depth")||!entry["Depth"].is_number_integer()||!entry.contains("ExternalDepth")||!entry["ExternalDepth"].is_number_integer()||num(entry,"Depth")<0||num(entry,"Depth")>filters.depth||num(entry,"ExternalDepth")<0||num(entry,"ExternalDepth")>filters.externalDepth)invalid();
   if(entry.contains("Description")&&(!entry["Description"].is_string()||str(entry,"Description").size()>1024))invalid();
   if(entry.contains("First")&&!entry["First"].is_boolean())invalid();auto url=grabberCanonical(str(entry,"Url"));
   if(yes(entry,"First")&&(url!=start||num(entry,"Depth")||num(entry,"ExternalDepth")))invalid();
   if(!filters.pageAllowed(url,yes(entry,"First")))invalid();if(!str(entry,"Referrer").empty())grabberCanonical(str(entry,"Referrer"));pending.push_back(entry);
  }
  for(const auto& entry:saved["Visited"]){if(!entry.is_string())invalid();visited.insert(grabberCanonical(entry.get<std::string>()));}
  for(const auto& entry:saved["Considered"]){if(!entry.is_string())invalid();considered.insert(grabberCanonical(entry.get<std::string>()));}
  if(saved.contains("Parents")){if(!saved["Parents"].is_object()||saved["Parents"].size()>10000)invalid();for(auto it=saved["Parents"].begin();it!=saved["Parents"].end();++it){if(!it->is_string())invalid();parents[grabberCanonical(it.key())]=it->get<std::string>().empty()?"":grabberCanonical(it->get<std::string>());}}
  for(const auto& entry:links){if(!entry.is_object()||!entry.contains("Url")||!entry["Url"].is_string())invalid();if(entry.contains("Description")&&(!entry["Description"].is_string()||str(entry,"Description").size()>1024))invalid();grabberCanonical(str(entry,"Url"));}
 }else pending.push_back({{"Url",start},{"Depth",0},{"ExternalDepth",0},{"First",true}});
 scheduled=visited;for(const auto& entry:pending)scheduled.insert(str(entry,"Url"));for(const auto& entry:links){addresses.insert(str(entry,"Url"));addresses.insert(str(entry,"DiscoveredUrl",str(entry,"Url")));if(num(entry,"Size",-1)>=0)duplicateKeys.insert(lower(str(entry,"FileName",grabberFileName(str(entry,"Url"))))+"\n"+std::to_string(num(entry,"Size")));}
 i64 filtered=resume?std::clamp<i64>(num(saved,"Filtered"),0,100000):0,duplicates=resume?std::clamp<i64>(num(saved,"Duplicates"),0,100000):0;bool stopped=false,limited=false;auto pool=std::make_shared<HttpSession>(sessionPrefs);std::string protectedSession=browserSignature;Json persistedSession=pool->browserSessionSnapshot();
 struct CheckpointFailure:std::runtime_error{using std::runtime_error::runtime_error;};
 auto lastUpdate=GetTickCount64();size_t lastLinks=0,lastPages=0;
 auto snapshot=[&](bool final){auto output=project;auto session=pool->browserSessionSnapshot();if(!session.empty()&&session!=persistedSession){protectedSession=protect(session.dump());persistedSession=session;}if(!session.empty())output["ProtectedBrowserSession"]=protectedSession;auto checkpointSignature=signature;if(yes(project,"BrowserLogin")&&protectedSession!=browserSignature)checkpointSignature=signature.substr(0,signature.size()-browserSignature.size())+protectedSession;output["Links"]=links;output["PageParents"]=Json(parents);output["Errors"]=errors;output["LastExplored"]=date();output["PagesVisited"]=visited.size();output["ExplorationRunning"]=!final;output["ExplorationStopped"]=stopped;output["ExplorationLimited"]=limited;output["ExplorationLimit"]=links.size()>=2000?"files":limited&&visited.size()>=(size_t)filters.maxPages?"pages":limited?"candidates":"";output["FilteredFiles"]=filtered;output["HiddenDuplicates"]=duplicates;output["ExploreState"]={{"Signature",checkpointSignature},{"Pending",Json(pending)},{"Visited",Json(visited)},{"Considered",Json(considered)},{"Filtered",filtered},{"Duplicates",duplicates},{"Parents",Json(parents)}};return output;};
 auto publish=[&](bool force,bool final=false){if(!checkpoint)return;if(!force&&GetTickCount64()-lastUpdate<250&&(lastLinks||links.empty()))return;if(!force&&links.size()==lastLinks&&visited.size()==lastPages)return;auto output=snapshot(final);try{checkpoint(output);}catch(const std::exception& e){throw CheckpointFailure(std::string("Unable to save Grabber progress: ")+e.what());}
  std::map<std::string,Json> updates;for(const auto& link:output["Links"])updates[str(link,"Url")]=link;for(auto& link:links){auto found=updates.find(str(link,"Url"));if(found!=updates.end())for(const char* key:{"Selected","SaveName","DownloadId"})if(found->second.contains(key))link[key]=found->second[key];}lastUpdate=GetTickCount64();lastLinks=links.size();lastPages=visited.size();};
 auto error=[&](const std::string& url,const std::string& message){if(errors.size()<1000)errors.push_back({{"Url",url},{"Message",message}});};
 auto accept=[&](Json info,bool page){const auto url=str(info,"Url"),original=str(info,"DiscoveredUrl",url);if(addresses.count(url)||addresses.count(original))return;if(!filters.fileLocationAllowed(url)||!filters.nameAllowed(original,str(info,"FileName"),page)||!filters.sizeAllowed(num(info,"Size",-1))){++filtered;if((filters.minimum||filters.maximum)&&num(info,"Size",-1)<0)error(original,"Size is unavailable; this file cannot satisfy the configured size limits.");return;}
  auto size=num(info,"Size",-1);auto duplicate=lower(str(info,"FileName"))+"\n"+std::to_string(size);if(filters.hideDuplicates&&size>=0&&!duplicateKeys.insert(duplicate).second){++duplicates;return;}addresses.insert(url);addresses.insert(original);links.push_back(std::move(info));};
 try{while(!pending.empty()&&visited.size()<(size_t)filters.maxPages&&links.size()<2000){
  cancel.check();auto task=pending.front();auto address=grabberCanonical(str(task,"Url"));if(visited.count(address)){pending.pop_front();continue;}const int level=(int)num(task,"Depth"),external=(int)num(task,"ExternalDepth");bool finished=false;
  if(report)report("Exploring page "+std::to_string(visited.size()+1)+" / "+std::to_string(filters.maxPages)+"; "+std::to_string(links.size())+" files found.");
  try{
   Cancel pageCancel;pageCancel.parent=std::shared_ptr<Cancel>(const_cast<Cancel*>(&cancel),[](Cancel*){});pageCancel.deadline=GetTickCount64()+30000;
   auto fetched=request(address,str(task,"Referrer"),filters,project,sessionPrefs,pageCancel,pool,false,false,false,yes(task,"First"));auto& response=*fetched.response;if(parents.size()<10000)parents[fetched.address]=str(task,"Referrer");auto type=previewMimeType(response.header(L"Content-Type"));if(response.status<200||response.status>=300)throw HttpRejected(response.status,response.header(L"Retry-After"));
   const bool css=type=="text/css"||(type.empty()&&fs::path(wide(lower(Url(fetched.address).path))).extension()==L".css");const bool html=type.empty()||type=="text/html"||type=="application/xhtml+xml";
   auto info=metadata(response,fetched.address,false);info["DiscoveredUrl"]=address;info["Referrer"]=str(task,"Referrer");info["Description"]=str(task,"Description");accept(info,html||css);
   if(html||css){auto data=response.all(2*1024*1024,pageCancel);std::string body(data.begin(),data.end()),base=fetched.address;auto references=siteReferences(body,css,yes(project,"ConvertLinks"));if(!css)for(const auto& ref:references)if(ref.remove&&!ref.value.empty()){try{base=grabberCanonical(combineUrl(fetched.address,ref.value));}catch(...){}break;}
    std::vector<std::string> candidates;std::map<std::string,std::string> descriptions;std::set<std::string> candidateSet;for(const auto& ref:references){if(ref.remove||(ref.navigation&&!ref.page)||ref.value.empty()||ref.value[0]=='#')continue;try{auto url=grabberCanonical(combineUrl(base,ref.value));Url parsed(url);if(yes(project,"UseLinkDescriptions",true)&&descriptions[url].empty())descriptions[url]=ref.description;const bool page=(ref.page&&pageAddress(url))||fs::path(wide(lower(parsed.path))).extension()==L".css";
      bool exploring=false;if(!descriptions[url].empty()){for(auto& waiting:pending)if(str(waiting,"Url")==url&&str(waiting,"Description").empty())waiting["Description"]=descriptions[url];for(auto& existing:links)if((str(existing,"Url")==url||str(existing,"DiscoveredUrl")==url)&&str(existing,"Description").empty())existing["Description"]=descriptions[url];}if(page&&filters.pageAllowed(url)){
       bool local=filters.currentSite(parsed);int childLevel=local?level+1:level,childExternal=local?0:external+1;
       if(local?childLevel<=filters.depth:(!filters.pagesSame&&childExternal<=filters.externalDepth)){exploring=true;if(!scheduled.count(url)){if(scheduled.size()>=10000){limited=true;break;}pending.push_back({{"Url",url},{"Depth",childLevel},{"ExternalDepth",childExternal},{"Referrer",fetched.address},{"Description",descriptions[url]}});scheduled.insert(url);}}
      }
      if(!exploring&&filters.fileLocationAllowed(url)&&filters.nameAllowed(url,grabberFileName(url),page)&&!considered.count(url)&&!addresses.count(url)&&!candidateSet.count(url)){if(considered.size()+candidates.size()>=10000){limited=true;break;}if(candidateSet.insert(url).second)candidates.push_back(url);}
     }catch(const std::exception&){} }
    // Metadata requests overlap, but results are applied in document order so
    // duplicate hiding consistently keeps the first discovered copy.
    for(size_t at=0,width=(size_t)num(project,"MetadataParallel",4);at<candidates.size()&&links.size()<2000;at+=width){cancel.check();std::vector<std::future<Found>> workers;std::set<std::string> batch;
     for(size_t i=at;i<std::min(candidates.size(),at+width);++i)if(!considered.count(candidates[i])&&batch.insert(candidates[i]).second){auto url=candidates[i];workers.push_back(std::async(std::launch::async,[&,url,referrer=fetched.address]{return probe(url,referrer,filters,project,sessionPrefs,cancel,pool);}));}
     for(auto& worker:workers){auto found=worker.get();if(found.cancelled){stopped=true;continue;}considered.insert(found.address);if(!found.error.empty())error(found.address,found.error);else if(links.size()<2000){found.link["Description"]=descriptions[found.address];accept(std::move(found.link),false);}}publish(false);if(stopped)throw Cancelled();
    }
   }finished=links.size()<2000&&!limited;if(!finished)limited=true;
  }catch(const CheckpointFailure&){throw;}catch(const Cancelled&){if(cancel.cancelled())throw;error(address,"Page lookup timed out.");finished=true;}catch(const std::exception& e){error(address,e.what());finished=true;}
  if(finished){visited.insert(address);pending.pop_front();}publish(false);if(limited)break;
 }}catch(const Cancelled&){stopped=true;}
 limited=limited||(!pending.empty()&&(visited.size()>=(size_t)filters.maxPages||links.size()>=2000));publish(true,true);return snapshot(true);
}
Json explore(const Json& project,const Json& prefs,const Cancel& cancel,std::function<void(std::string)> report){return exploreWithUpdates(project,prefs,cancel,std::move(report),{});}
Json runGrabber(Manager& manager,const Json& input,const Cancel& cancel,std::function<void(std::string)> report,std::function<void(const Json&)> update){
 auto project=input;Json prefs;{Lock lock(manager.mutex);validateGrabberActions(project);project["GrabberRunId"]=guid();manager.saveProject(project);prefs=manager.state["Settings"];}
 auto checkpoint=[&](Json& output){
  {Lock lock(manager.mutex);auto saved=grabberProject(manager,str(project,"Id"));if(str(saved,"GrabberRunId")!=str(project,"GrabberRunId"))throw std::runtime_error("Another exploration replaced this project.");
   output["DownloadWhileExploring"]=yes(saved,"DownloadWhileExploring");for(const char* field:{"SaveCategory","DownloadQueue"})if(saved.contains(field))output[field]=saved[field];std::map<std::string,Json> current;for(const auto& link:saved["Links"])current[str(link,"Url")]=link;
   for(auto& link:output["Links"]){auto found=current.find(str(link,"Url"));if(found!=current.end())for(const char* key:{"Selected","SaveName","DownloadId"})if(found->second.contains(key))link[key]=found->second[key];}
   if(yes(output,"DownloadWhileExploring"))manager.addProject(output,str(output,"DownloadQueue","Main queue"),false,true,false);else manager.saveProject(output);
   output=grabberProject(manager,str(project,"Id"));
  }if(update)update(output);
 };
 exploreWithUpdates(project,prefs,cancel,std::move(report),checkpoint);return grabberProject(manager,str(project,"Id"));
}
}
