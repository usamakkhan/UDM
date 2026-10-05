#include "Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){
 if(argc!=2)return 2;fs::path root=argv[1];if(fs::exists(root))return 3;fs::create_directories(root);Json checks=Json::array();
 auto check=[&](bool ok,const std::string& name){checks.push_back({{"name",name},{"passed",ok}});};
 struct Case{const char* name;bool sync;bool member;const char* field;const char* value;bool expected;};
 const Case cases[]={{"ordinary",false,true,"","",false},{"synchronization",true,true,"","",true},{"explicitly-removed",true,false,"","",false},{"adaptive",true,true,"ProtectedAdaptive","fixture",false},{"post",true,true,"ProtectedRequest","fixture",false},{"page-media",true,true,"SourceUrl","https://example.invalid/watch",false},{"archived",true,true,"PreviousVersionOf","previous",false},{"ftp",true,true,"Url","ftp://example.invalid/file.bin",true},{"ftp-ordinary",false,true,"Url","ftp://example.invalid/file.bin",false},{"ftp-explicitly-removed",true,false,"Url","ftp://example.invalid/file.bin",false}};
 try{fs::create_directories(root/L"files");for(const auto& item:cases){Manager m(root/wide(item.name));auto prefs=m.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"files").wstring());prefs["CategoryFolders"]=false;m.setSettings(prefs);auto q=defaultQueue("Publication");q["Synchronize"]=item.sync;q["Enabled"]=false;m.setQueue(q);auto job=m.add("https://example.invalid/file.bin","",std::string(item.name)+".bin","Publication",true);job->data["QueueMember"]=item.member;if(*item.field)job->data[item.field]=item.value;auto staging=root/(wide(item.name)+L".part");atomicText(staging,"publication bytes",false);m.publishFile(job,staging,fileHash(staging));check(str(job->data,"Status")=="Complete"&&yes(job->data,"QueueMember")==item.expected&&readText(job->target())=="publication bytes",std::string(item.name)+": exact publication bytes and intended membership");Manager reopened(m.root);check(reopened.jobs.size()==1&&yes(reopened.jobs[0]->data,"QueueMember")==item.expected,std::string(item.name)+": durable membership after reload");}
 }catch(const std::exception& e){check(false,std::string("Unexpected fixture error: ")+e.what());}
 bool passed=true;for(auto& c:checks)passed&=yes(c,"passed");atomicText(root/L"results.json",Json{{"passed",passed},{"checks",checks}}.dump(2),false);std::cout<<checks.dump(2)<<std::endl;return passed?0:1;
}
