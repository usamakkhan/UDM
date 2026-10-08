#pragma once
#include "ExportScope.hpp"
static void idmExportChecks(const udm::fs::path& root){
 using namespace udm;
 std::string sample="<\r\nhttps://example.test/one.zip?q=1\r\nreferer: https://example.test/page\r\ncookie: sid=fixture\r\npd: a=1&b=two+words\r\nUser-Agent: Fixture/1.0\r\n>\r\n<\r\nhttps://example.test/two.zip\r\n>\r\n";
 auto textLinks=importTextUrls("Saved links: <a href='https://example.test/a.zip?x=1&y=2'>One</a>\nAlso \"ftp://example.test/b.zip\"\nhttps://example.test/a.zip?x=1&y=2\nLocal file://C:/bad");check(textLinks==std::vector<std::string>{"https://example.test/a.zip?x=1&y=2","ftp://example.test/b.zip"},"Text import finds embedded HTML and quoted links without duplicating addresses");
 rejects([&]{importTextUrls("No links here.");},"Text import gives a clear empty-file result");
 auto parsed=parseIdmExport(sample);check(parsed.size()==2&&parsed[0].referer=="https://example.test/page"&&parsed[1].referer.empty(),"EF2 groups request context with exactly its own URL");
 check(parsed[0].cookie=="sid=fixture"&&parsed[0].post=="a=1&b=two+words"&&parsed[0].userAgent=="Fixture/1.0","EF2 reads all four fields present in the installed reference reader");
 check(serializeIdmExport(parsed)==sample,"EF2 writer emits the reference CRLF delimiter and field layout");
 rejects([&]{expand(sample);},"Previous generic URL-list parser rejects a bracketed IDM export");
 auto lf=sample;lf.erase(std::remove(lf.begin(),lf.end(),'\r'),lf.end());check(serializeIdmExport(parseIdmExport("\xef\xbb\xbf"+lf))==sample,"BOM and LF exports preserve all request fields");
 std::string utf16="\xff\xfe";for(unsigned char c:sample){utf16.push_back(c);utf16.push_back(0);}check(serializeIdmExport(parseIdmExport(utf16))==sample,"UTF-16LE BOM imports decode before parsing");
 std::string be="\xfe\xff";for(unsigned char c:sample){be.push_back(0);be.push_back(c);}check(serializeIdmExport(parseIdmExport(be))==sample,"UTF-16BE BOM imports decode before parsing");
 auto emptyPost=parseIdmExport("<\nhttps://example.test/post\npd: \n>\n");check(emptyPost[0].post&&emptyPost[0].post->empty(),"An empty saved form body still means POST");
 auto padded=parseIdmExport("<\nhttps://example.test/post\npd:  x=two  \n>\n");check(*padded[0].post==" x=two  ","Form data retains significant leading and trailing spaces");
 for(auto bad:{std::string(""),std::string("https://example.test/a"),std::string("<\n>"),std::string("<\nhttps://example.test/a\n"),std::string("<\nhttps://example.test/a\n<\nhttps://example.test/b\n>"),std::string("<\nhttps://example.test/a\nreferer: https://example.test/\nREFERER: https://other.test/\n>"),std::string("<\nhttps://example.test/a\npostdata: a=1\n>"),std::string("<\nfile:///C:/test\n>"),std::string("<\nhttps://example.test/a\nreferer: javascript:bad\n>"),std::string("<\nhttps://example.test/a\nUser-Agent: value\rInjected: header\n>"),sample+std::string("\0",1),utf16.substr(0,utf16.size()-1)})rejects([&]{parseIdmExport(bad);},"Invalid or unsupported export is rejected as a whole");
 rejects([&]{parseIdmExport(std::string(16*1024*1024+1,'x'));},"Export import has a bounded total file size");
 auto longRow=parsed[0];longRow.userAgent=std::string(16385,'x');rejects([&]{serializeIdmExport({longRow});},"Oversized saved headers cannot enter the request engine");
 longRow=parsed[0];longRow.post=std::string(MaxBrowserPostBytes+1,'x');rejects([&]{serializeIdmExport({longRow});},"Saved POST bodies keep the native four-MiB limit");
 Manager m(root/L"ef2-state");auto prefs=m.state["Settings"];auto folder=utf8((root/L"ef2-files").wstring());prefs["DownloadFolder"]=folder;prefs["CategoryFolders"]=false;m.setSettings(prefs);auto before=m.snapshot();
 rejects([&]{importIdmFile(m,parsed,folder,"Imported",false);},"Import requires explicit use of saved cookies and form data");check(m.snapshot()==before,"Refused saved-session import leaves the catalog unchanged");
 check(importIdmFile(m,parsed,folder,"Imported",true)==2,"EF2 import creates every selected record");auto first=m.jobs[0];check(str(first->data,"Status")=="Paused"&&!m.isActive(first)&&!yes(m.state["Queues"].back(),"Enabled"),"Imported downloads and new queue remain stopped");
 auto headers=readHeaders(first->data);check(headerValue(headers,"Referer")==parsed[0].referer&&headerValue(headers,"User-Agent")==parsed[0].userAgent&&headerValue(headers,"Cookie")==parsed[0].cookie,"Imported request headers survive Windows-account encryption");
 auto request=readPostRequest(first->data);check(str(request,"method")=="POST"&&str(request,"contentType")=="application/x-www-form-urlencoded"&&unb64(str(request,"body"))==Bytes(parsed[0].post->begin(),parsed[0].post->end()),"Imported pd field becomes an exact POST request");
 auto saved=readText(m.root/L"state.json");check(saved.find("sid=fixture")==std::string::npos&&saved.find("a=1&b=two+words")==std::string::npos,"Saved cookies and form payload do not remain plaintext in the catalog");
 check(importIdmFile(m,parsed,folder,"Imported",true)==0,"Reimport of the same URL, headers and form avoids duplicate records");
 auto changed=parsed[0];changed.post="a=different";check(importIdmFile(m,{changed},folder,"Imported",true)==1&&m.jobs.size()==3,"Distinct POST bodies at one URL are not wrongly deduplicated");
 check(exportIdmFile(m,{first,m.jobs[1]},true)==sample,"UDM job export retains interoperable URL and request context");
 auto exportPath=root/L"preserved.ef2";atomicText(exportPath,"keep",false);rejects([&]{writeDownloadExport(m,{first},exportPath,false,false,true);},"Sensitive EF2 export requires explicit plain-text opt-in");check(readText(exportPath)=="keep","Rejected export never replaces an existing file");writeDownloadExport(m,{first},exportPath,false,true,true);check(parseIdmExport(readText(exportPath)).size()==1,"Selected EF2 export writes only the chosen record");
 before=m.snapshot();auto invalid=parsed;invalid.back().url="javascript:bad";rejects([&]{importIdmFile(m,invalid,folder,"Imported",true);},"Invalid final entry rejects the whole import before saving");check(m.snapshot()==before,"Invalid later entry leaves earlier records untouched");
 auto newRow=parsed[1];newRow.url="https://example.test/new.zip";fs::create_directory(m.root/L"state.json.tmp");rejects([&]{importIdmFile(m,{newRow},folder,"New import queue",false);},"Import surfaces a catalog persistence failure");fs::remove(m.root/L"state.json.tmp");check(m.snapshot()==before,"Persistence failure rolls back all jobs and the newly created queue");
 {Manager reopened(m.root);check(readHeaders(reopened.jobs[0]->data)==headers&&readPostRequest(reopened.jobs[0]->data)==request,"Imported request context survives application restart");}
 auto authorization=m.add("https://example.test/auth",folder,"auth.bin","Main queue",true,{{"Authorization","Bearer fixture"}});rejects([&]{exportIdmFile(m,{authorization},true);},"EF2 cannot silently drop unsupported authentication headers");
 authorization->data["SourceUrl"]="https://example.test/watch";rejects([&]{exportIdmFile(m,{authorization},true);},"Media download plans are not silently reduced to an ordinary GET");
 {
  FormFixture server;auto url=server.url("/echo");IdmExportRecord row;row.url=url;row.referer=server.url("/page");row.cookie="fixture=one";row.userAgent="UDM-EF2-Fixture/1";row.post="x=one%2Btwo&empty=&utf8=%E2%9C%93";
  Manager live(root/L"ef2-live");auto settings=live.state["Settings"];settings["DownloadFolder"]=utf8((root/L"ef2-live-files").wstring());settings["CategoryFolders"]=false;settings["Retries"]=0;settings["ProxyMode"]="Connect directly";live.setSettings(settings);importIdmFile(live,parseIdmExport(serializeIdmExport({row})),"","Imported",true);check(server.records.empty(),"Import itself makes no network requests");auto job=live.jobs[0];live.resume(job);live.tick();for(int i=0;i<1200&&(live.isActive(job)||str(job->data,"Status")=="Queued");++i){Sleep(10);live.tick();}live.stop();
  check(str(job->data,"Status")=="Complete"&&readText(job->target())==*row.post,"An imported EF2 POST downloads exact echoed bytes through the actual engine");
  auto actual=server.snapshot();check(actual.size()==1&&str(actual[0],"method")=="POST"&&!yes(actual[0],"range"),"Imported form uses one POST with no GET, HEAD or range probe");
  check(str(actual[0],"referer")==row.referer&&str(actual[0],"cookie")==row.cookie&&str(actual[0],"agent")==row.userAgent,"Actual HTTP request sends the imported Referer, Cookie and User-Agent");
 }
}

static void individualStartChecks(const udm::fs::path& root,Fixture& fixture){
 using namespace udm;Manager m(root/L"individual-resume");auto settings=m.state["Settings"];settings["DownloadFolder"]=utf8((root/L"individual-files").wstring());settings["CategoryFolders"]=false;settings["Parallel"]=1;settings["Retries"]=0;settings["ProxyMode"]="Connect directly";m.setSettings(settings);
 auto q=defaultQueue("Stopped");q["Enabled"]=false;q["FinishAction"]="Exit UDM";m.setQueue(q);auto first=m.add(fixture.url("/slow"),"","first.bin","Stopped"),second=m.add(fixture.url("/slow"),"","second.bin","Stopped"),unselected=m.add(fixture.url("/range"),"","unselected.bin","Stopped");
 auto before=m.snapshot();fs::create_directory(m.root/L"state.json.tmp");rejects([&]{m.resume(first);},"Resume reports persistence failure before an individual start");fs::remove(m.root/L"state.json.tmp");check(m.snapshot()==before&&!m.isActive(first),"Failed Resume preserves individual state and never starts a request");
 m.resume(first);m.resume(second);m.tick();check(m.isActive(first)&&!m.isActive(second)&&str(second->data,"Status")=="Queued","Two explicit starts in a disabled queue still obey the global parallel limit");
 // A finished file can still have an active worker while final cleanup runs.
 // Wait for that lifecycle boundary before checking start-intent removal or
 // expecting another job to consume the single available active slot.
 auto settled=[&]{Lock lock(m.mutex);return str(first->data,"Status")=="Complete"&&str(second->data,"Status")=="Complete"&&!m.isActive(first)&&!m.isActive(second);};
 for(int i=0;i<1800&&!settled();++i){Sleep(10);m.tick();}
 check(str(first->data,"Status")=="Complete"&&str(second->data,"Status")=="Complete"&&readText(first->target())==fixture.payload&&readText(second->target())==fixture.payload,"Explicit starts complete exact files without enabling their queue");
 check(str(unselected->data,"Status")=="Paused"&&!yes(m.state["Queues"].back(),"Enabled")&&m.takeQueueCompletions().empty(),"Individual starts leave unselected files paused and do not arm queue completion actions");
 check(!first->data.contains("IndividualStart")&&!second->data.contains("IndividualStart"),"Completed individual downloads clear their temporary start intent");
 q["Enabled"]=true;q["RunOnce"]=true;q["StartOnceUtc"]=date(epoch()+3600000);q["StopOnceUtc"]=date(epoch()+7200000);m.setQueue(q);auto scheduled=m.add(fixture.url("/slow"),"","scheduled.bin","Stopped");m.resume(scheduled);m.tick();check(m.isActive(scheduled),"Explicit Resume overrides a future schedule for only the chosen file");m.pause(scheduled);for(int i=0;i<800&&m.isActive(scheduled);++i){Sleep(10);m.tick();}check(str(scheduled->data,"Status")=="Paused"&&!yes(scheduled->data,"IndividualStart"),"Pause cancels an individual start without a timer restarting it");
 auto never=m.add(fixture.url("/range"),"","never.bin","Stopped");m.resume(never);m.pause(never);auto requests=fixture.requests.load();m.tick();check(str(never->data,"Status")=="Paused"&&fixture.requests==requests,"Pause before the next timer tick prevents all network work");m.stop();
}
