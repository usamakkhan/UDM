#pragma once
#include "AuthenticationFixture.hpp"
#include "SiteLogins.hpp"
static void authenticationChecks(const udm::fs::path& root){
 using namespace udm;AuthenticationFixture server;Manager manager(root/L"authentication-state");auto settings=manager.state["Settings"];settings["DownloadFolder"]=utf8((root/L"authentication-files").wstring());settings["CategoryFolders"]=false;settings["Connections"]=4;settings["Retries"]=0;manager.setSettings(settings);
 auto runUntilStopped=[&](JobPtr job){manager.resume(job);auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);for(;;){manager.tick();{Lock lock(manager.mutex);if(!manager.isActive(job)&&str(job->data,"Status")!="Queued")return;}if(std::chrono::steady_clock::now()>deadline)throw std::runtime_error("Authentication workflow did not finish.");Sleep(10);}};
 auto basic=manager.add(server.url("/basic"),"","basic.bin");runUntilStopped(basic);
 check(str(basic->data,"Status")=="Failed"&&canRequestLogin(basic->data)&&yes(basic->data,"AuthenticationPromptPending"),"HTTP Basic challenge requests a login for the actual server");
 check(str(basic->data,"Error").find("expired")==std::string::npos,"HTTP 401 is distinguished from an expired download link");
 manager.setDownloadLogin(basic,"u","wrong");runUntilStopped(basic);check(str(basic->data,"Status")=="Failed"&&yes(basic->data,"AuthenticationPromptPending")&&!fs::exists(basic->target()),"Incorrect credentials do not publish a file or cause an automatic retry loop");
 manager.setDownloadLogin(basic,"u","p:2");runUntilStopped(basic);check(str(basic->data,"Status")=="Complete"&&readText(basic->target())==server.payload,"Login and retry downloads exact bytes through parallel authenticated requests");
 check(str(basic->data,"ProtectedHeaders").find("p:2")==std::string::npos&&basicLogin(readHeaders(basic->data)).second=="p:2","Per-download password is protected at rest and preserves colons");
 auto digest=manager.add(server.url("/digest"),"","digest.bin");manager.setDownloadLogin(digest,"u","p:2");runUntilStopped(digest);
 check(str(digest->data,"Status")=="Complete"&&readText(digest->target())==server.payload&&server.digestHeaders>=2,"WinHTTP Digest challenge-response is cryptographically verified by the server for probe and workers");
 auto denied=manager.add(server.url("/bad-digest"),"","denied-digest.bin");manager.setDownloadLogin(denied,"u","p:2");auto requests=server.requests.load();runUntilStopped(denied);
 check(str(denied->data,"Status")=="Failed"&&canRequestLogin(denied->data)&&server.requests-requests==2,"Rejected Digest credentials stop after one challenge retry");
 auto cross=manager.add(server.url("/cross"),"","cross-origin.bin");manager.setDownloadLogin(cross,"u","p:2");runUntilStopped(cross);
 check(str(cross->data,"Status")=="Failed"&&num(cross->data,"LastHttpStatus")==401&&!canRequestLogin(cross->data),"Cross-origin redirect does not forward credentials or mislabel the login destination");
 rejects([&]{manager.setDownloadLogin(cross,"u","p:2");},"Login editor rejects a challenge for a different origin");
 auto same=manager.add(server.url("/same"),"","same-origin.bin");manager.setDownloadLogin(same,"u","p:2");runUntilStopped(same);
 check(str(same->data,"Status")=="Complete"&&readText(same->target())==server.payload&&reveal(str(same->data,"ProtectedResolvedUrl"))==server.url("/basic"),"Same-origin redirects retain authentication and record the resolved address privately");
 auto bearer=manager.add(server.url("/bearer"),"","bearer.bin");runUntilStopped(bearer);check(!canRequestLogin(bearer->data)&&!yes(bearer->data,"AuthenticationPromptPending"),"Bearer or browser-session authentication does not trigger a Basic password prompt");
 auto saved=manager.add("https://downloads.example.test/file","","remembered.bin");manager.setDownloadLogin(saved,"fixture-user","fixture-secret",true);
 auto sameSite=manager.add("https://downloads.example.test/next","","remembered-next.bin");auto otherSite=manager.add("https://cdn.downloads.example.test/next","","other-origin.bin");auto otherPort=manager.add("https://downloads.example.test:444/next","","other-port.bin");
 check(basicLogin(siteRequestHeaders(str(sameSite->data,"Url"),readHeaders(sameSite->data),manager.state["Settings"])).first=="fixture-user"&&headerValue(readHeaders(otherSite->data),"Authorization").empty()&&headerValue(readHeaders(otherPort->data),"Authorization").empty(),"Remembered logins apply only to their exact HTTPS origin");
 check(manager.state["Settings"]["SiteLogins"][0]["ProtectedPassword"]!="fixture-secret","Saved site password uses Windows protection");
 manager.setDownloadLogin(bearer,"u","p:2",true);check(basicLogin(siteRequestHeaders(server.url("/basic"),{},manager.state["Settings"])).first=="u","Explicitly remembered HTTP login matches its exact origin");
 auto automatic=manager.add(server.url("/basic"),"","saved-http.bin");runUntilStopped(automatic);check(str(automatic->data,"Status")=="Complete"&&readText(automatic->target())==server.payload&&readHeaders(automatic->data).empty(),"Saved HTTP credentials authenticate actual parallel transfers without being copied into jobs");
 auto prior=saved->data,priorSettings=manager.state["Settings"];auto blocker=manager.root/L"state.json.tmp";fs::create_directory(blocker);
 rejects([&]{manager.setDownloadLogin(saved,"changed","new-secret",true);},"Login storage failure is reported");check(saved->data==prior&&manager.state["Settings"]==priorSettings,"Failed login save rolls back both per-file and remembered credentials");fs::remove(blocker);
 Headers h={{"Cookie","fixture-cookie"}};rejects([&]{setBasicLogin(h,"bad:name","password");},"Login names with colons are rejected consistently");setBasicLogin(h,"u","p:2");setBasicLogin(h,"","",false);check(h.size()==2&&headerValue(h,"Cookie")=="fixture-cookie","Removing a login retains unrelated browser headers");
 const std::string source="https://www.youtube.com/watch?v=abcdefghijk",video="https://r1.googlevideo.com/videoplayback?itag=137&signature=fixture",audio="https://r2.googlevideo.com/videoplayback?itag=140&signature=fixture";
 Json media={{"Url",source},{"SourceUrl",source},{"Video",{{"Url",video}}},{"Audio",{{"Url",audio}}}};auto links=downloadLinks(media);
 check(downloadAddress(media)==video&&links.size()==2&&links[1].address==audio,"Captured direct video shows the real video and audio links instead of the webpage");
 media["ProtectedSabr"]=protect(Json{{"url",video}}.dump());check(downloadAddress(media)==video&&downloadLinks(media).size()==1&&mediaAddressNote(media).find("not a standalone file link")!=std::string::npos,"SABR displays its actual shared endpoint and identifies its session requirement");
 auto unchanged=media;downloadLinks(media);check(media==unchanged&&str(media,"Url")==source,"Viewing media URLs does not replace the canonical webpage or mutate captured sessions");
 Json adaptive={{"Url","https://player.example.test/watch"},{"ProtectedAdaptive",protect(Json{{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","https://cdn.example.test/part-1.m4s"}}})}}})}}.dump())}};
 check(downloadAddress(adaptive)=="https://cdn.example.test/part-1.m4s"&&downloadLinks(adaptive)[0].label=="First video segment","Segmented video addresses are labelled as segments rather than standalone full files");
 check(downloadLinks(same->snapshot()).size()==2&&downloadLinks(same->snapshot())[1].address==server.url("/basic"),"Link details distinguish requested and resolved HTTP addresses");
 manager.updateCompleted(same,{{"Url",server.url("/replacement")}});check(str(same->data,"ProtectedResolvedUrl").empty()&&downloadLinks(same->snapshot()).size()==1,"Editing a completed file address clears the previous server redirect");
 auto captured=manager.add(source,"","captured.mp4");captured->data["SourceUrl"]=source;captured->data["Status"]="Complete";
 rejects([&]{manager.updateCompleted(captured,{{"Url",video}});},"Completed media properties cannot replace the webpage with a raw streaming endpoint");
 rejects([&]{manager.setDownloadLogin(captured,"u","p");},"Browser-captured media keeps its browser sign-in workflow");
}
