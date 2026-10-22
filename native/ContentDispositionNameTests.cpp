#include "ContentDispositionName.hpp"
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);
 Json checks=Json::array();auto check=[&](bool ok,const char* name){checks.push_back({{"name",name},{"passed",ok}});};
 const std::string fallback="download";
 check(contentDispositionName("attachment; filename*=UTF-8''guide%20book.pdf",fallback)=="guide book.pdf","Extended UTF-8 name is decoded");
 check(contentDispositionName("attachment; filename=old.zip; filename*=UTF-8'en'new%20name.iso",fallback)=="new name.iso","Extended name takes precedence over plain name");
 check(contentDispositionName("attachment; filename=\"folder/part;two.iso\"",fallback)=="part;two.iso","Quoted semicolon and server path are handled");
 check(contentDispositionName("attachment; filename=good.iso; filename*=UTF-8''bad%ZZ.iso",fallback)=="good.iso","Malformed extended escape falls back to plain name");
 check(contentDispositionName("attachment; filename=a.iso; filename=b.exe",fallback)==fallback,"Ambiguous duplicate names retain URL fallback");
 check(contentDispositionName("attachment; filename=ok.iso\r\nX-Bad: yes",fallback)==fallback,"Injected header line retains URL fallback");
 check(contentDispositionName("",fallback)==fallback,"Absent server name retains URL fallback");
 check(contentDispositionName("attachment; filename*=ISO-8859-1''caf%E9.pdf",fallback)==u8"caf\u00e9.pdf","Legacy Latin-1 extended name converts to UTF-8");
 check(contentDispositionName("attachment; filename=old.pdf; filename*=iso-8859-1'fr'%A3%20r%E9sum%E9.pdf",fallback)==u8"\u00a3 r\u00e9sum\u00e9.pdf","Latin-1 extended name overrides fallback and supports language");
 check(contentDispositionName("attachment; filename=good.pdf; filename*=ISO-8859-1''bad%ZZ.pdf",fallback)=="good.pdf","Malformed Latin-1 escapes preserve plain filename");
 check(contentDispositionName("attachment; filename=good.pdf; filename*=ISO-8859-1''bad%00.pdf",fallback)=="good.pdf","Encoded NUL cannot replace plain filename");
 check(contentDispositionName("attachment; filename*=ISO-8859-1''folder%2Fcaf%E9.pdf",fallback)==u8"caf\u00e9.pdf","Latin-1 server path is stripped after decoding");
 check(contentDispositionName("attachment; filename=good.pdf; filename*=unsupported''caf%E9.pdf",fallback)=="good.pdf","Unsupported charset retains explicit fallback");
 check(contentDispositionName("attachment; filename*=ISO-8859-1''a+b%25.pdf",fallback)=="a+b%.pdf","Extended names preserve plus signs and decode percent once");
 bool passed=true;for(const auto& result:checks)passed&=yes(result,"passed");
 atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks}}.dump(2),false);return passed?0:1;
}
