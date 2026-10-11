#include "YouTubePlayer.hpp"
#include <iostream>
using namespace udm;
int main(){int passed=0,failed=0;
 auto check=[&](bool ok,const char* name){(ok?++passed:++failed);std::cout<<(ok?"PASS ":"FAIL ")<<name<<std::endl;};
 Json request={{"videoId","abcdefghijk"},{"formatId","137"},{"height",1080}};
 auto stream=[](int itag,const char* mime){return Json{{"itag",itag},{"mimeType",mime},{"contentLength","1000"},{"url","https://rr1.googlevideo.com/videoplayback?itag="+std::to_string(itag)+"&clen=1000&expire=4102444800&sig=fixture"}};};
 auto video=stream(137,"video/mp4; codecs=\"avc1.640028\"");video["height"]=1080;
 auto audio=stream(140,"audio/mp4; codecs=\"mp4a.40.2\"");
 Json response={{"videoDetails",{{"videoId","abcdefghijk"}}},{"playabilityStatus",{{"status","OK"}}},{"streamingData",{{"adaptiveFormats",Json::array({video,audio})}}}};
 auto accepts=[&](const Json& value){try{return yes(youtubePlayerPair(value,request,1700000000000LL),"ok");}catch(...){return false;}};
 check(accepts(response),"Content video and audio pair remain valid");
 for(const auto& marker:{"advideo=1","advideo=0","el=adunit","el=adunit&el=detailpage","el=detailpage&el=adunit","advideo=1&advideo=","%61dvideo=1","el=%61dunit"})for(int track:{0,1}){
  auto changed=response;auto& selected=changed["streamingData"]["adaptiveFormats"][track];selected["url"]=str(selected,"url")+"&"+marker;
  check(!accepts(changed),track?"Ad-marked audio cannot enter native fallback":"Ad-marked video cannot enter native fallback");
 }
 for(const auto& marker:{"advideo=","el=detailpage","el=adunit-extra","title=adunit","redirect=https%3A%2F%2Fexample.test%2F%3Fel%3Dadunit"}){
  auto changed=response;auto& selected=changed["streamingData"]["adaptiveFormats"][0];selected["url"]=str(selected,"url")+"&"+marker;
  check(accepts(changed),"Unrelated query values preserve content playback");
 }
 std::cout<<passed<<" passed, "<<failed<<" failed"<<std::endl;return failed?1:0;
}
