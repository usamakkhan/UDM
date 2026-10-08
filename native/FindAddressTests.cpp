#include "GuiModels.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;Json checks=Json::array();bool passed=true;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});passed &= ok;};
 Json direct={{"Url","https://origin.example/file"},{"DownloadPage","https://page.example/watch"},{"ProtectedResolvedUrl",protect("https://cdn.example/redirected.bin")}};
 Json media={{"SourceUrl","https://page.example/watch"},{"Video",{{"Url","https://video.example/track"}}},{"Audio",{{"Url","https://audio.example/track"}}}};
 auto query=[](std::string text){return Json{{"Text",text},{"FileName",false},{"Address",true}};};
 check(matchesDownload(direct,query("origin.example")),"Original URL remains searchable");
 check(matchesDownload(direct,query("page.example")),"Parent page remains searchable");
 check(matchesDownload(direct,query("cdn.example")),"Properties redirected address is searchable");
 check(matchesDownload(media,query("video.example")),"Properties video stream is searchable");
 check(matchesDownload(media,query("audio.example")),"Properties audio stream is searchable");
 Json plan={{"manifestUrl","https://manifest.example/main.mpd"},{"tracks",Json::array({Json{{"kind","video"},{"segments",Json::array({Json{{"url","https://segment.example/first"}}})}}})}};
 Json adaptive={{"ProtectedAdaptive",protect(plan.dump())}};
 check(matchesDownload(adaptive,query("manifest.example")),"Properties manifest is searchable");
 check(matchesDownload(adaptive,query("segment.example")),"Properties first segment is searchable");
 Json sabr={{"ProtectedSabr",protect(Json{{"url","https://sabr.example/endpoint"}}.dump())}};
 check(matchesDownload(sabr,query("sabr.example")),"Properties SABR endpoint is searchable");
 auto q=query("VIDEO.EXAMPLE");check(matchesDownload(media,q),"Stream matching ignores case by default");q["MatchCase"]=true;check(!matchesDownload(media,q),"Stream matching honors case selection");
 q=query("video.example");q["WholeString"]=true;check(!matchesDownload(media,q),"Whole-string does not match a stream fragment");q["Text"]="https://video.example/track";check(matchesDownload(media,q),"Whole-string matches complete stream URL");q["Address"]=false;check(!matchesDownload(media,q),"Unchecked Address excludes stream URLs");
 Json livePlan={{"live",true},{"tracks",Json::array()}};
 for(const std::string kind:{"video","audio","subtitle"})livePlan["tracks"].push_back({{"kind",kind},{"playlist","https://"+kind+".example/live.m3u8"},{"segments",Json::array({{{"url","https://"+kind+".example/first"}}})}});
 Json live={{"ProtectedAdaptive",protect(livePlan.dump())}};auto liveBefore=live;auto links=downloadLinks(live);
 check(links.size()==6,"Live properties expose each selected track playlist and segment");
 check(links[0].label=="Live video playlist"&&links[1].label=="Live audio playlist"&&links[2].label=="Live subtitle playlist","Live properties distinguish video, audio and subtitle playlist labels");
 check(links[3].label=="First video segment"&&links[4].label=="First audio segment"&&links[5].label=="First subtitle segment","Live properties distinguish video, audio and subtitle segment labels");
 check(matchesDownload(live,query("subtitle.example/live.m3u8"))&&matchesDownload(live,query("subtitle.example/first")),"Both subtitle playlist and segment URLs remain searchable");
 check(live==liveBefore&&downloadAddress(live)=="https://video.example/live.m3u8","Viewing live links preserves protected metadata and the primary video address");
 auto before=media;matchesDownload(media,query("audio.example"));check(media==before,"Search leaves captured download metadata unchanged");
 atomicText(argv[1],Json{{"passed",passed},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; passed="<<passed<<"\n";return passed?0:1;
}