#include "Core.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("URL and fresh fixture directory required");
 fs::path root=wide(argv[2]);if(fs::exists(root))throw std::runtime_error("Fixture exists");
 Manager manager(root);manager.state["Settings"]["ProxyMode"]="Connect directly";
 auto job=manager.add(argv[1],utf8((root/L"files").wstring()),"Chosen name.mp3","Main queue",true);
 job->data["Connections"]=1;transfer(manager,job,std::make_shared<Cancel>());
 if(str(job->data,"Status")!="Complete"||!fs::exists(job->target()))throw std::runtime_error("Download failed");
 if(str(job->data,"FileName")!="Chosen name.mp3")throw std::runtime_error("User filename changed");
 auto metadata=job->data.at("AudioMetadata");
 if(str(metadata,"Title")!="Fixture title"||str(metadata,"Artist")!="Fixture artist"||str(metadata,"Album")!="Fixture album")throw std::runtime_error("Tag fields missing");
 manager.save();Manager reopened(root);if(reopened.jobs.front()->data.at("AudioMetadata")!=metadata)throw std::runtime_error("Metadata not durable");
 atomicText(root/L"result.json",Json{{"passed",true},{"file",utf8(job->target().wstring())},{"metadata",metadata},{"filenamePreserved",true},{"catalogReopen",true}}.dump(2),false);
 std::cout<<"PASS tagged MP3 transfer, chosen filename and catalog persistence\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
