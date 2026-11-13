#pragma once
#include "Core.hpp"
#include "PostResponse.hpp"
#include "MediaSize.hpp"
#include "StreamProgress.hpp"
namespace udm {
struct ProgressRequestState {
 bool form=false,redirect=false,saved=false;
};
inline ProgressRequestState progressRequestState(const Json& source){
 return {hasNonemptyString(source,"ProtectedRequest"),hasNonemptyString(source,"ProtectedPostGetUrl"),completedPostResponseCandidate(source)};
}
struct ProgressRecordView {Json data=Json::object();ProgressRequestState request;};
// Called under the manager lock. Only display data leaves this boundary; live
// request bodies, protected context and recovery journals remain on the job.
inline ProgressRecordView progressRecordView(const Json& source,bool connections){
 ProgressRecordView view;view.request=progressRequestState(source);
 const auto copySummary=[&](const char* key,std::initializer_list<const char*> fields){
  const auto value=source.find(key);if(value==source.end()||!value->is_object())return;
  Json summary=Json::object();for(const auto* field:fields)if(auto f=value->find(field);f!=value->end())summary[field]=*f;
  view.data[key]=std::move(summary);
 };
 for(auto it=source.begin();it!=source.end();++it){
  const auto& key=it.key();
  if(key.rfind("Protected",0)==0||key=="Video"||key=="Audio"||key=="OfflineProject"||
     key=="CompletedPrefetch"||key=="OriginalResponse"||key=="RetainedMediaOutput")continue;
  if(!connections&&(key=="Segments"||key=="SabrRanges"))continue;
  view.data[key]=it.value();
 }
 copySummary("CompletedPrefetch",{"Size"});
 copySummary("OriginalResponse",{"State"});
 // A refresh offer invalidates the old output's displayed size, even though
 // its protected payload is intentionally absent from the presentation copy.
 if(hasNonemptyString(source,"ProtectedRefreshOffer"))copySummary("RetainedMediaOutput",{"Version","Kind"});
 else copySummary("RetainedMediaOutput",{"Version","Kind","Size","Sha256"});
 return view;
}
inline bool capturedResponseSavePending(const Json& record){
 if(str(record,"Status")=="Complete")return false;
 const auto response=record.find("OriginalResponse");
 return response!=record.end()&&(str(*response,"State")=="Ready"||str(*response,"State")=="Publishing");
}
inline std::string progressResumeCapability(const Json& record,const ProgressRequestState* context=nullptr){
 if(capturedResponseSavePending(record))return "Response saved";
 const auto request=context?*context:progressRequestState(record);
 if(request.saved)return "Response saved";
 if(yes(record,"PostResponseComplete"))return "No";
 if(request.form&&!request.redirect)return "No";
 if(savedMediaOutputSize(record)>=0)return "Saved output";
 if(yes(record,"MediaTracksReady"))return "Streams saved";
 if(yes(record,"AdaptiveRangeResume")&&adaptiveSourceBytes(record)>0)return "Yes (saved ranges)";
 if(yes(record,"SabrResumeSupported")||num(record,"AdaptiveTotalSegments")>0)return "Yes (saved segments)";
 const auto capability=[](const Json& item){return item.contains("RangeSupported")&&item["RangeSupported"].is_boolean()?(yes(item,"RangeSupported")?1:0):-1;};
 bool tracks=false,unknown=false;
 for(auto name:{"Video","Audio"})if(record.contains(name)&&record[name].is_object()){
  tracks=true;const auto support=capability(record[name]);if(support==0)return "No";unknown|=support<0;
 }
 if(tracks)return unknown?"Unknown":"Yes";
 const auto support=capability(record);return support<0?"Unknown":support?"Yes":"No";
}
inline bool progressNeedsFormRestart(const Json& record,const ProgressRequestState* context=nullptr){
 const auto request=context?*context:progressRequestState(record);const auto state=str(record,"Status");
 return (state=="Paused"||state=="Failed")&&yes(record,"PostAttempted")&&!request.saved&&request.form&&(!request.redirect||yes(record,"PostResponseComplete"));
}
}
