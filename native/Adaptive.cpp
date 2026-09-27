#include "MediaStorage.hpp"
#include "BrowserRequest.hpp"
#include "Core.hpp"
#include "WebVtt.hpp"
#include <fstream>
#include <regex>
#include <future>
#include <algorithm>
namespace udm {
void validateAdaptive(const Json& plan){
 if(!plan.is_object()||plan.dump().size()>200000||(str(plan,"type")!="hls"&&str(plan,"type")!="dash"))throw std::runtime_error("Invalid or oversized streaming plan.");
 if(!plan.contains("tracks")||!plan["tracks"].is_array()||plan["tracks"].empty()||plan["tracks"].size()>3)throw std::runtime_error("Expected media tracks and an optional subtitle track.");
 if(num(plan,"height")<0||num(plan,"height")>8640||yes(plan,"encrypted"))throw std::runtime_error("Unsupported video dimensions or encrypted media.");
 if(plan.contains("audioOnly")&&!plan["audioOnly"].is_boolean())throw std::runtime_error("Invalid audio-only selection.");
 const bool audioOnly=yes(plan,"audioOnly");auto container=str(plan,"container","mp4");
 if((audioOnly&&(container!="m4a"||num(plan,"height")!=0||!yes(plan,"audioExpected")||plan["tracks"].size()!=1))||(!audioOnly&&container!="mp4"&&container!="ts"))throw std::runtime_error("Unsupported output container or track selection.");
 for(const char* field:{"audioName","audioLanguage"})if(plan.contains(field)&&!plan[field].is_string())throw std::runtime_error("Invalid selected audio description.");
 auto audioName=str(plan,"audioName"),audioLanguage=str(plan,"audioLanguage");
 if(audioName.size()>512||std::any_of(audioName.begin(),audioName.end(),[](unsigned char c){return c<32||c==127;})||audioLanguage.size()>63||(!audioLanguage.empty()&&!std::regex_match(audioLanguage,std::regex("[A-Za-z]{2,8}(-[A-Za-z0-9]{1,8})*"))))throw std::runtime_error("Invalid selected audio description.");
 if((!audioName.empty()||!audioLanguage.empty())&&!yes(plan,"audioExpected"))throw std::runtime_error("A selected audio track must require audio in the output.");
 for(const char* field:{"subtitleName","subtitleLanguage"})if(plan.contains(field)&&!plan[field].is_string())throw std::runtime_error("Invalid subtitle description.");
 auto subtitleName=str(plan,"subtitleName"),subtitleLanguage=str(plan,"subtitleLanguage");
 if(subtitleName.size()>512||std::any_of(subtitleName.begin(),subtitleName.end(),[](unsigned char c){return c<32||c==127;})||subtitleLanguage.size()>63||(!subtitleLanguage.empty()&&!std::regex_match(subtitleLanguage,std::regex("[A-Za-z]{2,8}(-[A-Za-z0-9]{1,8})*"))))throw std::runtime_error("Invalid subtitle description.");
 bool subtitle=false;size_t count=0;
 for(size_t t=0;t<plan["tracks"].size();++t){const auto& track=plan["tracks"][t];
  auto kind=str(track,"kind");bool validKind=t==0?kind==(audioOnly?"audio":"video"):((t==1&&kind=="audio")||(kind=="subtitle"&&!subtitle&&t+1==plan["tracks"].size()&&container=="mp4"));if(kind=="subtitle")subtitle=true;
  if(!validKind||!track.contains("segments")||!track["segments"].is_array()||track["segments"].empty())throw std::runtime_error("Invalid streaming track.");
  for(const auto& part:track["segments"]){if(++count>1200)throw std::runtime_error("This video exceeds the current 1200-segment limit.");if(part.contains("timeline")&&(!part["timeline"].is_number()||!std::isfinite(real(part,"timeline"))||real(part,"timeline")<0||real(part,"timeline")>7*86400))throw std::runtime_error("Invalid subtitle segment timeline.");auto address=str(part,"url");if(address.size()>16384)throw std::runtime_error("Media URL is too long.");Url u(address);if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("Streaming requires HTTP or HTTPS.");
   if(part.contains("start")||part.contains("length")){if(!part.contains("start")||!part.contains("length")||!part["start"].is_number_integer()||!part["length"].is_number_integer()||num(part,"start")<0||num(part,"length")<1||num(part,"length")>256LL*1024*1024||num(part,"start")>LLONG_MAX-num(part,"length"))throw std::runtime_error("Invalid streaming byte range.");}
  }
 }
 if(!subtitle&&(!subtitleName.empty()||!subtitleLanguage.empty()))throw std::runtime_error("Subtitle metadata requires a subtitle track.");
}
static std::string audioLanguageTag(std::string language){
 if(language.empty())return {};auto primary=lower(language.substr(0,language.find('-')));if(primary.size()==3)return primary;
 wchar_t iso[16]{};if(GetLocaleInfoEx(wide(language).c_str(),LOCALE_SISO639LANGNAME2,iso,16)||GetLocaleInfoEx(wide(primary).c_str(),LOCALE_SISO639LANGNAME2,iso,16)){auto result=lower(utf8(iso));if(result.size()==3)return result;}return {};
}
JobPtr receiveAdaptive(Manager& m,const Json& message){
 auto page=str(message,"url");Url source(page);if(source.scheme!="http"&&source.scheme!="https")throw std::runtime_error("Expected the video page URL.");
 if(!message.contains("plan"))throw std::runtime_error("Missing streaming plan.");auto plan=message["plan"];validateAdaptive(plan);
 if(!fs::exists(appDir()/L"tools"/L"ffmpeg.exe")||!fs::exists(appDir()/L"tools"/L"ffprobe.exe"))throw std::runtime_error("Run setup-media.ps1 to install the local media helpers.");
 Headers headers;for(auto pair:{std::pair<const char*,const char*>{"referrer","Referer"},{"userAgent","User-Agent"}}){auto value=str(message,pair.first);if(!value.empty())headers[pair.second]=value;}validateHeaders(headers);
 auto cookies=message.value("originCookies",Json::object());if(!cookies.is_object()||cookies.size()>20)throw std::runtime_error("Invalid media cookie scope.");
 for(auto it=cookies.begin();it!=cookies.end();++it){Url origin(it.key());if(origin.origin!=it.key()||!it.value().is_string()||it.value().get<std::string>().size()>16384)throw std::runtime_error("Invalid media cookie scope.");validateHeaders({{"Cookie",it.value().get<std::string>()}});}
 auto originHeaders=message.value("originHeaders",Json::object());
 if(!originHeaders.is_object()||originHeaders.size()>20||originHeaders.dump().size()>65536)throw std::runtime_error("Invalid media header scope.");
 std::set<std::string> origins;for(const auto& track:plan["tracks"])for(const auto& segment:track["segments"])origins.insert(Url(str(segment,"url")).origin);
 for(auto it=originHeaders.begin();it!=originHeaders.end();++it){if(!origins.count(it.key()))throw std::runtime_error("Media headers do not belong to this download.");browserHeaders({{"headers",it.value()}});}
 Lock lock(m.mutex);auto encoded=plan.dump();
 for(auto job:m.jobs)if(str(job->data,"Url")==page&&!str(job->data,"ProtectedAdaptive").empty()&&(m.isActive(job)||str(job->data,"Status")=="Awaiting confirmation"||str(job->data,"Status")=="Queued")&&reveal(str(job->data,"ProtectedAdaptive"))==encoded)return job;
 auto container=str(plan,"container","mp4");auto name=str(message,"filename","Video");name=std::regex_replace(name,std::regex("\\.(mp4|webm|mkv|ts|m4a)$",std::regex::icase),"")+"."+container;auto job=m.add(page,"",name,"Main queue",true,headers);
 try{job->data["ProtectedAdaptive"]=protect(encoded);job->data["ProtectedMediaCookies"]=protect(cookies.dump());job->data["ProtectedMediaHeaders"]=protect(originHeaders.dump());job->data["MediaPixelHeight"]=num(plan,"height");job->data["FormatDescription"]=std::string(str(plan,"type")=="hls"?"HLS":"DASH")+" to "+(container=="ts"?"TS":container=="m4a"?"M4A audio":"MP4")+" browser capture";if(!str(plan,"audioName").empty())job->data["FormatDescription"]=str(job->data,"FormatDescription")+"; audio: "+str(plan,"audioName")+(str(plan,"audioLanguage").empty()?"":" ("+str(plan,"audioLanguage")+")");if(!str(plan,"subtitleName").empty())job->data["FormatDescription"]=str(job->data,"FormatDescription")+"; subtitles: "+str(plan,"subtitleName")+(str(plan,"subtitleLanguage").empty()?"":" ("+str(plan,"subtitleLanguage")+")");job->data["Status"]=yes(m.state["Settings"],"SkipBrowserFileInfo")?"Queued":"Awaiting confirmation";m.save();return job;}
 catch(...){m.jobs.erase(std::remove(m.jobs.begin(),m.jobs.end(),job),m.jobs.end());m.save();throw;}
}
void adaptiveTransfer(Manager& m,JobPtr job,const std::shared_ptr<Cancel>& cancel){
 Json plan,prefs,cookies,originHeaders;Headers headers;{Lock lock(m.mutex);plan=Json::parse(reveal(str(job->data,"ProtectedAdaptive")));cookies=Json::parse(reveal(str(job->data,"ProtectedMediaCookies")));prefs=m.state["Settings"];headers=readHeaders(job->data);auto protectedOrigins=str(job->data,"ProtectedMediaHeaders");originHeaders=protectedOrigins.empty()?Json::object():Json::parse(reveal(protectedOrigins));}
 validateAdaptive(plan);auto session=std::make_shared<HttpSession>(prefs);auto folder=mediaWorkingDirectory(m,job)/L"adaptive";{Lock lock(m.mutex);m.save();}fs::create_directories(folder);fs::create_directories(job->target().parent_path());
 struct Part{Json info;fs::path path;size_t track=0;i64 limit=256LL*1024*1024;};std::vector<Part> parts;std::vector<std::vector<size_t>> tracks;
 for(size_t t=0;t<plan["tracks"].size();++t){tracks.emplace_back();for(auto info:plan["tracks"][t]["segments"]){tracks.back().push_back(parts.size());parts.push_back({info,folder/(std::to_wstring(parts.size())+L".part"),t,str(plan["tracks"][t],"kind")=="subtitle"?2LL*1024*1024:256LL*1024*1024});}}
 auto statePath=folder/L"completed.json";Json completed=Json::object();if(fs::exists(statePath))try{completed=Json::parse(readText(statePath));if(!completed.is_object())completed=Json::object();}catch(...){}
 std::atomic_size_t next{0};auto group=std::make_shared<Cancel>();group->parent=cancel;Rate rate;auto started=std::chrono::steady_clock::now();double priorTransfer,priorElapsed;int workerCount;
 {Lock lock(m.mutex);priorTransfer=real(job->data,"TransferSeconds");priorElapsed=real(job->data,"ElapsedSeconds");job->data["Size"]=-1;job->data["Received"]=0;job->data["AdaptiveTotalSegments"]=parts.size();job->data["AdaptiveCompletedSegments"]=0;workerCount=(int)std::min<size_t>(parts.size(),(size_t)std::clamp<i64>(num(job->data,"Connections",8),1,16));job->workers.assign(workerCount,{});}
 auto work=[&](int worker){try{for(;;){group->check();size_t index=next++;if(index>=parts.size())break;auto& part=parts[index];auto key=std::to_string(index);bool retained=false;
  {Lock lock(m.mutex);if(completed.contains(key)&&fs::exists(part.path)&&fs::file_size(part.path)==(uintmax_t)num(completed[key],"size")&&fileHash(part.path)==str(completed[key],"sha256"))retained=true;}
  if(retained){Lock lock(m.mutex);job->data["Received"]=num(job->data,"Received")+(i64)fs::file_size(part.path);job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;continue;}
  auto temp=part.path;temp+=L".tmp";int retries=m.retries(str(job->data,"Queue"));i64 written=0;
  for(int attempt=0;;++attempt){written=0;try{
   auto requestHeaders=headers;Url address(str(part.info,"url"));if(originHeaders.contains(address.origin)){auto captured=browserHeaders({{"headers",originHeaders[address.origin]}});for(const auto& entry:captured){for(auto it=requestHeaders.begin();it!=requestHeaders.end();)if(lower(it->first)==lower(entry.first))it=requestHeaders.erase(it);else ++it;requestHeaders[entry.first]=entry.second;}}if(cookies.contains(address.origin)&&cookies[address.origin].is_string()&&!cookies[address.origin].get<std::string>().empty())requestHeaders["Cookie"]=cookies[address.origin].get<std::string>();
   std::optional<i64> start,end;if(part.info.contains("start")){start=num(part.info,"start");end=*start+num(part.info,"length")-1;}
   Http response(address.full,requestHeaders,prefs,*group,start,end,"",nullptr,true,session);if(response.status<200||response.status>=300)throw std::runtime_error("Streaming server returned HTTP "+std::to_string(response.status)+".");
   if(!response.header(L"Content-Encoding").empty()&&lower(response.header(L"Content-Encoding"))!="identity")throw std::runtime_error("Unexpected encoded streaming response.");
   i64 expected=-1;auto length=response.header(L"Content-Length");if(!length.empty()){if(!std::regex_match(length,std::regex("[0-9]{1,18}")))throw std::runtime_error("Invalid segment size.");expected=std::stoll(length);}
   if(start){std::smatch match;auto cr=response.header(L"Content-Range");if(response.status!=206||!std::regex_match(cr,match,std::regex("bytes ([0-9]+)-([0-9]+)/([0-9]+)"))||std::stoll(match[1])!=*start||std::stoll(match[2])!=*end||std::stoll(match[3])<=*end||(expected>=0&&expected!=*end-*start+1))throw std::runtime_error("Streaming byte range was not honored.");expected=*end-*start+1;}else if(response.status==206)throw std::runtime_error("Unexpected partial streaming segment.");
   if(expected>part.limit)throw std::runtime_error("Streaming segment exceeds its size limit.");
   {Lock lock(m.mutex);job->workers[worker]={worker+1,0,expected>0?expected-1:-1,0,0,"Segment "+std::to_string(index+1)+" / "+std::to_string(parts.size())};}
   {std::ofstream out(temp,std::ios::binary|std::ios::trunc);if(!out)throw std::runtime_error("Cannot create streaming part.");char buffer[65536];for(;;){auto n=response.read(buffer,sizeof(buffer),*group);if(!n)break;if(written+(i64)n>part.limit||(expected>=0&&written+(i64)n>expected))throw std::runtime_error("Streaming segment length mismatch.");m.charge(n,*group,rate,job);out.write(buffer,n);if(!out)throw std::runtime_error("Cannot write streaming part.");written+=(i64)n;{Lock lock(m.mutex);job->data["Received"]=num(job->data,"Received")+(i64)n;job->data["TransferredBytes"]=num(job->data,"TransferredBytes")+(i64)n;auto& row=job->workers[worker];row.position=written;row.received=written;}}out.flush();if(!out)throw std::runtime_error("Cannot flush streaming part.");}
   if(written==0||(expected>=0&&written!=expected))throw std::runtime_error("Streaming segment was truncated.");group->check();
   if(!MoveFileExW(temp.c_str(),part.path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve streaming part.");
   auto partHash=fileHash(part.path);
   {Lock lock(m.mutex);completed[key]={{"size",written},{"sha256",partHash}};atomicText(statePath,completed.dump());job->data["AdaptiveCompletedSegments"]=num(job->data,"AdaptiveCompletedSegments")+1;job->workers[worker].state="Segment complete";}break;
  }catch(...){std::error_code ec;fs::remove(temp,ec);{Lock lock(m.mutex);job->data["Received"]=std::max<i64>(0,num(job->data,"Received")-written);}group->check();if(attempt>=retries)throw;group->wait(std::min(5000,300*(attempt+1)));}}
 }}catch(...){group->stop=true;throw;}};
 try{std::vector<std::future<void>> tasks;for(int i=0;i<workerCount;++i)tasks.push_back(std::async(std::launch::async,work,i));std::exception_ptr error;for(auto& task:tasks)try{task.get();}catch(const Cancelled&){if(!error)error=std::current_exception();}catch(...){error=std::current_exception();}cancel->check();if(error)std::rethrow_exception(error);
  {Lock lock(m.mutex);job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["Status"]="Merging";m.save();}
  std::vector<fs::path> inputs;int audioInput=-1,subtitleInput=-1;const bool audioOnly=yes(plan,"audioOnly");
  for(size_t t=0;t<tracks.size();++t){
   auto kind=str(plan["tracks"][t],"kind");auto path=folder/(std::to_wstring(t)+(kind=="subtitle"?L".vtt":L".bin"));inputs.push_back(path);
   if(kind=="audio")audioInput=(int)t;
   if(kind=="subtitle"){
    subtitleInput=(int)t;double mediaStart=0;
    if(str(plan,"type")=="hls"){
     auto timing=Json::parse(execute(appDir()/L"tools"/L"ffprobe.exe",{L"-v",L"error",L"-protocol_whitelist",L"file,pipe",L"-show_entries",L"format=start_time",L"-of",L"json",inputs[0].wstring()},20,*cancel));
     auto value=str(timing["format"],"start_time");if(!value.empty()){try{size_t used=0;mediaStart=std::stod(value,&used);if(used!=value.size()||!std::isfinite(mediaStart))throw std::runtime_error("Invalid media clock.");}catch(...){throw std::runtime_error("Cannot determine the subtitle synchronization clock.");}}
    }
    std::vector<SubtitleCue> cues;size_t subtitleBytes=0;
    for(auto index:tracks[t]){cancel->check();auto text=readText(parts[index].path);subtitleBytes+=text.size();if(subtitleBytes>16*1024*1024)throw std::runtime_error("Subtitle input exceeds 16 MB.");appendWebVtt(cues,text,mediaStart,real(parts[index].info,"timeline"),str(plan,"type")=="hls");}
    if(cues.empty())throw std::runtime_error("The selected subtitle track contains no usable cues.");atomicText(path,mergedWebVtt(std::move(cues)));continue;
   }
   std::ofstream out(path,std::ios::binary|std::ios::trunc);for(auto index:tracks[t]){std::ifstream input(parts[index].path,std::ios::binary);if(!input)throw std::runtime_error("A streaming part is missing.");char buffer[65536];while(input){cancel->check();input.read(buffer,sizeof(buffer));out.write(buffer,input.gcount());}if(!input.eof())throw std::runtime_error("Cannot read streaming part.");}out.flush();if(!out)throw std::runtime_error("Cannot assemble streaming track.");
  }
  auto tools=appDir()/L"tools",staging=job->target().parent_path()/(L".udm-"+wide(job->id())+L".adaptive."+wide(str(plan,"container","mp4")));auto mergeStart=std::chrono::steady_clock::now();
  try{std::vector<std::wstring> args={L"-hide_banner",L"-loglevel",L"error",L"-nostdin",L"-y"};
   for(const auto& input:inputs)args.insert(args.end(),{L"-protocol_whitelist",L"file,pipe",L"-i",input.wstring()});
   if(audioOnly)args.insert(args.end(),{L"-map",L"0:a:0"});
   else {args.insert(args.end(),{L"-map",L"0:v:0",L"-map",audioInput>=0?std::to_wstring(audioInput)+L":a:0":L"0:a:0?"});}
   args.insert(args.end(),{L"-c",L"copy"});
   if(subtitleInput>=0){args.insert(args.end(),{L"-map",std::to_wstring(subtitleInput)+L":s:0",L"-c:s",L"mov_text",L"-metadata:s:s:0",L"handler_name="+wide(str(plan,"subtitleName"))});auto language=audioLanguageTag(str(plan,"subtitleLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:s:0",L"language="+wide(language)});}
   if(!str(plan,"audioName").empty())args.insert(args.end(),{L"-metadata:s:a:0",L"title="+wide(str(plan,"audioName")),L"-metadata:s:a:0",L"handler_name="+wide(str(plan,"audioName"))});auto language=audioLanguageTag(str(plan,"audioLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:a:0",L"language="+wide(language)});if(str(plan,"container","mp4")=="ts")args.insert(args.end(),{L"-f",L"mpegts"});else args.insert(args.end(),{L"-movflags",L"+faststart"});args.push_back(staging.wstring());execute(tools/L"ffmpeg.exe",args,300,*cancel);
   auto probe=Json::parse(execute(tools/L"ffprobe.exe",{L"-v",L"error",L"-show_entries",L"stream=codec_type,height",L"-of",L"json",staging.wstring()},20,*cancel));bool video=false,audio=false,subtitle=false;for(const auto& stream:probe["streams"]){if(str(stream,"codec_type")=="video"){video=true;if(num(plan,"height")>0&&num(stream,"height")!=num(plan,"height"))throw std::runtime_error("Output dimensions differ from the selected quality.");}audio|=str(stream,"codec_type")=="audio";subtitle|=str(stream,"codec_type")=="subtitle";}
   if((audioOnly?video:!video)||(yes(plan,"audioExpected")&&!audio)||(subtitleInput>=0&&!subtitle))throw std::runtime_error("The output does not contain the selected media tracks.");auto digest=fileHash(staging);if(!str(job->data,"ExpectedSha256").empty()&&lower(digest)!=lower(str(job->data,"ExpectedSha256")))throw std::runtime_error("SHA-256 verification failed.");cancel->check();if(!MoveFileExW(staging.c_str(),job->target().c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish video; the destination may already exist.");markZone(job->target());
   {Lock lock(m.mutex);job->data["Sha256"]=digest;job->data["Size"]=fs::file_size(job->target());job->data["Received"]=job->data["Size"];job->data["MergeSeconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-mergeStart).count();job->data["Status"]="Complete";job->data["Finished"]=date();job->data["Error"]="";m.save();}
  }catch(...){std::error_code ec;fs::remove(staging,ec);throw;}
  for(auto& part:parts){std::error_code ec;fs::remove(part.path,ec);}for(auto& input:inputs){std::error_code ec;fs::remove(input,ec);}
 }catch(...){Lock lock(m.mutex);job->data["TransferSeconds"]=priorTransfer+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();job->data["ElapsedSeconds"]=priorElapsed+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();throw;}
 {Lock lock(m.mutex);job->data["ElapsedSeconds"]=priorElapsed+std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();}
}
}
