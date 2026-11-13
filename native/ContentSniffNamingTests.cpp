#include "Core.hpp"
#include "ContentSniff.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){
 Json checks=Json::array();bool passed=false;std::string failure;
 auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 try{
  Bytes body(64,0);body[0]='P';body[1]='K';body[2]=3;body[3]=4;
  const auto archive=sniffContent(body.data(),body.size(),true);
  check(archive.verdict==SniffVerdict::Match&&archive.extension=="zip","Fixture identifies actual ZIP bytes");
  for(const auto* name:{"old.udmfixture","package.unitypackage","model.safetensors","app.application","file.longcustomextension","file.UDMFIXTURE"})
   check(correctedFileName(name,archive).empty(),name);
  check(correctedFileName("file."+std::string(100,'a'),archive).empty(),"Long explicit suffix stays within the existing filename instead of gaining another extension");
  check(correctedFileName("file.docx",archive).empty(),"Existing short explicit suffix is retained");
  check(correctedFileName("file.zip",archive).empty(),"Matching suffix is retained");
  check(correctedFileName("file.bin",archive)=="file.zip","Generic binary suffix still follows recognized bytes");
  check(correctedFileName("download.php",archive)=="download.zip","Script endpoint suffix still follows recognized bytes");
  check(correctedFileName("download",archive)=="download.zip","Missing suffix still gains the recognized type");
  check(correctedFileName("package.crdownload",archive)=="package.zip","Long known temporary suffix is corrected using the existing weak-suffix policy");
  check(correctedFileName("release.20261111",archive)=="release.20261111.zip","Numeric version suffix remains distinct from an explicit type");
  check(correctedFileName("",archive).empty(),"Empty names are unchanged");
  auto unknown=sniffContent(nullptr,0,true);
  check(correctedFileName("file.longcustomextension",unknown).empty(),"Unknown content never appends an invented extension");
  passed=true;
 }catch(const std::exception& error){failure=error.what();}
 if(argc>1){const auto root=fs::path(wide(argv[1]));fs::create_directories(root);atomicText(root/L"results.json",Json{{"passed",passed},{"error",failure},{"checks",checks}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":failure)<<"\n";return passed?0:1;
}
