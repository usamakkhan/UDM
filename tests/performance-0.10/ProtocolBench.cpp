#include "../../native/Core.hpp"
#include <iostream>
using namespace udm;
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)throw std::runtime_error("Use: ProtocolBench input.json unique-run-label");
 auto input=Json::parse(readText(argv[1]));auto root=fs::absolute(argv[1]).parent_path();auto label=utf8(argv[2]);
 if(label.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-_")!=std::string::npos)throw std::runtime_error("Invalid run label.");
 auto address=str(input,"url");Url parsed(address);if(parsed.scheme!="http"||parsed.host!="127.0.0.1")throw std::runtime_error("This benchmark accepts only its local HTTP fixture.");
 Manager manager(root/wide("state-"+label));manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"downloads").wstring());
 auto job=manager.add(address,"",label+".bin","Main queue",true,{},str(input,"sha256"));job->data["Connections"]=8;
 auto start=std::chrono::steady_clock::now();transfer(manager,job,std::make_shared<Cancel>());
 Json report={{"label",label},{"seconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()},{"networkSeconds",real(job->data,"TransferSeconds")},{"bytes",num(job->data,"Received")},{"splits",num(job->data,"DynamicSplits")},{"sha256",str(job->data,"Sha256")},{"completedUtc",epoch()}};
 atomicText(root/wide(label+"-result.json"),report.dump(2));std::cout<<report.dump()<<std::endl;
 }catch(const std::exception& error){std::cerr<<error.what()<<std::endl;return 1;}return 0;}
