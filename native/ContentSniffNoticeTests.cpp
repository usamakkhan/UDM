#include "Core.hpp"
#include "ContentSniff.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){
 Json checks=Json::array();bool passed=false;std::string error;
 const auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});if(!ok)throw std::runtime_error(name);};
 const auto sniff=[](const std::string& text,bool complete=true){return sniffContent(reinterpret_cast<const uint8_t*>(text.data()),text.size(),complete);};
 try{
  const std::string pdf="%PDF-1.7\nASCII-only file content\n%%EOF\n";
  for(const auto& notice:std::vector<std::string>{"<html><body>Preparing download</body></html>\n","<HTML><BODY>Preparing download</BODY></HTML>\r\n\t","<body>Preparing download</body>\n"}){
   for(const auto& file:std::vector<std::string>{pdf,"%PDF-2.0\r\nASCII file\n%%EOF\n",std::string("\xef\xbb\xbf")+pdf}){
    const auto result=sniff(notice+file);check(result.verdict==SniffVerdict::MatchAfterPrefix&&result.extension=="pdf"&&result.prefixLength==notice.size(),"ASCII PDF after a completed HTML notice reports its exact file offset");
   }
  }
  const std::string notice="<html><body>Preparing download</body></html>\n";
  auto result=sniff(notice+pdf+std::string(contentSniffWindow,'x'),false);check(result.verdict==SniffVerdict::MatchAfterPrefix&&result.prefixLength==notice.size(),"A complete leading window identifies a prefixed ASCII PDF before response EOF");
  for(const auto& text:std::vector<std::string>{"<html><body><pre>"+pdf+"</pre></body></html>","<html><script>const sample='"+pdf+"';</script></html>","<html><body>Unclosed notice\n"+pdf,notice+"Example: "+pdf,notice+"%PDF-A.7\n",notice+"%PDF-17\n",notice+"%PDF-1.7 examples\n",notice+"%PDF-1.",notice+std::string(2001,' ')+pdf}){
   check(sniff(text).verdict==SniffVerdict::Html,"HTML examples, malformed headers and notices beyond the prefix limit remain HTML");
  }
  check(sniff(pdf).verdict==SniffVerdict::Match&&sniff(pdf).prefixLength==0,"A normal ASCII PDF remains a direct PDF match");
  check(sniff(notice).verdict==SniffVerdict::Html,"An ordinary HTML notice remains HTML");
  check(sniff("Plain text notice\n"+pdf).verdict==SniffVerdict::Text,"Unstructured textual examples are not guessed to be prefixed PDFs");
  std::string binary="PK\x03\x04";binary+=std::string(300,'\0');result=sniff(notice+binary);check(result.verdict==SniffVerdict::MatchAfterPrefix&&result.extension=="zip"&&result.prefixLength==notice.size(),"Existing binary archive prefix recognition remains intact");
  check(sniff(notice+pdf,false).verdict==SniffVerdict::NeedMore,"A short incomplete window retains its existing pending classification");
  passed=true;
 }catch(const std::exception& e){error=e.what();}
 if(argc>1){const auto root=fs::path(wide(argv[1]));fs::create_directories(root);atomicText(root/L"results.json",Json{{"passed",passed},{"error",error},{"checks",checks}}.dump(2),false);}
 std::cout<<checks.size()<<" checks; "<<(passed?"passed":error)<<"\n";return passed?0:1;
}
