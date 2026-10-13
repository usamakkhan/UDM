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
 bool passed=true;for(const auto& result:checks)passed&=yes(result,"passed");
 atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks}}.dump(2),false);return passed?0:1;
}
