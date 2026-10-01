#pragma once
#include "YouTubePlayer.hpp"
class PlayerPostFixture {
 SOCKET listener=INVALID_SOCKET;std::thread server;
public:
 unsigned short port=0;std::string received;bool complete=false;
 PlayerPostFixture(){
  listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  if(bind(listener,(sockaddr*)&address,sizeof(address))||listen(listener,1))throw std::runtime_error("Player POST fixture failed.");int size=sizeof(address);getsockname(listener,(sockaddr*)&address,&size);port=ntohs(address.sin_port);
  server=std::thread([this]{fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{10,0};if(select(0,&set,nullptr,nullptr,&wait)<=0)return;auto client=accept(listener,nullptr,nullptr);if(client==INVALID_SOCKET)return;
   DWORD timeout=3000;setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));char buffer[4096];size_t required=0;
   for(;;){int n=recv(client,buffer,sizeof(buffer),0);if(n<=0)break;received.append(buffer,n);auto end=received.find("\r\n\r\n");if(end!=std::string::npos){std::smatch match;if(std::regex_search(received,match,std::regex("Content-Length: ([0-9]+)",std::regex::icase)))required=end+4+std::stoull(match[1]);if(required&&received.size()>=required){complete=true;break;}}if(received.size()>32768)break;}
   std::string response="HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}";::send(client,response.data(),(int)response.size(),0);shutdown(client,SD_BOTH);closesocket(client);
  });
 }
 void join(){if(server.joinable())server.join();}
 ~PlayerPostFixture(){join();closesocket(listener);}
 std::string url(){return "http://127.0.0.1:"+std::to_string(port)+"/player";}
};
static void playerChecks(){
 using namespace udm;const i64 now=1700000000000LL;
 Json request={{"videoId","abcdefghijk"},{"formatId","137"},{"height",1080},{"signatureTimestamp",20500}};
 auto body=youtubePlayerRequest(request);check(body["context"]["client"]["clientName"]=="VISIONOS"&&body["playbackContext"]["contentPlaybackContext"]["signatureTimestamp"]==20500&&body["contentCheckOk"]==true&&body["racyCheckOk"]==true&&body["context"]["client"]["osName"]=="visionOS"&&body["context"]["client"]["deviceModel"]=="RealityDevice17,1","Native player request carries selected video and bounded playback timestamp");
 auto visitor=request;visitor["visitorData"]="fixture-visitor%3D";check(youtubePlayerRequest(visitor)["context"]["client"]["visitorData"]=="fixture-visitor%3D","Current browser visitor context is carried to the matching player service");
 visitor["visitorData"]="bad\r\nInjected: value";rejects([&]{youtubePlayerRequest(visitor);},"Malformed visitor context is rejected");visitor["visitorData"]=std::string(2049,'x');rejects([&]{youtubePlayerRequest(visitor);},"Visitor context is bounded before network access");
 auto extra=request;extra["cookies"]="secret";extra["url"]="https://untrusted.invalid";check(youtubePlayerRequest(extra)==body,"Player request does not forward arbitrary endpoint or browser credentials");
 auto bad=request;bad["videoId"]="other";rejects([&]{youtubePlayerRequest(bad);},"Malformed player video identifier is rejected");bad=request;bad["height"]=9000;rejects([&]{youtubePlayerRequest(bad);},"Unsupported player selection is rejected");
 auto makeStream=[](int itag,const char* mime){return Json{{"itag",itag},{"mimeType",mime},{"contentLength","1000"},{"url","https://rr1.googlevideo.com/videoplayback?itag="+std::to_string(itag)+"&clen=1000&expire=4102444800&sig=fixture"}};};
 auto video=makeStream(137,"video/mp4; codecs=\"avc1.640028\"");video["height"]=816;video["qualityLabel"]="1080p";auto audio=makeStream(140,"audio/mp4; codecs=\"mp4a.40.2\"");
 Json response={{"videoDetails",{{"videoId","abcdefghijk"},{"lengthSeconds","120"}}},{"playabilityStatus",{{"status","OK"}}},{"streamingData",{{"adaptiveFormats",Json::array({video,audio})}}}};
 auto pair=youtubePlayerPair(response,request,now);check(pair["pixelHeight"]==816&&pair["height"]==1080&&pair["audio"]["itag"]==140,"Player pair preserves nominal quality and actual cropped pixel height");
 auto rejectResponse=[&](const std::function<void(Json&)>& alter,const char* name){auto changed=response;alter(changed);rejects([&]{youtubePlayerPair(changed,request,now);},name);};
 rejectResponse([](Json& j){j["videoDetails"]["videoId"]="advertvideo";},"Unrelated or advertisement response cannot supply player links");
 rejectResponse([](Json& j){j["videoDetails"]["isLiveContent"]=true;},"Live player response is rejected");
 rejectResponse([](Json& j){j["playabilityStatus"]["status"]="LOGIN_REQUIRED";},"Playback restriction is not treated as a usable HTTP 200 response");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][0]["qualityLabel"]="720p";},"Native player never silently substitutes another quality");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][0]["itag"]=136;},"Native player never silently substitutes another format");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][0]["signatureCipher"]="secret";},"Ciphered formats remain unavailable until deciphering is implemented");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][0]["drmFamilies"]={"WIDEVINE"};},"DRM media is excluded from player retrieval");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][1]["audioTrack"]["audioIsDefault"]=false;},"Non-default audio is not silently paired with video");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][1]["mimeType"]="audio/webm; codecs=opus";},"MP4 handoff does not select incompatible audio");
 auto rejectUrl=[&](std::string url,const char* name){rejectResponse([&](Json& j){j["streamingData"]["adaptiveFormats"][0]["url"]=url;},name);};
 auto original=str(video,"url");
 rejectUrl("https://googlevideo.com.evil.test/videoplayback?itag=137&expire=4102444800","Deceptive player media host is rejected");
 rejectUrl("http://rr1.googlevideo.com/videoplayback?itag=137&expire=4102444800","Unencrypted player media URL is rejected");
 rejectUrl(original+"&sabr=1","SABR playback endpoint is not relabelled as a direct file");
 rejectUrl(original+"&ump=1","UMP playback endpoint is not relabelled as a direct file");
 rejectUrl(original+"&sq=1","Individual playback segment is not relabelled as a full file");
 rejectUrl(original+"&n=requires-transform","Untransformed n parameter is not claimed ready or fast");
 rejectUrl(original+"&range=0-99","Playback-scoped range URL is rejected");
 rejectUrl("https://rr1.googlevideo.com/videoplayback?itag=137&expire=1","Expired player link is rejected");
 rejectUrl(original+"&clen=999","Player URL length must match its format metadata");
 rejectResponse([](Json& j){j["streamingData"]["adaptiveFormats"][0]["contentLength"]="0";},"Empty player format is rejected");
 validatePlayerProbe(pair["video"],206,"bytes 500-500/1000","video/mp4",1,500);check(true,"Middle-of-file player probe validates the exact offset");
 validatePlayerProbe(pair["audio"],206,"bytes "+std::to_string(num(pair["audio"],"size")-1)+"-"+std::to_string(num(pair["audio"],"size")-1)+"/"+std::to_string(num(pair["audio"],"size")),"audio/mp4",1,num(pair["audio"],"size")-1);check(true,"End-of-file audio probe validates the exact offset");
 rejects([&]{validatePlayerProbe(pair["video"],403,"","video/mp4",0,500);},"Readable prefix cannot authorize a rejected middle-of-file request");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 0-0/1000","video/mp4",1,500);},"Server repeating the first byte cannot pass a later probe");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 500-500/999","video/mp4",1,500);},"Later probes reject changed stream size");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 999-999/1000","video/mp4",0,999);},"Truncated final-byte probes cannot mark a stream ready");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 1000-1000/1000","video/mp4",1,1000);},"Out-of-bounds player probe offsets are rejected");
 validatePlayerProbe(pair["video"],206,"bytes 0-0/1000","video/mp4",1);check(true,"Exact byte-range media probe is accepted");
 rejects([&]{validatePlayerProbe(pair["video"],200,"bytes 0-0/1000","video/mp4",1);},"Server ignoring the range cannot enable preferred parallel transport");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 0-0/999","video/mp4",1);},"Changed media length fails validation");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 0-0/1000","text/html",1);},"HTML error body cannot pass as a media probe");
 rejects([&]{validatePlayerProbe(pair["video"],206,"bytes 0-0/1000","video/mp4",0);},"Truncated byte probe cannot pass validation");
 Cancel expired;expired.deadline=GetTickCount64()-1;rejects([&]{expired.check();},"Native retrieval deadline interrupts pending work");
 Fixture stall;Cancel deadline;deadline.deadline=GetTickCount64()+150;auto began=GetTickCount64();rejects([&]{Http hanging(stall.url("/stall-headers"),{},defaultSettings(),deadline);},"Native retrieval deadline cancels a stalled HTTP response");check(GetTickCount64()-began<2000,"Stalled native retrieval returns promptly for browser fallback");
 PlayerPostFixture fixture;auto text=body.dump();Bytes bytes(text.begin(),text.end());Cancel cancel;
 Http posted(fixture.url(),{{"Content-Type","application/json"},{"Accept","application/json"}},defaultSettings(),cancel,{}, {},"",&bytes,false);posted.all(20,cancel);fixture.join();
 check(fixture.complete&&fixture.received.substr(fixture.received.find("\r\n\r\n")+4)==text,"JSON player POST transfers its exact body");
 auto headers=lower(fixture.received.substr(0,fixture.received.find("\r\n\r\n")));
 check(headers.find("content-type: application/json")!=std::string::npos&&headers.find("application/x-protobuf")==std::string::npos&&headers.find("accept: application/json")!=std::string::npos,"JSON POST does not inherit SABR protobuf content headers");
}
