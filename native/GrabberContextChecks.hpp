#pragma once
#include "GrabberAuth.hpp"
static void grabberContextChecks(const fs::path& root){
 auto description=[](const std::string& html){for(auto ref:siteReferences(html))if(ref.page)return ref.description;return std::string();};
 check(description("<a href='a.zip'> Download <b>the report</b> &amp; notes </a>")=="Download the report & notes","Grabber descriptions preserve formatted link text and decode entities once");
 check(description("<a href='a.zip'>A&nbsp;&#32;B<br>C</a>")=="A B C","Link descriptions collapse HTML whitespace and line breaks");
 check(description("<a href='a.zip'><img src='x.png' alt='Report &amp;lt;draft&amp;gt;'></a>")=="Report &lt;draft&gt;","Image-only links use decoded alt text without double decoding");
 check(description("<a href='a.zip'>日本語 &#x1f642;</a>")=="日本語 🙂","Link descriptions preserve Unicode and supplementary code points");
 check(description("<a href='a.zip'>A<!--hidden--><script>bad()</script><style>.bad{}</style>B</a>")=="AB","Script, stylesheet and comment text are excluded from descriptions");
 auto refs=siteReferences("<a href='one.zip'>One<a href='two.zip'>Two</a>");check(refs.size()==2&&refs[0].description=="One"&&refs[1].description=="Two","Malformed nested anchors keep separate bounded descriptions");
 check(description("<a href='a.zip'>Trailing text")=="Trailing text","Unclosed anchors retain their trailing visible text");
 check(description("<a href='a.zip'>Visible<script>never closed")=="Visible","Unclosed raw-text scripts do not become download descriptions");
 check(description("<a href='a.zip'>Visible<style>never closed")=="Visible","Unclosed stylesheet text does not become a download description");
 auto large=description("<a href='a.zip'>"+std::string(1023,'x')+"日本語"+std::string(10000,'a')+"</a>");check(large.size()<=1024&&utf8(wide(large))==large,"Description limits preserve valid UTF-8 boundaries");
 auto empty=siteReferences("<a href='a.zip'></a><a href='b.zip'>B</a>");check(empty[0].description.empty()&&empty[1].description=="B","Empty anchors do not borrow neighboring labels");

 GrabberFilterFixture fixture;auto project=modernGrabber(fixture.url("/private/index.html"));project["MaxPages"]=1;project["MetadataParallel"]=3;
 auto settings=defaultSettings();settings["ProxyMode"]="Connect directly";settings["Retries"]=0;settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((root/L"grabber-context-downloads").wstring());Cancel cancel;
 fixture.page("/private/index.html","<a href='file.pdf'><b>Annual</b> report</a><a href='range.zip'>ZIP &amp; data</a><a href='next.html'>Next page</a>");
 fixture.file("/private/file.pdf","exact-report-bytes","Content-Type: application/pdf\r\n");fixture.file("/private/range.zip","exact-archive-bytes","Content-Type: application/zip\r\n",false,true);
 fixture.page("/private/next.html","<a href='last.pdf'>Last report</a>");fixture.file("/private/last.pdf","last-report");
 Headers auth;setBasicLogin(auth,"reader","test:password");const auto authorization=headerValue(auth,"Authorization");
 for(const char* path:{"/private/index.html","/private/file.pdf","/private/range.zip","/private/next.html","/private/last.pdf"})fixture.requireLogin(path,authorization);
 auto denied=explore(project,settings,cancel);check(denied["Links"].empty()&&!denied["Errors"].empty(),"Protected Grabber pages fail cleanly without project credentials");
 setGrabberLogin(project,"reader","wrong");denied=explore(project,settings,cancel);check(denied["Links"].empty()&&!denied["Errors"].empty(),"Incorrect project passwords do not produce collected files");
 setGrabberLogin(project,"reader","test:password");auto protectedValue=str(project,"ProtectedLogin");setGrabberLogin(project,"reader","test:password");check(str(project,"ProtectedLogin")==protectedValue,"Unchanged project credentials retain resumable exploration identity");
 auto collected=explore(project,settings,cancel);check(collected["Links"].size()==2&&collected["Errors"].empty(),"Project login authenticates page exploration and parallel file metadata");
 check(fixture.has("/private/range.zip","Authorization: "+authorization)&&fixture.count("/private/range.zip","HEAD")>0&&fixture.count("/private/range.zip","GET")>0,"Authenticated metadata fallback performs both HEAD and range GET");
 auto get=[&](const Json& values,const std::string& suffix){for(auto row:values["Links"])if(str(row,"Url")==fixture.url(suffix))return row;return Json::object();};
 check(str(get(collected,"/private/file.pdf"),"Description")=="Annual report"&&str(get(collected,"/private/range.zip"),"Description")=="ZIP & data","Exploration attaches link text to real collected file metadata");
 check(collected["ExploreState"]["Pending"].size()==1&&str(collected["ExploreState"]["Pending"][0],"Description")=="Next page","Pending page descriptions survive a stopped exploration checkpoint");
 collected["MaxPages"]=3;auto resumed=explore(collected,settings,cancel);check(resumed["Links"].size()==3&&fixture.count("/private/index.html")==3,"Resuming an authenticated project does not refetch its completed start page");
 auto changed=collected;setGrabberLogin(changed,"reader","wrong");auto rejected=explore(changed,settings,cancel);check(rejected["Links"].empty(),"Changed credentials invalidate old exploration continuation");
 auto malformed=collected;malformed["ExploreState"]["Pending"][0]["Description"]=Json::array();rejects([&]{explore(malformed,settings,cancel);},"Malformed saved page descriptions are rejected before continuation");

 malformed=collected;malformed["Links"][0]["Description"]=std::string(1025,'a');rejects([&]{explore(malformed,settings,cancel);},"Oversized saved file descriptions cannot bypass continuation validation");
 AuthenticationFixture digest;auto digestProject=modernGrabber(digest.url("/digest"));setGrabberLogin(digestProject,"u","p:2");auto digestHeaders=grabberRequestHeaders(digestProject,"",digest.url("/digest"));Http digestResponse(digest.url("/digest"),digestHeaders,settings,cancel);check(digestResponse.status==200&&digest.digestHeaders>0&&digest.accepted>0,"Project logins negotiate server-verified Digest authentication");
 Manager manager(root/L"grabber-context-state");manager.setSettings(settings);manager.saveProject(resumed);
 check(readText(manager.root/L"state.json").find("test:password")==std::string::npos&&readText(manager.root/L"state.json").find(authorization)==std::string::npos,"Saved projects do not contain plaintext passwords or Authorization values");
 {Manager reopened(manager.root);auto saved=grabberProject(reopened,str(project,"Id"));check(str(grabberLogin(saved),"Password")=="test:password"&&saved["Links"]==resumed["Links"],"Protected login and descriptions survive catalog restart");}
 check(manager.addProject(resumed,"Main queue",true)==3,"Authenticated collected files can be added for later downloading");
 bool downloaded=true;for(auto job:manager.jobs){transfer(manager,job,std::make_shared<Cancel>());downloaded&=fs::exists(job->target())&&fs::file_size(job->target())>0&&basicLogin(readHeaders(job->data)).second=="test:password";}
 check(downloaded&&readText(manager.jobs.front()->target())=="exact-report-bytes","Queued project downloads retain authentication and publish exact bytes");
 check(str(manager.jobs.front()->data,"Description")=="Annual report","Link descriptions reach the main download catalog");
 auto templateItem=saveGrabberTemplate(manager,resumed,"Authenticated reports");check(!templateItem["Settings"].contains("ProtectedLogin")&&!templateItem["Settings"].contains("StartUrl"),"Templates do not copy project login credentials or source addresses");
 auto before=manager.state["Projects"];auto blocked=resumed;setGrabberLogin(blocked,"reader","replacement");{Handle locked(CreateFileW((manager.root/L"state.json").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));check(bool(locked),"Project login rollback fixture locks the saved catalog");rejects([&]{manager.saveProject(blocked);},"A failed project login save is reported");check(manager.state["Projects"]==before,"Failed credential persistence restores the saved project");}
 auto other=project;other["StartUrl"]="https://different.example.test/";rejects([&]{grabberLogin(other);},"Changing the starting origin never silently reuses a saved project password");
 check(headerValue(grabberRequestHeaders(project,"",fixture.url("/file.pdf",true)),"Authorization").empty(),"Project credentials are not sent to a different host");
 check(headerValue(grabberRequestHeaders(project,"","https://127.0.0.1:"+std::to_string(fixture.port)+"/file.pdf"),"Authorization").empty(),"Project credentials are not sent to a different scheme");
 check(headerValue(grabberRequestHeaders(project,"","http://127.0.0.1:1/file.pdf"),"Authorization").empty(),"Project credentials are not sent to a different port");
 auto corrupt=project;corrupt["ProtectedLogin"]="not-a-windows-secret";rejects([&]{grabberLogin(corrupt);},"Unreadable project credentials require reentry rather than silent anonymous fallback");
 setGrabberLogin(corrupt,"replacement-user","replacement-password");check(str(grabberLogin(corrupt),"UserName")=="replacement-user","Re-entered credentials repair an unreadable saved login");
 corrupt=project;corrupt["ProtectedLogin"]=Json::object();rejects([&]{grabberLogin(corrupt);},"Malformed protected credential values are rejected");
 rejects([&]{setGrabberLogin(other,"bad:name","secret");},"Project logins validate username syntax");
 setGrabberLogin(other,"","",false);check(!other.contains("ProtectedLogin"),"Disabling a project login removes its saved credential");
 fixture.page("/cross.html","<a href='/same.pdf'>Same</a><a href='"+fixture.url("/foreign.pdf",true)+"'>Foreign</a><a href='/redirect.pdf'>Redirect</a>");
 fixture.file("/same.pdf","same");fixture.file("/foreign.pdf","foreign");fixture.redirect("/redirect.pdf",fixture.url("/foreign.pdf",true));
 auto cross=modernGrabber(fixture.url("/cross.html"));cross["FilesSameSite"]=false;setGrabberLogin(cross,"reader","test:password");auto crossResult=explore(cross,settings,cancel);
 check(crossResult["Errors"].empty()&&fixture.has("/same.pdf","Authorization: "+authorization)&&!fixture.has("/foreign.pdf","Authorization: "+authorization),"Real foreign links and cross-origin redirects strip project authentication");
 auto anonymous=modernGrabber(fixture.url("/cross.html"));anonymous["UseLinkDescriptions"]=false;auto noDescriptions=explore(anonymous,settings,cancel);check(std::all_of(noDescriptions["Links"].begin(),noDescriptions["Links"].end(),[](const Json& row){return str(row,"Description").empty();}),"Disabling descriptions leaves collected files unannotated");
 auto invalid=resumed;invalid["Links"][0]["Description"]=std::string(1025,'a');rejects([&]{manager.saveProject(invalid);},"Saved project descriptions have a bounded size");
 auto archive=project;archive["Id"]=guid();archive["Template"]="Offline website (ZIP)";archive["Name"]="Protected archive";archive["Links"]=Json::array();auto zip=manager.addOfflineProject(archive,"Main queue",true);offlineTransfer(manager,zip,std::make_shared<Cancel>());check(fs::exists(zip->target())&&basicLogin(readHeaders(zip->data)).second=="test:password","Offline website ZIP downloads inherit the same protected project login");
}

