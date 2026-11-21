#pragma once
#include "GuiModels.hpp"
#include "QueueLabels.hpp"
#include <string_view>
namespace udm {
// Request identity is separate from render metadata. Keep protected values private,
// and compare borrowed values under Manager::mutex instead of cloning plans again.
template<class Visitor> inline void visitFileInfoSelection(const Job& job,Visitor&& visit){
 const auto fields=[&](const Json& record,const char* prefix){
  for(const auto* key:{"Id","Url","SourceUrl","ProtectedHeaders","ProtectedRequest","ProtectedAdaptive","ProtectedSabr",
      "ProtectedResolvedUrl","ProtectedAdaptiveDisplayAddress","ProtectedBrowserSession","ProtectedProxy","ProtectedBrowserProxy","MediaOutput",
      "SabrTrackAddressBound","OfflineProject","LiveRecording"})
   if(auto found=record.find(key);found!=record.end()){
    // A direct response records an empty resolved address when no redirect
    // occurred. That is metadata, equivalent to the field being absent.
    if(std::string_view(key)=="ProtectedResolvedUrl"&&found->is_string()&&found->get_ref<const std::string&>().empty())continue;
    visit(std::string(prefix)+key,*found);
   }
 };
 fields(job.data,"");
 // Presence is significant even before a track URL has been resolved.
 if(job.video){visit("Video",Json(true));fields(job.video->data,"Video.");}
 if(job.audio){visit("Audio",Json(true));fields(job.audio->data,"Audio.");}
}
inline Json fileInfoSelection(const Job& job){
 Json result=Json::object();visitFileInfoSelection(job,[&](const std::string& key,const Json& value){result[key]=value;});return result;
}
inline bool fileInfoSelectionUnchanged(const Job& job,const Json& selection){
 if(!selection.is_object()||selection.empty())return false;
 bool same=true;size_t count=0;
 visitFileInfoSelection(job,[&](const std::string& key,const Json& value){++count;const auto found=selection.find(key);if(found==selection.end()||*found!=value)same=false;});
 return same&&count==selection.size();
}
// Owned render inputs, with no JobPtr, Manager reference or mutable live JSON.
// Protected address inputs retain the existing display-address precedence; this
// private in-process value must never be serialized to diagnostics or the wire.
struct FileInfoInitialView {
 const std::string jobId;
 const Json data,preferences,addressInputs;
 const fs::path target;
 const i64 savedSize;
 const bool media,directResponse,formRequest,trimResponse;
 const std::vector<std::string> categories;
 // Private receipt ownership, not a control value or serialized UI payload.
 const std::string admissionFingerprint;
 const Json selection;
 const std::vector<std::string> queues;
 const QueueLabels queueLabels;
 FileInfoInitialView(std::string id,Json record,Json prefs,Json address,fs::path path,i64 saved,
                     bool isMedia,bool direct,bool form,bool trim,std::vector<std::string> keys,std::string fingerprint={},Json requestSelection=Json::object(),std::vector<std::string> queueKeys={},QueueLabels labels={}):
  jobId(std::move(id)),data(std::move(record)),preferences(std::move(prefs)),addressInputs(std::move(address)),target(std::move(path)),
  savedSize(saved),media(isMedia),directResponse(direct),formRequest(form),trimResponse(trim),categories(std::move(keys)),admissionFingerprint(std::move(fingerprint)),selection(std::move(requestSelection)),queues(std::move(queueKeys)),queueLabels(std::move(labels)){}
};
inline std::shared_ptr<const FileInfoInitialView> captureFileInfoInitialView(Manager& manager,const Job& job,std::string fingerprint={}){
 Lock lock(manager.mutex);const auto& source=job.data;Json data=Json::object(),prefs=Json::object();
 for(const auto* key:{"Url","SourceUrl","FileName","Category","Size","HttpContentEncoding","HttpDecodedSize","HttpDecodedReceived",
     "Status","LastHttpStatus","RefreshPendingValidation","BrowserReportedDirectSize","EstimatedMediaSize","ReportedMediaSize","ContentType","ResponseFileNameUrl",
     "Description","DownloadPage","Queue","ProtectedHeaders","FormatDescription","BrowserVerificationRequired","SabrSessionRefreshRequired","SabrRetainedBytes","StreamCompletedMs",
     "ConfirmationPending","ReplacementOf","ResponseFileName","BrowserReportedFileName"})
  if(auto found=source.find(key);found!=source.end())data[key]=*found;
 const bool media=hasNonemptyString(source,"ProtectedAdaptive")||hasNonemptyString(source,"SourceUrl");
 if(job.video)data["Video"]={{"Size",num(job.video->data,"Size",-1)}};
 if(job.audio)data["Audio"]={{"Size",num(job.audio->data,"Size",-1)}};
 const auto& settings=manager.state["Settings"];
 for(const auto* key:{"SaveFolderHistory","CategoryRememberLast","DownloadFolder","CategoryPaths","CategoryFolders","IdmCategoryDefaults"})if(auto found=settings.find(key);found!=settings.end())prefs[key]=*found;
 if(!media)if(auto found=settings.find("SiteLogins");found!=settings.end())prefs["SiteLogins"]=*found;
 const bool direct=!media&&!source.contains("OfflineProject");
 std::vector<std::string> queues;for(const auto& queue:manager.state["Queues"])queues.push_back(str(queue,"Name"));
 return std::make_shared<const FileInfoInitialView>(job.id(),std::move(data),std::move(prefs),downloadAddressInputs(job),job.target(),savedMediaOutputSize(source),
  media,direct,hasNonemptyString(source,"ProtectedRequest"),direct&&mayStripResponsePrefix(source),manager.categories(),std::move(fingerprint),fileInfoSelection(job),std::move(queues),queueDisplayLabels(manager.state["Queues"]));
}
// Rendering-cache equality only. Receipt ownership is checked by the session.
inline bool sameFileInfoInitialView(const FileInfoInitialView& a,const FileInfoInitialView& b){
 return a.jobId==b.jobId&&a.data==b.data&&a.preferences==b.preferences&&a.addressInputs==b.addressInputs&&a.target==b.target&&
  a.savedSize==b.savedSize&&a.media==b.media&&a.directResponse==b.directResponse&&a.formRequest==b.formRequest&&a.trimResponse==b.trimResponse&&a.categories==b.categories&&a.queues==b.queues&&a.queueLabels==b.queueLabels;
}
}
