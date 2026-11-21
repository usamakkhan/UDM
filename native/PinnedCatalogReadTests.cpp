#include "PinnedCatalogRead.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 3;
 fs::create_directories(root);Json checks=Json::array();std::string error;
 const auto check=[&](bool value,const char* name){checks.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);};
 const auto rejected=[&](HANDLE file,size_t limit){try{readPinnedCatalogText(file,limit);return false;}catch(const std::exception&){return true;}};
 try{
  const auto path=root/L"state.json";const std::string original="{\"proof\":\"original\"}";
  atomicText(path,original,false);
  {
   Handle pinned(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr));
   check(bool(pinned),"The proof fixture obtains the actual nonsharing read pin");
   check(readPinnedCatalogText(pinned.h,original.size())==original,"Exact-limit pinned read returns complete original bytes");
   Handle writer(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
   check(!writer,"The production pin excludes concurrent writers through comparison");
   check(!MoveFileExW(path.c_str(),(root/L"forbidden.json").c_str(),0),"The production pin excludes pathname retirement through comparison");
   check(rejected(pinned.h,original.size()-1),"Oversized catalog is refused before content allocation");
   check(readPinnedCatalogText(pinned.h,original.size())==original,"Repeated pinned reads reset position rather than accepting an empty EOF");
  }
  {
   // Deliberately allow rename only in this object-identity control. Production
   // callers above keep stricter sharing throughout their proof.
   Handle pinned(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr));
   check(bool(pinned)&&MoveFileExW(path.c_str(),(root/L"original.json").c_str(),0),"Identity control retains its open object across an allowed rename");
   atomicText(path,"{\"proof\":\"foreign\"}",false);
   check(readPinnedCatalogText(pinned.h,original.size())==original&&readText(path)!=original,
         "Pinned reader cannot silently reopen a replacement pathname");
  }
  for(const auto content:{std::string(),std::string("\xEF\xBB\xBF")+"{}",std::string("\xEF"),std::string(1048576+19,'x')}){
   const auto fixture=root/(wide(guid())+L".json");atomicText(fixture,content,false);
   Handle pinned(CreateFileW(fixture.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
   const auto expected=content.rfind("\xEF\xBB\xBF",0)==0?content.substr(3):content;
   check(bool(pinned)&&readPinnedCatalogText(pinned.h,content.size())==expected,
         "Empty, UTF-8 BOM, partial BOM and multi-chunk inputs retain existing text semantics");
  }
  {
   Handle writeOnly(CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr));
   check(bool(writeOnly)&&rejected(writeOnly.h,1024),"A readable-size handle without read access cannot prove checkpoint content");
   check(rejected(INVALID_HANDLE_VALUE,1024),"Invalid handles fail conservatively");
   Handle directory(CreateFileW(root.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr));
   check(bool(directory)&&rejected(directory.h,1024),"Directory handles cannot stand in for regular checkpoint files");
  }
  {
   HANDLE read=nullptr,write=nullptr;check(CreatePipe(&read,&write,nullptr,0)!=FALSE,"Nondisk control creates an owned anonymous pipe");
   Handle reader(read),writer(write);
   check(rejected(reader.h,1024),"Nondisk handles are refused before a potentially blocking read");
  }
 }catch(const std::exception& exception){error=exception.what();}
 atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);
 std::cout<<checks.size()<<" checks; "<<(error.empty()?"passed":error)<<"\n";return error.empty()?0:1;
}
