#pragma once
static void cookieResumeChecks(const fs::path& root){
 GrabberFilterFixture fixture;const auto payload=std::string(3*1024*1024,'c');
 fixture.redirect("/begin",fixture.url("/resume.bin"),"Set-Cookie: session=resume-token; Path=/; HttpOnly\r\n");
 fixture.file("/resume.bin",payload,"Content-Type: application/octet-stream\r\nETag: \"cookie-resume-fixture\"\r\n");fixture.requireCookie("/resume.bin","session=resume-token");
 auto folder=root/L"cookie-resume-state";i64 retained=0;std::string nextRange;Json savedSession;
 {
  Manager manager(folder);auto settings=defaultSettings();settings["ProxyMode"]="Connect directly";settings["Retries"]=0;settings["CategoryFolders"]=false;settings["DownloadFolder"]=utf8((root/L"cookie-resume-downloads").wstring());manager.setSettings(settings);
  auto job=manager.add(fixture.url("/begin"),"","resume.bin","Main queue",true);Json session={{"Origin",Url(fixture.url("/")).origin},{"UserAgent","Cookie resume fixture"},{"LogoutPages",""},{"Cookies",Json::array({sessionCookie("session","original-token")})}};job->data["ProtectedBrowserSession"]=protect(session.dump());job->data["Connections"]=4;job->data["LimitKbps"]=128;manager.save();
  auto cancel=std::make_shared<Cancel>();std::atomic_bool finished{false},cancelled{false};std::exception_ptr error;
  std::thread worker([&]{try{transfer(manager,job,cancel);}catch(const Cancelled&){cancelled=true;}catch(...){error=std::current_exception();}finished=true;});
  auto deadline=GetTickCount64()+15000;while(!finished&&GetTickCount64()<deadline){{Lock lock(manager.mutex);if(num(job->data,"Received")>=65536)break;}Sleep(10);}cancel->stop=true;worker.join();if(error)std::rethrow_exception(error);
  retained=num(job->data,"Received");savedSession=readBrowserSession(job->data);for(const auto& part:job->data["Segments"]){auto count=num(part,"Done");if(count>0&&count<num(part,"End")-num(part,"Start")+1){nextRange="Range: bytes="+std::to_string(num(part,"Start")+count)+"-";break;}}
  check(cancelled&&retained>0&&retained<(i64)payload.size()&&!nextRange.empty(),"Authenticated download pauses with a real partial range retained");
  check(str(savedSession["Cookies"][0],"value")=="resume-token","Paused transfer persists the server's replacement cookie");job->data["Status"]="Paused";manager.save();
 }
 // The entry point now rejects the originally captured token. Successful resume
 // therefore requires the encrypted replacement saved by the previous process.
 fixture.requireCookie("/begin","session=resume-token");Manager reopened(folder);auto job=reopened.jobs.at(0);check(num(job->data,"Received")==retained&&readBrowserSession(job->data)==savedSession,"Restart restores both the retained range and updated authentication");job->data["LimitKbps"]=0;transfer(reopened,job,std::make_shared<Cancel>());
 check(readText(job->target(),4*1024*1024)==payload&&fixture.has("/resume.bin",nextRange),"Resumed authenticated transfer requests the saved byte offset and publishes exact output");
}
