#include "GuiModels.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;Json checks=Json::array(),timing=Json::object();bool passed=true;std::string error;try{
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
 q=query("video.example");q["WholeString"]=true;check(matchesDownload(media,q),"Whole-string matches the complete stream site");q["Text"]="video";check(!matchesDownload(media,q),"Whole-string excludes site fragments");q["Text"]="https://video.example/track";check(matchesDownload(media,q),"Whole-string matches complete stream URL");q["Address"]=false;check(!matchesDownload(media,q),"Unchecked Address excludes stream URLs");
 q=query("PAGE.EXAMPLE");q["WholeString"]=true;check(matchesDownload(direct,q),"Whole-string site search covers the parent page with Windows case folding");q["MatchCase"]=true;check(!matchesDownload(direct,q),"Case-sensitive whole-site search preserves spelling");
 Json authority={{"Url","https://site.example:8443/file"}};q=query("site.example");q["WholeString"]=true;check(!matchesDownload(authority,q),"Whole-site comparison retains authority port");q["Text"]="site.example:8443";check(matchesDownload(authority,q),"Whole-site comparison accepts the full literal authority");
 // IDM 004b1560 and 0045fd40 use CharUpperBuffW for query and filename.
 check(!matchesDownload(Json{{"FileName","ς.bin"}},Json{{"Text","σ.bin"},{"WholeString",true}}),"Windows reference folding keeps final sigma distinct from ordinary sigma on this host");
 check(matchesDownload(Json{{"FileName","σ.bin"}},Json{{"Text","Σ.BIN"},{"WholeString",true}}),"Ordinary sigma matches its uppercase form through Windows reference folding");
 check(matchesDownload(Json{{"FileName","ς.bin"}},Json{{"Text","ς.BIN"},{"WholeString",true}}),"Final sigma still matches itself with an insensitive extension");
 checks.back()["observed"]={{"filenameFold",utf8(searchFold("ς.bin",false))},{"queryFold",utf8(findSearchNeedle(Json{{"Text","σ.bin"},{"WholeString",true}}))}};
 Json livePlan={{"live",true},{"tracks",Json::array()}};
 for(const std::string kind:{"video","audio","subtitle"})livePlan["tracks"].push_back({{"kind",kind},{"playlist","https://"+kind+".example/live.m3u8"},{"segments",Json::array({{{"url","https://"+kind+".example/first"}}})}});
 Json live={{"ProtectedAdaptive",protect(livePlan.dump())}};auto liveBefore=live;auto links=downloadLinks(live);
 check(links.size()==6,"Live properties expose each selected track playlist and segment");
 check(links[0].label=="Live video playlist"&&links[1].label=="Live audio playlist"&&links[2].label=="Live subtitle playlist","Live properties distinguish video, audio and subtitle playlist labels");
 check(links[3].label=="First video segment"&&links[4].label=="First audio segment"&&links[5].label=="First subtitle segment","Live properties distinguish video, audio and subtitle segment labels");
 check(matchesDownload(live,query("subtitle.example/live.m3u8"))&&matchesDownload(live,query("subtitle.example/first")),"Both subtitle playlist and segment URLs remain searchable");
 check(live==liveBefore&&downloadAddress(live)=="https://video.example/live.m3u8","Viewing live links preserves protected metadata and the primary video address");
 auto before=media;matchesDownload(media,query("audio.example"));check(media==before,"Search leaves captured download metadata unchanged");
 Json unreadableRedirect={{"Url","https://origin.example/file"},{"ProtectedResolvedUrl","not-a-valid-encrypted-context"}};
 check(downloadAddress(unreadableRedirect)=="https://origin.example/file","Primary display address does not decrypt an unused redirected address");
 const auto rejected=[&](const std::function<void()>& action){try{action();}catch(const std::exception&){return true;}return false;};
 check(rejected([&]{downloadLinks(unreadableRedirect);}),"Full Links still reports unreadable redirected context");
 check(downloadAddress(direct)=="https://origin.example/file"&&downloadLinks(direct).size()==2,"Healthy primary display and complete redirected Links retain their distinct roles");
 Json fallback={{"Url",""},{"ProtectedResolvedUrl",protect("https://cdn.example/fallback")}};
 check(downloadAddress(fallback)=="https://cdn.example/fallback","A missing requested address retains the resolved fallback");
 fallback["ProtectedResolvedUrl"]="not-a-valid-encrypted-context";
 check(rejected([&]{downloadAddress(fallback);}),"An unreadable selected fallback is still rejected");
 fallback=direct;fallback["Url"]="https://";
 check(rejected([&]{downloadAddress(fallback);}),"An invalid selected primary does not silently switch to a redirect");
 auto split=media;split["Audio"]["Url"]="https://";
 check(downloadAddress(split)=="https://video.example/track","A valid video display does not parse an unused invalid audio URL");
 check(rejected([&]{downloadLinks(split);}),"Full split Links still validates the secondary audio URL");
 split["Video"]["Url"]="";split["Audio"]["Url"]="https://audio.example/track";
 check(downloadAddress(split)=="https://audio.example/track","A missing video address retains the audio fallback");
 split["Video"]["Url"]="https://";
 check(rejected([&]{downloadAddress(split);}),"An invalid selected video remains an error instead of switching tracks");
 check(downloadAddress(Json{{"SourceUrl","https://page.example/watch"}}).empty(),"A page without stream URLs is not presented as the selected stream");
 auto projectedData=unreadableRedirect;projectedData["Id"]=guid();projectedData["FileName"]="projected.bin";
 Job projected(std::move(projectedData));projected.data["ProtectedResolvedUrl"]=std::string(4*1024*1024,'x');
 const auto primary=downloadAddressInputs(projected);
 check(primary.size()==1&&primary.contains("Url"),"Primary display snapshot excludes even a large unused redirect payload");
 projected.data["ProtectedResolvedUrl"]="changed-unused-context";
 check(downloadAddressInputsUnchanged(projected,primary),"Unused redirect changes do not invalidate a primary display snapshot");
 projected.data["Url"]="";
 check(!downloadAddressInputsUnchanged(projected,primary)&&downloadAddressInputs(projected).contains("ProtectedResolvedUrl"),"Removing the primary invalidates its snapshot and selects redirect context");
 projected.data=Json{{"Id",guid()},{"FileName","projected.bin"},{"SourceUrl","https://page.example/watch"}};
 projected.video=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","video.bin"},{"Url","https://video.example/track"}});projected.audio=std::make_shared<Job>(Json{{"Id",guid()},{"FileName","audio.bin"},{"Url","https://audio.example/track"}});
 const auto videoInputs=downloadAddressInputs(projected);
 check(videoInputs.size()==2&&videoInputs.contains("Video")&&!videoInputs.contains("Audio"),"Video display snapshot excludes secondary audio");
 projected.audio->data["Url"]="https://audio.example/changed";
 check(downloadAddressInputsUnchanged(projected,videoInputs),"Secondary audio changes do not invalidate selected video");
 projected.video.reset();const auto audioInputs=downloadAddressInputs(projected);
 check(!downloadAddressInputsUnchanged(projected,videoInputs)&&audioInputs.contains("Audio")&&!audioInputs.contains("Video"),"Removing selected video switches the display snapshot to audio");
 check(downloadAddress(audioInputs)=="https://audio.example/changed","Projected audio fallback resolves to its selected address");
 size_t matches=0;Json prior=Json::array(),current=Json::array();
 const auto measure=[&](bool old){auto start=std::chrono::steady_clock::now();for(int i=0;i<128;++i){std::string value;if(old){auto all=downloadLinks(direct);value=all.empty()?std::string{}:all.front().address;}else value=downloadAddress(direct);matches+=value=="https://origin.example/file";}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/128;};
 for(int trial=0;trial<7;++trial){if(trial%2){current.push_back(measure(false));prior.push_back(measure(true));}else{prior.push_back(measure(true));current.push_back(measure(false));}}
 check(matches==1792,"Both address algorithms return the same healthy requested URL in every timing iteration");
 timing={{"scope","Seven alternating trials of 128 direct-address lookups with a valid encrypted redirect. Prior all-link resolver code replay versus primary-only lookup; no whole-dialog, installed-app or IDM latency measurement."},{"priorMsPerLookup",prior},{"currentMsPerLookup",current}};
 }catch(const std::exception& e){passed=false;error=e.what();std::cerr<<error<<"\n";}
 const auto output=fs::path(argv[1]);const auto result=output.extension()==L".json"?output:output/L"results.json";
 atomicText(result,Json{{"passed",passed},{"error",error},{"checks",checks},{"timing",timing}}.dump(2),false);std::cout<<checks.size()<<" checks; passed="<<passed<<"\n";return passed?0:1;
}
