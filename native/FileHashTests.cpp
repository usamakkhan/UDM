#include "Core.hpp"
#include <iostream>
using namespace udm;

int wmain(int argc,wchar_t** argv){
 if(argc!=2||fs::exists(argv[1]))return 2;
 auto root=fs::absolute(argv[1]);fs::create_directories(root);
 Json checks=Json::array();std::string error;
 auto check=[&](bool passed,const char* name){checks.push_back({{"name",name},{"passed",passed}});if(!passed)throw std::runtime_error(name);};
 try{
  auto path=root/L"saved.part";writeBytes(path,Bytes(3*65536+7,'x'));
  auto expected=fileHash(path);int blocks=0;
  check(fileHash(path,[&]{++blocks;})==expected&&blocks>=4,"Checkpointed SHA-256 preserves the file digest");
  blocks=0;bool cancelled=false;
  try{fileHash(path,[&]{if(++blocks==2)throw Cancelled();});}catch(const Cancelled&){cancelled=true;}
  check(cancelled&&blocks==2&&fileHash(path)==expected,"Pause interrupts hashing after a completed block without changing saved bytes");
 }catch(const std::exception& e){error=e.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<error<<std::endl;return error.empty()?0:1;
}
