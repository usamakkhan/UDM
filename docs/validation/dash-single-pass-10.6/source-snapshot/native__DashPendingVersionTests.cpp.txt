#include "DashPendingVersion.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;const auto root=fs::absolute(argv[1]);fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool ok,const char* label){checks.push_back({{"name",label},{"passed",ok}});if(!ok)throw std::runtime_error(label);};
 try{
  const Json version={{"url","http://127.0.0.1:19001/file"},{"total",4096},{"etag","\"owned\""}},urls=Json::array({"http://127.0.0.1:19001/file"});
  const Json pending={{"versionRequired",true},{"urls",urls},{"committed",false}};
  check(dashPendingFullResourceVersion(pending),"A fresh full-resource intent requires its first version");
  auto bound=pending;bound["resourceVersion"]=version;check(!dashPendingFullResourceVersion(bound),"A bound intent can verify only its versioned receipt");
  check(!dashPendingFullResourceVersion(Json{{"committed",true},{"resource",Json::object()}}),"Legacy resources retain their original recovery policy");
  auto rejects=[&](Json value,const char* label){bool threw=false;try{dashPendingFullResourceVersion(value);}catch(const std::exception&){threw=true;}check(threw,label);};
  auto value=pending;value["committed"]=true;rejects(value,"An unbound intent cannot claim committed data");
  value=pending;value["resource"]=Json::object();rejects(value,"An unbound intent cannot import an unversioned resource record");
  value=pending;value["file"]="old.part";rejects(value,"An unbound initialization cannot adopt an unversioned file");
  value=bound;value["byteRange"]={{"start",0},{"length",1}};rejects(value,"Ranged resources cannot enter full-response discovery");
  value=pending;value["versionRequired"]="yes";rejects(value,"Nonboolean version policy is rejected");
  value=pending;value.erase("urls");rejects(value,"Missing pending resource locations are rejected");
  value=pending;value["urls"]=Json::array();rejects(value,"Empty pending locations cannot authorize a body");
  value=pending;value["urls"]=Json::array({17});rejects(value,"Nonstrings cannot become resource locations");
  value=pending;value["urls"]=Json::array();for(int n=0;n<33;++n)value["urls"].push_back("http://127.0.0.1/file");rejects(value,"Pending URL alternatives retain their bound");
  const Json init={{"pendingInitializationRequiresVersion",true},{"pendingInitialization",urls}};
  check(dashPendingInitializationVersion(init),"A pending initialization cannot reuse an unversioned receipt");
  value=init;value["pendingInitializationVersion"]=version;check(!dashPendingInitializationVersion(value),"A checkpointed initialization version permits verified recovery");
  auto rejectsInit=[&](Json input,const char* label){bool threw=false;try{dashPendingInitializationVersion(input);}catch(const std::exception&){threw=true;}check(threw,label);};
  value=init;value["initialization"]=Json::object();rejectsInit(value,"Already accepted initialization cannot acquire its first version");
  value=init;value["pendingInitializationByteRange"]={{"start",0},{"length",1}};rejectsInit(value,"Initialization ranges retain preflight validation");
  value=init;value.erase("pendingInitialization");rejectsInit(value,"A dangling initialization policy is invalid");
  value=init;value["pendingInitialization"]=Json::array();rejectsInit(value,"A deferred initialization requires actual locations");
  Json index={{"schema",2},{"tracks",Json::array({Json{{"segments",{{"1",pending}}}}})}};dashPendingVersionSchema(index);check(true,"Schema 2 admits a structurally valid pending intent");
  auto rejectsSchema=[&](Json input,const char* label){bool threw=false;try{dashPendingVersionSchema(input);}catch(const std::exception&){threw=true;}check(threw,label);};
  value=index;value["schema"]=1;rejectsSchema(value,"Pending resource semantics cannot be smuggled into a legacy schema");
  value=index;value["schema"]=3;rejectsSchema(value,"Unknown recording policies fail explicitly");
  value={{"schema",1},{"tracks",Json::array({Json{{"segments",Json::object()}}})}};dashPendingVersionSchema(value);check(true,"Legacy schema remains readable without new policy fields");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";return error.empty()?0:1;
}
