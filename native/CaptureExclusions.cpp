#include "CaptureExclusions.hpp"
namespace udm {
void Manager::rememberAutomaticCapture(JobPtr job,const std::string& token,const std::string& address){
 Lock lock(mutex);
 // This state belongs only to the current app session and to accepted automatic
 // browser receipts. Manual links, media-panel clicks and restored history do not enter it.
 for(auto it=automaticCaptureOffers.begin();it!=automaticCaptureOffers.end();)if(std::none_of(jobs.begin(),jobs.end(),[&](auto j){return j->id()==it->first;}))it=automaticCaptureOffers.erase(it);else ++it;
 auto status=str(job->data,"Status");bool pending=status=="Awaiting confirmation"||status=="Awaiting duplicate choice"||std::find(pendingOffers.begin(),pendingOffers.end(),job)!=pendingOffers.end()||hasBrowserPresentation(job);
 if(!pending||!str(job->data,"ProtectedRefreshOffer").empty()){lastCancelledCaptureHost.clear();captureCancellationCount=0;return;}
 // Admission was already persisted. Optional prompt bookkeeping must never turn
 // an accepted browser handoff into an error/replay after the browser transfers ownership.
 try{auto canonical=exactCaptureAddress(address);Url url(canonical);if(automaticCaptureOffers.size()<2048||automaticCaptureOffers.count(job->id()))automaticCaptureOffers[job->id()]={{"token",token},{"address",canonical},{"host",url.host}};}catch(...){}
}
Json Manager::takeAutomaticCapture(JobPtr job){
 Lock lock(mutex);auto found=automaticCaptureOffers.find(job->id());if(found==automaticCaptureOffers.end())return Json::object();
 auto context=found->second;automaticCaptureOffers.erase(found);activeCaptureDecisions[str(context,"token")]=context;return context;
}
Json Manager::finishAutomaticCapture(const Json& context,bool cancelled){
 Lock lock(mutex);auto found=activeCaptureDecisions.find(str(context,"token"));if(found==activeCaptureDecisions.end()||found->second!=context)return Json::object();
 auto offer=found->second;activeCaptureDecisions.erase(found);
 if(!cancelled||!yes(state["Settings"],"OfferCaptureExclusions",true)){lastCancelledCaptureHost.clear();captureCancellationCount=0;return Json::object();}
 auto host=str(offer,"host");captureCancellationCount=host==lastCancelledCaptureHost?captureCancellationCount+1:1;lastCancelledCaptureHost=host;
 if(captureCancellationCount<2)return Json::object();
 // A declined prompt starts a fresh pair; it must not reappear on every later click.
 lastCancelledCaptureHost.clear();captureCancellationCount=0;return offer;
}
void Manager::applyCaptureExclusions(const Json& offer,bool site,bool address,bool suppress){
 Lock lock(mutex);auto next=captureExceptionSettings(state["Settings"],offer,site,address,suppress);setSettings(next);
}
}
