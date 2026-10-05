#pragma once
#include "CaptureExclusions.hpp"
#include "CaptureReceipts.hpp"
static std::string exclusionCaptureToken(){auto value=guid();value.insert(20,"-");value.insert(16,"-");value.insert(12,"-");value.insert(8,"-");return std::to_string(epoch())+"-"+value;}
static void captureExclusionChecks(const fs::path& root){
 auto dir=root/L"capture-exclusions";std::string savedToken,savedId;Json policies;
 {
  Manager m(dir);auto p=m.state["Settings"];p["DownloadFolder"]=utf8((root/L"capture-files").wstring());p["PrefetchFileInfo"]=false;p["SkipBrowserFileInfo"]=false;p["DuplicatePolicy"]="Numbered";m.setSettings(p);
  auto request=[&](std::string address){return Json{{"action","add"},{"url",address},{"referrer","https://page.example.test/watch"},{"captureToken",exclusionCaptureToken()}};};
  auto take=[&](const std::string& address){auto r=request(address);auto j=m.receive(r);return m.takeAutomaticCapture(j);};
  auto original=m.state["Settings"];auto r=request("https://CDN.example.test/first.zip");savedToken=str(r,"captureToken");auto j=m.receive(r);savedId=j->id();auto first=m.takeAutomaticCapture(j);
  check(!first.empty()&&str(first,"host")=="cdn.example.test","Automatic receipt attributes cancellation to the file server rather than the referring page");
  check(m.takeAutomaticCapture(j).empty(),"One automatic offer cannot be presented twice to the cancellation tracker");
  check(m.finishAutomaticCapture(first,true).empty(),"First automatic cancellation never prompts or adds an exception");
  check(m.finishAutomaticCapture(first,true).empty(),"Repeated callback for one offer cannot count as a second cancellation");
  check(m.receive(r)==j&&m.takeAutomaticCapture(j).empty(),"Replayed accepted receipt does not create another cancellation decision");
  auto second=take("https://cdn.example.test/second.zip?literal=*&token=example#part");auto offer=m.finishAutomaticCapture(second,true);
  check(!offer.empty()&&str(offer,"address")=="https://cdn.example.test/second.zip?literal=*&token=example","Second consecutive cancellation offers the fragment-free exact download address");
  check(m.state["Settings"]==original,"Showing or declining an exclusion offer does not modify settings");
  auto third=take("https://cdn.example.test/third.zip");check(m.finishAutomaticCapture(third,true).empty(),"A declined offer requires another pair rather than prompting on every cancellation");
  auto accepted=take("https://cdn.example.test/fourth.zip");check(m.finishAutomaticCapture(accepted,false).empty(),"Start, Download Later, existing-file resume and completion reset the cancellation streak");
  check(m.finishAutomaticCapture(take("https://cdn.example.test/fifth.zip"),true).empty(),"A cancellation after acceptance starts a new streak");
  check(m.finishAutomaticCapture(take("https://other.example.test/sixth.zip"),true).empty(),"Different file server resets consecutive site cancellation counting");
  check(m.finishAutomaticCapture(take("https://cdn.example.test/seventh.zip"),true).empty(),"Returning to a previous file server does not reuse an older streak");
  auto manual=m.receive({{"action","add"},{"url","https://cdn.example.test/manual.zip"}});check(m.takeAutomaticCapture(manual).empty(),"Manual and context-menu Add requests without automatic receipts are not counted");
  auto disable=m.state["Settings"];disable["OfferCaptureExclusions"]=false;m.setSettings(disable);disable["OfferCaptureExclusions"]=true;m.setSettings(disable);
  check(m.finishAutomaticCapture(take("https://cdn.example.test/eighth.zip"),true).empty(),"Disabling and re-enabling the prompt clears the prior session streak");
  auto next=m.finishAutomaticCapture(take("https://cdn.example.test/ninth.zip"),true);check(!next.empty(),"Re-enabled prompt triggers after two new automatic cancellations");
  m.applyCaptureExclusions(offer,false,true,false);auto exactSettings=m.state["Settings"];
  check(str(exactSettings,"CaptureExcludedUrls")=="="+str(offer,"address")&&str(exactSettings,"CaptureExcludedHosts").empty(),"Exact-address acceptance does not silently broaden to the entire host or interpret a literal asterisk");
  m.applyCaptureExclusions(offer,false,true,false);check(m.state["Settings"]==exactSettings,"Repeatedly accepting the same exact address does not duplicate it");policies["exact"]=browserPreferences(exactSettings);
  m.applyCaptureExclusions(offer,true,false,true);check(str(m.state["Settings"],"CaptureExcludedHosts")=="cdn.example.test"&&!yes(m.state["Settings"],"OfferCaptureExclusions",true),"Site acceptance and prompt suppression persist together");policies["site"]=browserPreferences(m.state["Settings"]);
  check(str(m.state["Settings"],"DownloadFolder")==str(p,"DownloadFolder")&&str(m.state["Settings"],"DuplicatePolicy")=="Numbered","Accepting an exclusion preserves unrelated settings");
  check(m.finishAutomaticCapture(take("https://cdn.example.test/disabled1.zip"),true).empty()&&m.finishAutomaticCapture(take("https://cdn.example.test/disabled2.zip"),true).empty(),"Suppressed prompts remain suppressed across repeated cancellations");
  auto before=m.snapshot();auto blocked=m.root/L"state.json.tmp";fs::create_directory(blocked);rejects([&]{m.applyCaptureExclusions({{"address","https://new.example.test/new.zip"}},true,true,false);},"Exclusion acceptance reports a failed settings write");fs::remove(blocked);check(m.snapshot()==before,"Failed exclusion persistence rolls back host, exact address and all other settings");
  auto ipv6=captureExceptionSettings(p,{{"address","https://[::1]:8443/file.zip?x=*"}},true,true,false);check(str(ipv6,"CaptureExcludedHosts")=="[::1]","Literal IPv6 file servers can be excluded");policies["ipv6"]=browserPreferences(ipv6);
  auto longAddress="https://long.example.test/file.zip?token="+std::string(3000,'a');check(addressExceptions("="+longAddress).front()=="="+longAddress,"Exact signed addresses can exceed the old pattern length without truncation");
  for(auto bad:std::vector<std::string>{"=https://user:password@example.test/file","=ftp://example.test/file","=https://example.test/a b","=https://example.test/a\\b","="+std::string(17000,'x')})rejects([&]{addressExceptions(bad);},"Invalid or oversized exact exception is rejected");
  auto wrong=p;wrong["OfferCaptureExclusions"]="yes";rejects([&]{m.setSettings(wrong);},"Cancellation prompt setting requires a Boolean");
  std::vector<std::string> full;for(int i=0;i<100;++i)full.push_back("https://limit.example.test/"+std::to_string(i));auto limited=p;limited["CaptureExcludedUrls"]=captureExceptionText(full);rejects([&]{captureExceptionSettings(limited,offer,false,true,false);},"Exclusion offer respects the existing hundred-address limit");
  m.stop();
 }
 {
  Manager restored(dir);check(!yes(restored.state["Settings"],"OfferCaptureExclusions",true)&&!str(restored.state["Settings"],"CaptureExcludedUrls").empty(),"Exception choices and suppression survive a real state reload");JobPtr saved;for(auto job:restored.jobs)if(job->id()==savedId)saved=job;
  check(saved&&restored.takeAutomaticCapture(saved).empty(),"Restored history cannot replay a cancellation counter or prompt");
  auto replay=restored.receive({{"action","add"},{"url","https://cdn.example.test/first.zip"},{"captureToken",savedToken}});check(replay==saved&&restored.takeAutomaticCapture(replay).empty(),"Durable receipt retry after restart does not create a new automatic offer");restored.stop();
 }
 {
  Manager duplicate(root/L"capture-duplicates");auto p=duplicate.state["Settings"];p["DownloadFolder"]=utf8((root/L"duplicates-files").wstring());p["PrefetchFileInfo"]=false;duplicate.setSettings(p);duplicate.add("https://dupe.example.test/file.zip");
  auto one=duplicate.receive({{"action","add"},{"url","https://dupe.example.test/file.zip"},{"captureToken",exclusionCaptureToken()}});auto context=duplicate.takeAutomaticCapture(one);duplicate.resolveDuplicate(one,"Cancel");check(!context.empty()&&duplicate.finishAutomaticCapture(context,true).empty(),"Cancelling an automatic duplicate dialog counts once even after removing its candidate");
  auto two=duplicate.receive({{"action","add"},{"url","https://dupe.example.test/file.zip"},{"captureToken",exclusionCaptureToken()}});context=duplicate.takeAutomaticCapture(two);duplicate.resolveDuplicate(two,"Cancel");check(!duplicate.finishAutomaticCapture(context,true).empty(),"Second automatic duplicate cancellation produces the reviewed exclusion offer");
 }
 atomicText(root/L"capture-exclusion-policies.json",policies.dump(2),false);
}
