#include "GrabberRenderedStore.hpp"
#include "OfflineSite.hpp"
#include "Catalog.hpp"
#include <iostream>
using namespace udm;
int main(int argc,char** argv){int checks=0;try{
 if(argc!=3&&argc!=4)throw std::runtime_error("URL, fresh fixture root and optional media flag required");auto root=fs::path(wide(argv[2]));if(fs::exists(root))throw std::runtime_error("Fixture exists");
 Manager manager(root);manager.state["Settings"]["ProxyMode"]="Connect directly";manager.state["Settings"]["CategoryFolders"]=false;manager.state["Settings"]["DownloadFolder"]=utf8((root/L"files").wstring());
 auto check=[&](bool ok){if(!ok)throw std::runtime_error("Rendered offline assertion "+std::to_string(checks));++checks;};
 const std::string url=argv[1],base=Url(url).origin+"/assets/";
 Json project={{"Id",guid()},{"Name","Rendered offline"},{"StartUrl",url},{"Template","Offline website (ZIP)"},{"Depth",1},{"MaxPages",10},{"MaxMiB",2},{"Links",Json::array()}};manager.saveProject(project);
 auto request=beginRenderedGrabberRequest(manager,str(project,"Id"));auto html="<!doctype html><html><head><base href='"+base+"'><link rel='stylesheet' href='style.css'></head><body><h1>Runtime-only heading</h1><picture><img id='responsive' src='"+base+"pixel.png'></picture><a href='next.html'>Next</a><section id='shadow'><template shadowrootmode='open'><style>h2 { color: rgb(12, 34, 56); background-image: url(css-only.png); }</style><h2>Saved shadow heading</h2><img src='pixel.png'><a href='next.html'>Shadow child</a></template></section><audio id='saved-audio' controls src='tone.wav'><track kind='captions' srclang='en' label='English' src='captions.vtt' default></audio></body></html>";
 if(argc==4)html.insert(html.find("</body>"),"<video id='saved-video' controls src='clip.mp4'><track kind='captions' srclang='en' src='captions.vtt' default></video>");
 Json snapshot={{"version",1},{"url",url},{"baseUrl",base},{"title","Rendered"},{"documentId","fixture"},{"tabId",1},{"frameId",0},{"visited",8},{"truncated",false},{"references",Json::array()},{"html",html},{"htmlUnavailable",false}};request["snapshot"]=snapshot;completeRenderedGrabberRequest(manager,request);
 // Oversized and policy-stale captures must fail before catalog mutation.
 auto baselineJobs=manager.jobs.size();auto originalRecords=manager.state["RenderedGrabber"];
 auto oversizedRequest=beginRenderedGrabberRequest(manager,str(project,"Id"));auto oversized=snapshot;oversized.erase("html");oversized["htmlUnavailable"]=true;oversizedRequest["snapshot"]=oversized;completeRenderedGrabberRequest(manager,oversizedRequest);
 bool oversizedRejected=false;try{manager.addOfflineProject(project,"Main queue",true);}catch(const std::exception& e){oversizedRejected=std::string(e.what()).find("too large")!=std::string::npos;}check(oversizedRejected);check(manager.jobs.size()==baselineJobs);
 manager.state["RenderedGrabber"]=originalRecords;manager.save();auto changedProject=project;changedProject["Depth"]=2;
 bool staleRejected=false;try{manager.addOfflineProject(changedProject,"Main queue",true);}catch(const std::exception& e){staleRejected=std::string(e.what()).find("settings changed")!=std::string::npos;}check(staleRejected);check(manager.jobs.size()==baselineJobs);
 auto job=manager.addOfflineProject(project,"Main queue",true);check(!str(job->data,"ProtectedRenderedPage").empty());check(job->data.dump().find("Runtime-only heading")==std::string::npos);check(str(job->data["OfflineProject"],"RenderedTicket")==str(request,"ticket"));
 manager.state.erase("RenderedGrabber");manager.save();Manager reopened(root);auto restored=reopened.jobs.front();check(restored->data["OfflineProject"]==job->data["OfflineProject"]);
 // Interrupt during the first image response, after HTML and CSS are durable.
 auto interrupted=std::make_shared<Cancel>();std::atomic_bool finished{false};
 std::thread pauser([&]{while(!finished){if(fs::exists(root/L"pause-now")){interrupted->stop=true;return;}std::this_thread::sleep_for(std::chrono::milliseconds(5));}});
 bool paused=false;std::exception_ptr pauseFailure;
 try{offlineTransfer(reopened,restored,interrupted);}catch(const Cancelled&){paused=true;}catch(...){pauseFailure=std::current_exception();}
 finished=true;pauser.join();if(pauseFailure)std::rethrow_exception(pauseFailure);
 check(paused);check(!fs::exists(restored->target()));check(fs::exists(root/L"parts"/wide(restored->id())/L"offline"/L"cache.json"));reopened.save();
 Manager resumed(root);auto resumedJob=resumed.jobs.front();check(str(resumedJob->data,"ProtectedRenderedPage")==str(restored->data,"ProtectedRenderedPage"));
 auto cancel=std::make_shared<Cancel>();offlineTransfer(resumed,resumedJob,cancel);restored->data=resumedJob->data;check(str(restored->data,"Status")=="Complete");check(fs::is_regular_file(restored->target()));check(num(restored->data,"OfflinePages")==2);check(restored->data["OfflineErrors"].empty());
 auto saved=restored->data;restored->data.erase("ProtectedRenderedPage");bool rejected=false;try{offlineTransfer(reopened,restored,cancel);}catch(...){rejected=true;}check(rejected);restored->data=saved;
 auto originalTarget=restored->target();auto originalHash=fileHash(originalTarget);auto again=reopened.redownload(restored);
 check(again->data["OfflineProject"]==restored->data["OfflineProject"]);check(str(again->data,"ProtectedRenderedPage")==str(restored->data,"ProtectedRenderedPage"));check(again->target()!=originalTarget);check(str(again->data,"RedownloadOf")==restored->id());
 offlineTransfer(reopened,again,cancel);check(str(again->data,"Status")=="Complete");check(fileHash(originalTarget)==originalHash);
 auto countBefore=reopened.jobs.size();restored->data.erase("ProtectedRenderedPage");bool missingRejected=false;try{reopened.redownload(restored);}catch(...){missingRejected=true;}check(missingRejected&&reopened.jobs.size()==countBefore);restored->data=saved;
 auto ordinaryProject=project;ordinaryProject["Id"]=guid();ordinaryProject["Name"]="Ordinary offline";ordinaryProject["StartUrl"]=Url(url).origin+"/ordinary";
 auto ordinary=reopened.addOfflineProject(ordinaryProject,"Main queue",true);check(!ordinary->data.contains("ProtectedRenderedPage"));offlineTransfer(reopened,ordinary,cancel);check(str(ordinary->data,"Status")=="Complete");check(num(ordinary->data,"OfflinePages")==1);
 auto exported=exportCatalog(reopened,{restored});check(!exported["Downloads"][0].contains("ProtectedRenderedPage"));check(exported.dump().find("Runtime-only heading")==std::string::npos);
 Manager imported(root/L"imported");check(importCatalog(imported,exported,utf8((root/L"imported-files").wstring()))==1);auto importedJob=imported.jobs.front();check(str(importedJob->data,"Status")=="Needs browser capture");check(!yes(importedJob->data,"QueueMember"));check(str(importedJob->data,"Error").find("captured page")!=std::string::npos);
 bool importedRejected=false;try{offlineTransfer(imported,importedJob,cancel);}catch(...){importedRejected=true;}check(importedRejected&&!fs::exists(importedJob->target()));
 Manager attached(root/L"attached");check(importCatalog(attached,exported,"",false,true)==1);check(str(attached.jobs.front()->data,"Status")=="Complete");check(attached.jobs.front()->target()==originalTarget);check(fileHash(originalTarget)==originalHash);
 auto ordinaryCatalog=exportCatalog(reopened,{ordinary});Manager ordinaryImport(root/L"ordinary-import");check(importCatalog(ordinaryImport,ordinaryCatalog,utf8((root/L"ordinary-import-files").wstring()))==1);check(str(ordinaryImport.jobs.front()->data,"Status")=="Paused"&&yes(ordinaryImport.jobs.front()->data,"QueueMember"));
 Manager recapture(root/L"recapture");recapture.saveProject(project);
 auto firstRequest=beginRenderedGrabberRequest(recapture,str(project,"Id"));firstRequest["snapshot"]=snapshot;completeRenderedGrabberRequest(recapture,firstRequest);
 check(recapture.addProject(project,"Main queue",true)==1);auto firstJob=recapture.jobs.front();auto firstData=firstJob->data;
 check(recapture.addProject(project,"Main queue",true)==0);
 auto nextRequest=beginRenderedGrabberRequest(recapture,str(project,"Id"));auto nextSnapshot=snapshot;nextSnapshot["html"]=html+"<!-- refreshed capture -->";nextRequest["snapshot"]=nextSnapshot;completeRenderedGrabberRequest(recapture,nextRequest);
 check(recapture.addProject(project,"Main queue",true)==1);check(recapture.jobs.size()==2);check(firstJob->data==firstData);
 auto nextJob=recapture.jobs.back();check(nextJob->target()!=firstJob->target());check(str(nextJob->data["OfflineProject"],"RenderedTicket")==str(nextRequest,"ticket"));
 check(recapture.addProject(project,"Main queue",true)==0);
 atomicText(root/L"results.json",Json{{"passed",true},{"checks",checks},{"archive",utf8(restored->target().wstring())},{"ordinaryArchive",utf8(ordinary->target().wstring())},{"redownloadArchive",utf8(again->target().wstring())}}.dump(2),false);std::cout<<checks<<" rendered offline checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
