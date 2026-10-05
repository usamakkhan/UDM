#pragma once
#include "CaptureReceipts.hpp"
static void captureLegacyChecks(const fs::path& root){
 auto token=[](i64 time=0){auto value=guid();value.insert(20,"-");value.insert(16,"-");value.insert(12,"-");value.insert(8,"-");return std::to_string(time?time:epoch())+"-"+value;};
 auto command=[](const std::string& id){return Json{{"action","capture-legacy-release"},{"captureToken",id},{"confirmed",true}};};
 auto directory=root/L"legacy-reviewed";std::string pending,expired;
 {
  Manager m(directory);pending=token();expired=token(epoch()-172800000);
  auto possible=m.add("https://example.invalid/old.zip","","old.zip","Main queue",true);
  m.state["BrowserCaptures"][pending]={{"created",epoch()},{"status","pending"}};m.save();auto jobs=m.snapshot()["Downloads"];
  check(str(releaseLegacyCapture(m,command(pending)),"status")=="released","Reviewed legacy pending reservation becomes a durable release barrier");
  check(m.snapshot()["Downloads"]==jobs,"Legacy resolution never guesses, modifies or deletes a possible matching job");
  check(str(releaseLegacyCapture(m,command(pending)),"status")=="released","Legacy release is idempotent after a lost reply");
  check(str(releaseLegacyCapture(m,command(expired)),"status")=="released","Explicit review can close a missing expired receipt");
  auto invalid=command(token());invalid["confirmed"]=false;rejects([&]{releaseLegacyCapture(m,invalid);},"Unconfirmed legacy choices cannot create barriers");
  invalid=command("malformed");rejects([&]{releaseLegacyCapture(m,invalid);},"Legacy review validates capture tokens");
  invalid=command(token(epoch()+61000));rejects([&]{releaseLegacyCapture(m,invalid);},"Future legacy tokens are rejected");
  for(const auto& status:{"prepared","review","discarded"}){auto id=token();m.state["BrowserCaptures"][id]={{"created",epoch()},{"protocol",2},{"status",status},{"request","protected-fixture"}};m.save();auto before=m.snapshot();rejects([&]{releaseLegacyCapture(m,command(id));},"Legacy review cannot consume protocol-2 captured requests");check(m.snapshot()==before,"Protocol-2 review rejection preserves all state");}
  auto accepted=token();m.state["BrowserCaptures"][accepted]={{"created",epoch()},{"status","accepted"},{"id",possible->id()}};m.save();auto before=m.snapshot();
  auto receipt=releaseLegacyCapture(m,command(accepted));check(str(receipt,"status")=="accepted"&&str(receipt,"id")==possible->id(),"Known native ownership takes precedence over an uncertain browser review");check(m.snapshot()==before,"Known native receipt remains unchanged by legacy review");
  m.remove(possible);check(str(releaseLegacyCapture(m,command(accepted)),"status")=="accepted","Deleted accepted history does not authorize a replay");
 }
 {
  Manager m(directory);check(str(reconcileCapture(m,pending),"status")=="released","Reviewed legacy release survives native restart");
  for(const auto& id:{pending,expired})rejects([&]{m.receive(Json{{"action","add"},{"captureToken",id},{"url","https://example.invalid/late.zip"}});},"Late native Add is rejected after manual recovery");
  check(m.jobs.empty(),"Legacy recovery and delayed messages create no downloads");
 }
 {
  Manager m(root/L"legacy-review-disk-error");auto id=token();m.state["BrowserCaptures"][id]={{"created",epoch()},{"status","pending"}};auto before=m.snapshot();fs::create_directory(m.root/L"state.json");
  rejects([&]{releaseLegacyCapture(m,command(id));},"Legacy release requires successful catalog persistence");check(m.snapshot()==before,"A failed legacy release rolls back its memory state");fs::remove(m.root/L"state.json");
 }
}
