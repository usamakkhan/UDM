#pragma once
// Real loopback HTTP responses verify that refreshing a signed link preserves bytes
// only when the replacement identifies the same ranged representation.
static void recoveryChecks(const fs::path& root,Fixture& fixture){
 Manager manager(root/L"recovery-state");manager.state["Settings"]["CategoryFolders"]=false;
 manager.state["Settings"]["DownloadFolder"]=utf8((root/L"recovery-downloads").wstring());
 manager.state["Settings"]["Retries"]=0;
 const i64 retained=262144,size=(i64)fixture.payload.size();
 Bytes prefix(fixture.payload.begin(),fixture.payload.begin()+(size_t)retained);
 auto seed=[&](const char* name){
  auto j=manager.add(fixture.url("/expired"),"",name);
  j->data["Size"]=size;j->data["ETag"]="\"fixture-v1\"";j->data["RangeSupported"]=true;j->data["Received"]=retained;
  j->data["Segments"]=Json::array({{{"Index",0},{"Start",0},{"End",size-1},{"Done",retained}}});
  fs::create_directories(manager.root/L"parts"/wide(j->id()));writeBytes(manager.root/L"parts"/wide(j->id())/L"0000.part",prefix);manager.save();return j;
 };
 auto part=[&](JobPtr j){return manager.root/L"parts"/wide(j->id())/L"0000.part";};
 auto run=[&](JobPtr j){transfer(manager,j,std::make_shared<Cancel>());};
 auto j=seed("recovered.iso");auto savedHash=fileHash(part(j));
 bool http403=false;try{run(j);}catch(const HttpRejected& e){http403=e.status==403&&std::string(e.what()).find("Refresh download address")!=std::string::npos;}
 check(http403,"Expired HTTP 403 explains address recovery");
 check(fileHash(part(j))==savedHash&&num(j->data,"Received")==retained,"HTTP rejection preserves all saved partial bytes");
 auto expected=root/L"recovery-expected.bin";writeBytes(expected,Bytes(fixture.payload.begin(),fixture.payload.end()));
 auto oldId=j->id();manager.refreshAddress(j,fixture.url("/fresh?token=renewed"));
 check(j->id()==oldId&&fileHash(part(j))==savedHash&&yes(j->data,"RefreshPendingValidation"),"Replacing URL preserves identity, range plan and partial files");
 {Manager reopened(manager.root);JobPtr restored;for(auto item:reopened.jobs)if(item->id()==oldId)restored=item;
  fixture.resumedPrefix=false;transfer(reopened,restored,std::make_shared<Cancel>());
  check(fixture.resumedPrefix&&fileHash(restored->target())==fileHash(expected),"Refreshed URL resumes after saved offset across restart with exact SHA-256");}
 for(const char* variant:{"tag","size","no-range","no-validator","invalid-plan","stale-counter"}){
  std::string label=std::string(variant)+".bin";auto bad=seed(label.c_str());
  if(std::string(variant)=="tag"||std::string(variant)=="stale-counter")bad->data["ETag"]="\"different\"";
  if(std::string(variant)=="size")bad->data["Size"]=size+1;
  if(std::string(variant)=="no-validator")bad->data["ETag"]="";
  if(std::string(variant)=="invalid-plan")bad->data["Segments"][0]["End"]=size-2;
  if(std::string(variant)=="stale-counter")bad->data["Received"]=0;
  auto count=num(bad->data,"Received");auto plan=bad->data["Segments"];
  manager.refreshAddress(bad,fixture.url(std::string(variant)=="no-range"?"/plain":"/fresh"));
  bool rejected=false;try{run(bad);}catch(const std::exception& e){rejected=std::string(e.what()).find("Saved partial data was kept")!=std::string::npos;}
  check(rejected&&fileHash(part(bad))==savedHash&&bad->data["Segments"]==plan&&num(bad->data,"Received")==count&&yes(bad->data,"RefreshPendingValidation"),("Unsafe refresh keeps partial files: "+std::string(variant)).c_str());
 }
 auto props=seed("properties.bin");manager.configure(props,{{"Url",fixture.url("/plain")}});
 check(yes(props->data,"RefreshPendingValidation"),"Properties URL edits receive identical resume protection");
 auto auth=manager.add("https://old.example.test/file.iso","","headers.iso","Main queue",true,{{"Cookie","old-secret"},{"authorization","private"},{"Referer","https://old.example.test/account"},{"User-Agent","test-agent"}});
 manager.refreshAddress(auth,"https://new.example.test/file.iso");auto h=readHeaders(auth->data);
 check(h.size()==1&&h["User-Agent"]=="test-agent","Manual cross-origin refresh drops old credentials and referrer");
 manager.refreshAddress(auth,"https://new.example.test/file.iso?new",Headers{{"Cookie","fresh-cookie"}});
 check(readHeaders(auth->data)["Cookie"]=="fresh-cookie","Explicit browser replacement uses fresh request credentials");
 rejects([&]{manager.refreshAddress(auth,"file:///C:/private");},"Refresh rejects non-HTTP addresses");
 rejects([&]{manager.refreshAddress(auth,"https://new.example.test/file.iso",Headers{{"Cookie","a\r\nb"}});},"Refresh rejects header injection");
 auto capture=seed("browser-recover.iso");manager.beginAddressRefresh(capture);auto count=manager.jobs.size();auto original=str(capture->data,"Url");
 auto unrelated=manager.captureAddressRefresh(fixture.url("/unrelated.zip"),{},"unrelated.zip","");check(!unrelated,"Recovery ignores unrelated browser filenames");
 auto incoming=manager.receive({{"action","add"},{"url",fixture.url("/browser-recover.iso?fresh")},{"filename","browser-recover.iso"},{"cookies","fresh-private-cookie"},{"referrer","https://downloads.example.test/page"}});
 auto offer=manager.addressRefreshCandidate(capture);
 check(incoming==capture&&manager.jobs.size()==count&&str(capture->data,"Url")==original&&str(offer,"url").find("?fresh")!=std::string::npos,"Matching browser handoff stages replacement on original record for review");
 auto disk=readText(manager.root/L"state.json");
 check(disk.find("fresh-private-cookie")==std::string::npos&&disk.find("browser-recover.iso?fresh")==std::string::npos,"Pending browser URL and credentials remain encrypted at rest");
 auto second=seed("second.iso");rejects([&]{manager.beginAddressRefresh(second);},"Only one download can wait for a replacement at once");
 manager.cancelAddressRefresh(capture);check(!manager.captureAddressRefresh(fixture.url("/browser-recover.iso?late"),{},"browser-recover.iso",""),"Cancelled refresh stops intercepting new browser downloads");
 auto savedOffer=offer;offer["received"]=epoch()-11*60*1000;capture->data["ProtectedRefreshOffer"]=protect(offer.dump());check(manager.addressRefreshCandidate(capture).is_null(),"Expired pending browser offers are not presented");
 manager.refreshAddress(capture,str(savedOffer,"url"),savedOffer["headers"].get<Headers>(),str(savedOffer,"page"));
 check(readHeaders(capture->data)["Cookie"]=="fresh-private-cookie"&&!capture->data.contains("ProtectedRefreshOffer")&&recoveryPage(capture->data)=="https://downloads.example.test/page","Approved replacement applies fresh headers and source page");
 auto before=capture->data;auto realRoot=manager.root,blocked=root/L"recovery-blocked-root";writeBytes(blocked,Bytes{'x'});manager.root=blocked;
 rejects([&]{manager.refreshAddress(capture,fixture.url("/another"));},"Failed persistent replacement reports storage failure");manager.root=realRoot;
 check(capture->data==before,"Failed persistent replacement rolls back download metadata");
 capture->data["Status"]="Complete";check(!manager.canRefreshAddress(capture),"Completed files cannot enter address recovery");capture->data["Status"]="Paused";capture->data["SourceUrl"]="https://www.youtube.com/watch?v=abcdefghijk";check(!manager.canRefreshAddress(capture),"Video captures retain their dedicated stream workflow");
 check(recoveryPage({{"Url","https://software.download.prss.microsoft.com/dbazure/Win11_25H2_English_x64_v2.iso?expired"}})=="https://www.microsoft.com/en-us/software-download/windows11","Legacy Microsoft ISO points to the official fresh-link page");
 check(recoveryPage({{"Url","https://microsoft.com.evil.test/Win11_bad.iso"}}).empty(),"Recovery page fallback rejects deceptive host suffixes");
}
