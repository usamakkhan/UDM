#pragma once
#include "Core.hpp"
namespace udm {
bool canPreviewDownload(const Json&);
// The caller holds Manager::mutex while reading live job and track records.
bool canPreviewDownload(const Job&,bool draftLogin);
std::string previewMimeType(const std::string&);
// Informational metadata only: never a resume validator or an instruction to rename.
Json probeDownload(const Json&,const Json&,const Cancel&,std::function<void(const Json&)> saveBrowserSession={});
class DownloadPreview {
 struct State;
 std::shared_ptr<State> state;
public:
 DownloadPreview(Json download,Json preferences,int timeoutMs=10000,Manager* sessionManager=nullptr,JobPtr sessionJob={});
 ~DownloadPreview();
 DownloadPreview(const DownloadPreview&)=delete;DownloadPreview& operator=(const DownloadPreview&)=delete;
 Json snapshot()const;
 void stop()noexcept;
};
// Ephemeral request context for a preview. This is never serialized to the catalog.
struct BatchPreviewRequest {
 std::string address;
 Headers headers;
 std::string protectedOverrides;
 Json proxy=Json::object(),session=Json::object();
};
Headers batchRequestHeaders(Headers headers,const std::string& protectedOverrides);
// Bounded metadata-only work. Workers own their state, never a dialog or Manager.
class BatchDownloadPreview {
 struct State;std::shared_ptr<State> state;
public:
 BatchDownloadPreview(std::vector<BatchPreviewRequest> requests,Json preferences);
 ~BatchDownloadPreview();
 BatchDownloadPreview(const BatchDownloadPreview&)=delete;
 BatchDownloadPreview& operator=(const BatchDownloadPreview&)=delete;
 void replace(size_t index,BatchPreviewRequest request);
 std::vector<std::pair<size_t,Json>> take();
};
}
