#include "Streaming.hpp"
#include "GuiModels.hpp"
#include "StreamProgress.hpp"
#include "MediaStorage.hpp"
#include <iostream>
using namespace udm;
static Json checks=Json::array();
static void check(bool ok,const std::string& name){checks.push_back({{"passed",ok},{"name",name}});if(!ok)throw std::runtime_error(name);}
template<class F>static void rejects(F action,const std::string& name){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,name);}
#include "MediaChecks.hpp"
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;const auto root=fs::absolute(argv[1]);if(fs::exists(root))return 2;fs::create_directories(root);WSADATA data{};WSAStartup(MAKEWORD(2,2),&data);CoInitializeEx(nullptr,COINIT_MULTITHREADED);std::string error;
try{Manager manager(root/L"catalog");manager.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());manager.state["Settings"]["CategoryFolders"]=false;mediaChecks(manager,root);manager.stop();Cancel cancel;
for(auto job:manager.jobs)if(str(job->data,"Status")=="Complete"&&fs::is_regular_file(job->target())&&(job->target().extension()==L".mp4"||job->target().extension()==L".m4a")){execute(appDir()/L"tools/ffmpeg.exe",{L"-hide_banner",L"-loglevel",L"error",L"-xerror",L"-nostdin",L"-i",job->target().wstring(),L"-map",L"0",L"-f",L"null",L"-"},30,cancel);check(true,"Completed media decodes through EOF with no FFmpeg error");}
}catch(const std::exception& e){error=e.what();}atomicText(root/L"results.json",Json{{"passed",error.empty()},{"error",error},{"checks",checks}}.dump(2),false);std::cout<<checks.size()<<" checks; "<<error<<"\n";CoUninitialize();WSACleanup();return error.empty()?0:1;}
