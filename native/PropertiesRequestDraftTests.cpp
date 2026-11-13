#include "PropertiesRequestDraft.hpp"
#include <iostream>
using namespace udm;
static bool sealFails=false;
static std::string fixtureSeal(const std::string& value){if(sealFails)throw std::runtime_error("Controlled credential protection failure");return protect(value);}
int main(int argc,char** argv){
 Json checks=Json::array();bool passed=false;std::string error;
 auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 auto initial=[](){Headers headers;setBasicLogin(headers,"original-user","original-password",true);setHeader(headers,"Referer","https://fixture.example/original");return Json{{"ProtectedHeaders",protect(legacyDictionary(Json(headers)).dump())},{"Description","keep metadata"}};};
 auto originalInput=[](){return PropertiesRequestInput{"original-user","original-password","https://fixture.example/original"};};
 auto rejects=[](PropertiesRequestDraft& controls,Json& draft,const PropertiesRequestInput& input){try{controls.apply(draft,input);return false;}catch(const std::exception&){return true;}};
 try{
  {
   auto draft=initial(),before=draft;PropertiesRequestDraft controls(originalInput());
   PropertiesRequestInput input{"edited-user","edited-password","https://fixture.example/"+std::string(16400,'x')};
   check(rejects(controls,draft,input),"An oversized Referer rejects the combined credential edit");
   check(draft==before,"Rejected request edits preserve the full draft");
   check(rejects(controls,draft,input),"Retrying the same invalid input cannot silently accept the old headers");
   input.referer="https://fixture.example/corrected";controls.apply(draft,input);
   auto headers=readHeaders(draft);
   check(basicLogin(headers)==std::make_pair(input.user,input.password),"Correcting Referer preserves the other typed login changes");
   check(headerValue(headers,"Referer")==input.referer,"The corrected Referer saves with the credentials");
   check(str(draft,"Description")=="keep metadata","Request editing leaves unrelated metadata intact");
  }
  {
   auto draft=initial(),before=draft;PropertiesRequestDraft controls(originalInput(),fixtureSeal);
   PropertiesRequestInput input{"protected-user","protected-password","https://fixture.example/protected"};
   sealFails=true;check(rejects(controls,draft,input),"Credential protection failure rejects the edit");
   check(draft==before,"Protection failure preserves the previous protected headers");
   check(rejects(controls,draft,input),"Retrying failed protection still attempts the changed credentials");
   sealFails=false;controls.apply(draft,input);
   check(basicLogin(readHeaders(draft))==std::make_pair(input.user,input.password),"A later successful protection attempt retains the typed login");
   check(headerValue(readHeaders(draft),"Referer")==input.referer,"A later successful protection attempt retains the typed Referer");
  }
  {
   auto draft=initial();PropertiesRequestDraft controls(originalInput());
   auto headers=readHeaders(draft);setHeader(headers,"Authorization","Bearer fixture-fresh-session");setHeader(headers,"Referer","https://fixture.example/fresh");
   draft["ProtectedHeaders"]=protect(legacyDictionary(Json(headers)).dump());const auto before=draft;
   controls.apply(draft,originalInput());
   check(draft==before,"Untouched controls preserve refreshed credentials and their protected representation");
   auto input=originalInput();input.user="chosen-user";input.password="chosen-password";controls.apply(draft,input);
   check(basicLogin(readHeaders(draft))==std::make_pair(input.user,input.password),"An explicit login edit replaces the refreshed authorization");
   check(headerValue(readHeaders(draft),"Referer")=="https://fixture.example/fresh","An untouched Referer preserves its refreshed value during a login edit");
   controls.apply(draft,{"","",""});
   check(headerValue(readHeaders(draft),"Authorization").empty()&&headerValue(readHeaders(draft),"Referer").empty(),"Explicitly clearing fields removes both request headers");
  }
  {
   auto draft=initial();PropertiesRequestDraft controls(originalInput());const auto before=draft;controls.apply(draft,originalInput());
   check(draft==before,"Unchanged request fields do not re-encrypt protected headers");
   PropertiesRequestInput advanced{"advanced-user","advanced-password","https://fixture.example/advanced"};
   auto headers=readHeaders(draft);setBasicLogin(headers,advanced.user,advanced.password,true);setHeader(headers,"Referer",advanced.referer);draft["ProtectedHeaders"]=protect(legacyDictionary(Json(headers)).dump());controls.reset(advanced);const auto accepted=draft;
   controls.apply(draft,advanced);check(draft==accepted,"Accepting Advanced fields establishes a clean control baseline");
   advanced.password="corrected-advanced-password";controls.apply(draft,advanced);
   check(basicLogin(readHeaders(draft)).second==advanced.password,"Editing after Advanced still saves the new password");
  }
  {
   const Headers before{{"Authorization","Bearer old"},{"Cookie","fixture=old"},{"Referer","https://fixture.example/old"}};
   auto desired=before;setHeader(desired,"authorization","Bearer chosen");
   const Headers fresh{{"AUTHORIZATION","Bearer fresh"},{"Cookie","fixture=fresh"},{"Referer","https://fixture.example/fresh"},{"Accept","application/octet-stream"}};
   const Json current{{"ProtectedHeaders",protect(legacyDictionary(Json(fresh)).dump())}};
   Json edit{{"Description","retained metadata"}};PropertiesHeaderPatch patch(before,desired);patch.apply(edit,current);
   auto merged=readHeaders(edit);
   check(headerValue(merged,"Authorization")=="Bearer chosen","A header patch owns only the explicitly changed logical header");
   check(headerValue(merged,"Cookie")=="fixture=fresh"&&headerValue(merged,"Referer")=="https://fixture.example/fresh","Commit merging preserves unrelated refreshed cookie and Referer values");
   check(headerValue(merged,"Accept")=="application/octet-stream"&&str(edit,"Description")=="retained metadata","Header merging retains newly added session headers and staged metadata");
   check(readHeaders(current)==fresh,"Applying a request patch never mutates the supplied current record");
   desired=before;desired.erase("Referer");Json removed;PropertiesHeaderPatch(before,desired).apply(removed,current);
   check(!readHeaders(removed).count("Referer")&&headerValue(readHeaders(removed),"Authorization")=="Bearer fresh","Explicit header removal deletes that header without reverting current authentication");
   desired=before;desired["Authorization"]="";Json cleared;PropertiesHeaderPatch(before,desired).apply(cleared,current);
   check(readHeaders(cleared).count("Authorization")&&readHeaders(cleared).at("Authorization").empty(),"An explicit empty Authorization marker remains distinct from removing the header");
   Json untouched{{"Description","unchanged"}};const auto untouchedBefore=untouched;sealFails=true;
   PropertiesHeaderPatch(before,before).apply(untouched,current,fixtureSeal);
   check(untouched==untouchedBefore,"Unchanged logical headers do not re-encrypt the refreshed session");
   desired=before;setHeader(desired,"Authorization","Bearer fresh");PropertiesHeaderPatch satisfied(before,desired);satisfied.apply(untouched,current,fixtureSeal);
   check(untouched==untouchedBefore,"A header patch already satisfied by the current session does not re-encrypt it");
   auto onlyCase=before;onlyCase.erase("Authorization");onlyCase["authorization"]="Bearer old";PropertiesHeaderPatch(before,onlyCase).apply(untouched,current,fixtureSeal);
   check(untouched==untouchedBefore,"Header-name casing alone does not claim ownership of a refreshed value");
   Json retry{{"Description","retry draft"}};const auto retryBefore=retry;bool rejected=false;
   try{patch.apply(retry,current,fixtureSeal);}catch(const std::exception&){rejected=true;}
   check(rejected&&retry==retryBefore,"Failed commit-time protection preserves the entire staged edit");
   sealFails=false;patch.apply(retry,current,fixtureSeal);
   check(headerValue(readHeaders(retry),"Authorization")=="Bearer chosen"&&headerValue(readHeaders(retry),"Cookie")=="fixture=fresh","Retrying protection retains explicit request changes and the fresh session");
   desired=before;desired["Referer"]="invalid\r\nheader";rejected=false;
   try{PropertiesHeaderPatch invalid(before,desired);}catch(const std::exception&){rejected=true;}
   check(rejected,"Invalid request values are rejected before a commit patch is created");
  }
  {
   const Headers before{{"Authorization","Bearer old"},{"Cookie","fixture=old"},{"Referer","https://fixture.example/old"},{"Origin","https://fixture.example"}};
   auto desired=before;setHeader(desired,"Authorization","Bearer chosen");
   Json current{{"Url","https://fixture.example/changed"},{"ProtectedHeaders",protect(legacyDictionary(Json(before)).dump())}};
   Json edit;PropertiesHeaderPatch(before,desired,"https://FIXTURE.example:443/original").apply(edit,current);
   check(headerValue(readHeaders(edit),"Authorization")=="Bearer chosen","Same-origin refreshes use normalized host and default-port identity");
   for(const auto& target:{"https://other-fixture.example/file","http://fixture.example/file","https://fixture.example:444/file"}){
    current["Url"]=target;Json staged{{"Description","keep draft"}};const auto saved=staged;bool rejected=false;
    try{PropertiesHeaderPatch(before,desired,"https://fixture.example/original").apply(staged,current);}catch(const std::exception& e){rejected=std::string(e.what()).find("another site")!=std::string::npos;}
    check(rejected&&staged==saved,"Host, scheme or port changes reject staged authentication without altering the edit");
   }
   current["Url"]="https://other-fixture.example/file";
   for(const auto& name:{"Authorization","Cookie","Referer","Origin"}){
    desired=before;setHeader(desired,name,"chosen fixture value");Json staged;bool rejected=false;
    try{PropertiesHeaderPatch(before,desired,"https://fixture.example/original").apply(staged,current);}catch(const std::exception&){rejected=true;}
    check(rejected&&staged.is_null(),"Each explicitly changed sensitive header remains bound to the opening target origin");
   }
   desired=before;setHeader(desired,"User-Agent","fixture agent");Json agent;PropertiesHeaderPatch(before,desired,"https://fixture.example/original").apply(agent,current);
   check(headerValue(readHeaders(agent),"User-Agent")=="fixture agent","An ordinary User-Agent edit does not transplant staged authentication across origins");
   desired=before;desired.erase("Authorization");Json removed;PropertiesHeaderPatch(before,desired,"https://fixture.example/original").apply(removed,current);
   check(headerValue(readHeaders(removed),"Authorization").empty(),"Explicit authorization removal remains available after an origin change");
   desired=before;desired["Authorization"]="";Json cleared;PropertiesHeaderPatch(before,desired,"https://fixture.example/original").apply(cleared,current);
   check(readHeaders(cleared).count("Authorization")&&readHeaders(cleared).at("Authorization").empty(),"An empty authorization suppression marker is not treated as transmitting a credential");
   Json untouched;PropertiesHeaderPatch(before,before,"https://fixture.example/original").apply(untouched,current);
   check(untouched.is_null(),"An untouched session can follow a target change without carrying a request-header patch");
  }
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 if(argc>1){fs::path output=wide(argv[1]);fs::create_directories(output);atomicText(output/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
