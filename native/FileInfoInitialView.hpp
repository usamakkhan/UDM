#pragma once
#include "GuiModels.hpp"
namespace udm {
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
 FileInfoInitialView(std::string id,Json record,Json prefs,Json address,fs::path path,i64 saved,
                     bool isMedia,bool direct,bool form,bool trim,std::vector<std::string> keys):
  jobId(std::move(id)),data(std::move(record)),preferences(std::move(prefs)),addressInputs(std::move(address)),target(std::move(path)),
  savedSize(saved),media(isMedia),directResponse(direct),formRequest(form),trimResponse(trim),categories(std::move(keys)){}
};
inline std::shared_ptr<const FileInfoInitialView> captureFileInfoInitialView(Manager& manager,const Job& job){
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
 return std::make_shared<const FileInfoInitialView>(job.id(),std::move(data),std::move(prefs),downloadAddressInputs(job),job.target(),savedMediaOutputSize(source),
  media,direct,hasNonemptyString(source,"ProtectedRequest"),direct&&mayStripResponsePrefix(source),manager.categories());
}
inline bool sameFileInfoInitialView(const FileInfoInitialView& a,const FileInfoInitialView& b){
 return a.jobId==b.jobId&&a.data==b.data&&a.preferences==b.preferences&&a.addressInputs==b.addressInputs&&a.target==b.target&&
  a.savedSize==b.savedSize&&a.media==b.media&&a.directResponse==b.directResponse&&a.formRequest==b.formRequest&&a.trimResponse==b.trimResponse&&a.categories==b.categories;
}
}
