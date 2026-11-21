#include "Core.hpp"
#include <iostream>
using namespace udm;
// The external runner owns the origin, payload digest and isolated output root.
int wmain(int argc,wchar_t** argv){
 if(argc!=5)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;
 fs::create_directories(root);Json checks=Json::array();std::string error;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  WSADATA wsa{};if(WSAStartup(MAKEWORD(2,2),&wsa))throw std::runtime_error("WSA startup");
  const auto url=utf8(argv[2]),expected=utf8(argv[3]),mode=utf8(argv[4]);
  if(mode!="plain"&&mode!="verified"&&mode!="mismatch"&&mode!="ranged")throw std::runtime_error("Unknown fixture mode");
  Manager manager(root/L"catalog");auto prefs=manager.state["Settings"];
  prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;
  prefs["ProxyMode"]="Connect directly";prefs["Retries"]=0;prefs["StopOnWebPage"]=false;
  prefs["CorrectFileExtensions"]=false;prefs["PrefetchFileInfo"]=false;manager.setSettings(prefs);
  auto job=manager.add(url,"","payload.bin","Main queue",true);job->data["Connections"]=8;
  if(mode=="verified")job->data["ExpectedSha256"]=expected;
  if(mode=="mismatch")job->data["ExpectedSha256"]=std::string(64,'0');
  bool rejected=false;std::string rejection;try{transfer(manager,job,std::make_shared<Cancel>());}catch(const std::exception& e){rejected=true;rejection=e.what();}
  if(mode=="mismatch"){
   check(rejected&&rejection.find("SHA-256 mismatch")!=std::string::npos,"A required digest mismatch is rejected by digest verification");
   check(!fs::exists(job->target())&&str(job->data,"Status")!="Complete","Digest rejection cannot publish or report completion");
  }else{
   check(!rejected&&str(job->data,"Status")=="Complete","Production HTTP transfer completes");
   check(fs::file_size(job->target())==8*1048576LL,"Large response preserves its exact byte count");
   check(fileHash(job->target())==expected&&str(job->data,"Sha256")==expected,"Output and recorded digest match the independent origin digest");
   const auto phases=job->data.value("FinishPhases",Json::object());
   check((str(phases,"HashMode")=="inline")==(mode!="ranged"),"Only proven sequential responses use the extended inline hash path");
   check(num(job->data,"Received")==num(job->data,"Size"),"Successful write accounting covers the complete representation");
  }
  manager.stop();
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
