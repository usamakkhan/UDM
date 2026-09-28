#include "YouTubePlayer.hpp"
#include <regex>
#include <algorithm>
namespace udm {
static constexpr const char* playerAgent="Mozilla/5.0 (Macintosh; Intel Mac OS X 15_7_3) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.0 Safari/605.1.15";
Json youtubePlayerRequest(const Json& request){
 auto id=str(request,"videoId"),format=str(request,"formatId");
 if(!std::regex_match(id,std::regex("[A-Za-z0-9_-]{11}"))||!std::regex_match(format,std::regex("[0-9]{1,6}"))||num(request,"height")<144||num(request,"height")>4320)
  throw std::runtime_error("Invalid player video or selected format.");
 Json body={{"videoId",id},{"context",{{"client",{{"clientName","VISIONOS"},{"clientVersion","1.02"},{"userAgent",playerAgent},{"osName","visionOS"},{"osVersion","26.5.23O471"},{"deviceMake","Apple"},{"deviceModel","RealityDevice17,1"},{"hl","en"},{"timeZone","UTC"},{"utcOffsetMinutes",0}}}}},{"playbackContext",{{"contentPlaybackContext",{{"html5Preference","HTML5_PREF_WANTS"}}}}}};
 body["contentCheckOk"]=true;body["racyCheckOk"]=true;
 auto visitor=str(request,"visitorData");
 if(visitor.size()>2048||(!visitor.empty()&&!std::regex_match(visitor,std::regex("[A-Za-z0-9_=%+/-]+"))))throw std::runtime_error("Invalid player visitor context.");
 if(!visitor.empty())body["context"]["client"]["visitorData"]=visitor;
 auto timestamp=num(request,"signatureTimestamp");
 if(timestamp>0&&timestamp<1000000)body["playbackContext"]["contentPlaybackContext"]["signatureTimestamp"]=timestamp;
 return body;
}
static i64 positiveDecimal(const std::string& value){
 if(!std::regex_match(value,std::regex("[0-9]{1,15}")))throw std::runtime_error("Player stream size or identity is invalid.");
 auto number=std::stoll(value);if(number<=0)throw std::runtime_error("Player stream size or identity is invalid.");return number;
}
static Json playerStream(const Json& format,const std::string& kind,i64 now){
 if(format.contains("drmFamilies")&&!format["drmFamilies"].empty())throw std::runtime_error("Encrypted player format is unsupported.");
 if(format.contains("drmTrackType")||format.contains("signatureCipher")||format.contains("cipher"))throw std::runtime_error("This player format requires deciphering.");
 auto address=str(format,"url"),mime=lower(str(format,"mimeType"));
 if(address.empty()||address.size()>16000||mime.rfind(kind+"/mp4",0)!=0)throw std::runtime_error("No direct MP4 player link.");
 if(kind=="audio"&&mime.find("mp4a")==std::string::npos)throw std::runtime_error("Player audio is not AAC.");
 if(kind=="video"&&mime.find("avc1")==std::string::npos&&mime.find("av01")==std::string::npos)throw std::runtime_error("Player video codec is unsupported.");
 Url url(address);auto params=query(address);
 if(url.scheme!="https"||url.port!=443||!hostIs(url.host,"googlevideo.com")||url.path!="/videoplayback"||params.count("sabr")||params.count("sq")||params.count("ump"))throw std::runtime_error("Player response does not contain a standalone media link.");
 const auto itag=num(format,"itag"),size=positiveDecimal(str(format,"contentLength"));
 if(itag<1||itag>999999||params["itag"]!=std::to_string(itag)||positiveDecimal(params["expire"])*1000<now+120000)throw std::runtime_error("Player link identity or expiry is invalid.");
 if(params.count("clen")&&positiveDecimal(params["clen"])!=size)throw std::runtime_error("Player link length does not match its format.");
 // n-transformed URLs can appear syntactically complete but still be throttled.
 // Do not call them ready without an implementation of the player's transform.
 if(params.count("n"))throw std::runtime_error("This player link requires an n transform.");
 if(params.count("range")||params.count("rn")||params.count("rbuf"))throw std::runtime_error("Player link is scoped to a playback request.");
 return {{"url",address},{"itag",itag},{"size",size},{"mime",mime},{"kind",kind}};
}
Json youtubePlayerPair(const Json& response,const Json& request,i64 now){
 youtubePlayerRequest(request);if(!now)now=epoch();
 const auto details=response.value("videoDetails",Json::object()),status=response.value("playabilityStatus",Json::object());
 if(str(status,"status")!="OK")throw std::runtime_error("Player service did not authorize playback.");
 if(str(details,"videoId")!=str(request,"videoId"))throw std::runtime_error("Player response belongs to a different video.");
 if(yes(details,"isLiveContent")||yes(details,"isLive"))throw std::runtime_error("Live player retrieval is unsupported.");

 const auto formats=response.value("streamingData",Json::object()).value("adaptiveFormats",Json::array());
 if(!formats.is_array()||formats.size()>400)throw std::runtime_error("Player format list is invalid.");
 Json video,audio;std::string reason="Player response has no direct pair for the selected quality.";
 for(const auto& format:formats){
  if(!format.is_object())continue;
  auto mime=lower(str(format,"mimeType"));
  try{
   if(std::to_string(num(format,"itag"))==str(request,"formatId")){
    auto height=num(format,"height"),nominal=height;std::smatch match;auto label=str(format,"qualityLabel");if(std::regex_search(label,match,std::regex("^([0-9]+)p")))nominal=std::stoll(match[1]);
    if(nominal!=num(request,"height")||height<1||height>4320)throw std::runtime_error("Player format does not match the selected quality.");
    video=playerStream(format,"video",now);video["pixelHeight"]=height;
   }else if(mime.rfind("audio/mp4",0)==0&&yes(format.value("audioTrack",Json::object()),"audioIsDefault",true)){
    auto candidate=playerStream(format,"audio",now);if(audio.is_null()||num(candidate,"size")>num(audio,"size"))audio=std::move(candidate);
   }
  }catch(const std::exception& e){if(std::to_string(num(format,"itag"))==str(request,"formatId"))reason=e.what();}
 }
 if(video.is_null()||audio.is_null())throw std::runtime_error(reason);
 return {{"ok",true},{"videoId",str(request,"videoId")},{"formatId",str(request,"formatId")},{"height",num(request,"height")},{"pixelHeight",num(video,"pixelHeight")},{"video",video},{"audio",audio},{"userAgent",playerAgent},{"transport","player-direct"}};
}
void validatePlayerProbe(const Json& stream,DWORD status,const std::string& range,const std::string& type,size_t received,i64 offset){
 const auto size=num(stream,"size");if(size<1||offset<0||offset>=size)throw std::runtime_error("Player probe offset is outside the media file.");
 const auto expected="bytes "+std::to_string(offset)+"-"+std::to_string(offset)+"/"+std::to_string(size);
 if(status!=206||trim(range)!=expected||received!=1||lower(trim(type)).rfind(str(stream,"kind")+"/",0)!=0)throw std::runtime_error("Player link did not pass the media range check.");
}
Json retrieveYouTubePlayer(const Json& request,const Json& preferences){
 // Run in the short-lived native host, outside the UI and named-pipe listener.
 // This action has no download-list or filesystem side effects.
 const auto began=GetTickCount64();std::string phase="player-request";
 try{
 auto bodyText=youtubePlayerRequest(request).dump();Bytes body(bodyText.begin(),bodyText.end());
 Cancel cancel;cancel.deadline=GetTickCount64()+8000;
 Headers headers={{"User-Agent",playerAgent},{"Content-Type","application/json"},{"Accept","application/json"},{"Origin","https://www.youtube.com"}};
 auto pool=std::make_shared<HttpSession>(preferences);
 Http response("https://www.youtube.com/youtubei/v1/player?prettyPrint=false",headers,preferences,cancel,{}, {},"",&body,false,pool);
 if(response.status!=200)throw std::runtime_error("Player service returned HTTP "+std::to_string(response.status)+".");
 phase="player-response";auto data=response.all(2*1024*1024,cancel);auto parsed=Json::parse(data.begin(),data.end());
 Json pair;
 try{pair=youtubePlayerPair(parsed,request);}catch(const std::exception&){
  // Diagnostic fields are counts and enums only; never return a body excerpt.
  auto status=str(parsed.value("playabilityStatus",Json::object()),"status");if(!std::regex_match(status,std::regex("[A-Z_]{1,40}")))status="UNKNOWN";
  auto formats=parsed.value("streamingData",Json::object()).value("adaptiveFormats",Json::array());size_t direct=0,cipher=0,transform=0;
  if(formats.is_array())for(const auto& f:formats){if(!str(f,"url").empty()){++direct;try{if(query(str(f,"url")).count("n"))++transform;}catch(...){}}if(f.contains("signatureCipher")||f.contains("cipher"))++cipher;}
  return {{"ok",false},{"error",status=="OK"?"Player response has no verified direct pair for this selection.":"Player service did not authorize playback."},{"diagnostics",{{"client","VISIONOS"},{"playability",status},{"videoMatches",str(parsed.value("videoDetails",Json::object()),"videoId")==str(request,"videoId")},{"formats",formats.is_array()?formats.size():0},{"direct",direct},{"cipher",cipher},{"nTransform",transform}}}};
 }
 Headers mediaHeaders={{"User-Agent",playerAgent},{"Referer","https://www.youtube.com/"}};
 for(const char* kind:{"video","audio"}){
  auto stream=pair[kind];const auto size=num(stream,"size");std::set<i64> offsets={0,size/2,size-1};
  for(auto offset:offsets){
   phase=std::string(kind)+(offset==0?"-range-start":offset==size-1?"-range-end":"-range-middle");
   Http probe(str(stream,"url"),mediaHeaders,preferences,cancel,offset,offset,"",nullptr,false,pool);
   // A cached prefix can be readable while the rest returns 403. Require
   // successful bounded probes across the file before preferring direct URLs.
   // Never follow redirects or consume a full response as a probe.
   validatePlayerProbe(stream,probe.status,probe.header(L"Content-Range"),probe.header(L"Content-Type"),1,offset);
   auto byte=probe.all(1,cancel);validatePlayerProbe(stream,probe.status,probe.header(L"Content-Range"),probe.header(L"Content-Type"),byte.size(),offset);
  }
 }

 pair["validatedAt"]=epoch();pair["diagnostics"]={{"phase","complete"},{"elapsedMs",GetTickCount64()-began}};return pair;
 }catch(const Cancelled&){return {{"ok",false},{"error","Player retrieval timed out; browser capture remains available."},{"diagnostics",{{"phase",phase},{"elapsedMs",GetTickCount64()-began},{"timeout",true}}}};}
}
}
