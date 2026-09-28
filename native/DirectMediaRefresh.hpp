#pragma once
#include "Core.hpp"
#include "BrowserRequest.hpp"
#include <regex>
namespace udm {
inline bool directMediaJob(JobPtr job){return job&&!str(job->data,"SourceUrl").empty()&&str(job->data,"ProtectedSabr").empty()&&str(job->data,"ProtectedAdaptive").empty()&&job->video&&str(job->data,"MediaOutput")!="audio";}
inline std::string directMediaPageId(const std::string& value){
 validateSource(value);Url url(value);std::string id;auto params=query(value);
 if(url.host=="youtu.be")id=url.path.substr(1);
 else if(url.path=="/watch"&&params.count("v"))id=params["v"];
 else {std::smatch match;if(std::regex_match(url.path,match,std::regex("/(?:shorts|embed|live)/([A-Za-z0-9_-]{11})/?")))id=match[1];}
 if(!std::regex_match(id,std::regex("[A-Za-z0-9_-]{11}")))throw std::runtime_error("The original YouTube video identity is missing.");return id;
}
inline void validateDirectMediaTrack(const std::string& old,const std::string& fresh){
 validateStream(old);validateStream(fresh);if(fresh.size()>16384)throw std::runtime_error("Media URL is too long.");
 auto before=query(old),after=query(fresh);
 // CDN host, signature, session ID and expiry can rotate. Captured content
 // metadata cannot change; HTTP resume validation is still required on wire.
 for(const auto* key:{"itag","lmt","clen","mime","dur","xtags"})if(before.count(key)&&after.count(key)&&before[key]!=after[key])throw std::runtime_error("The captured stream format, revision or language changed. Saved data was left unchanged.");
 for(const auto* key:{"range","sq","sabr"})if(after.count(key))throw std::runtime_error("Choose a full direct playback stream for this download.");
 if(after.count("expire")&&(!std::regex_match(after["expire"],std::regex("[0-9]{1,12}"))||std::stoll(after["expire"])<epoch()/1000+120))throw std::runtime_error("The replacement playback link is expired or about to expire. Refresh the video panel again.");
}
inline Headers validateDirectMediaRefresh(JobPtr job,const Json& message){
 if(!directMediaJob(job)||str(message,"action")!="media"||directMediaPageId(str(message,"url"))!=directMediaPageId(str(job->data,"SourceUrl")))throw std::runtime_error("Capture the original video again before refreshing this download.");
 if(num(message,"height")!=num(job->data,"MediaHeight")||num(message,"pixelHeight")!=num(job->data,"MediaPixelHeight")||str(message,"formatId")!=str(job->data,"MediaFormatId")||yes(message,"exactQuality")!=yes(job->data,"ExactMediaQuality")||(!str(message,"audioUrl").empty())!=bool(job->audio)||str(message,"output")=="audio")throw std::runtime_error("The captured quality or audio/video selection changed. Choose the original selection or start a new download.");
 validateDirectMediaTrack(str(job->video->data,"Url"),str(message,"videoUrl"));if(job->audio)validateDirectMediaTrack(str(job->audio->data,"Url"),str(message,"audioUrl"));
 auto format=query(str(message,"videoUrl"));if(format.count("itag")&&!str(message,"formatId").empty()&&format["itag"]!=str(message,"formatId"))throw std::runtime_error("The playback stream does not match the selected video format.");
 return browserHeaders(message);
}
}
