#pragma once
#include "Core.hpp"
namespace udm {
bool canPreviewDownload(const Json&);
std::string previewMimeType(const std::string&);
// Informational metadata only: never a resume validator or an instruction to rename.
Json probeDownload(const Json&,const Json&,const Cancel&,std::function<void(const Json&)> saveBrowserSession={});
class DownloadPreview {
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();
 mutable std::mutex mutex;Json result;std::thread worker;
public:
 DownloadPreview(Json download,Json preferences,int timeoutMs=10000,std::function<void(const Json&)> saveBrowserSession={});
 ~DownloadPreview();
 DownloadPreview(const DownloadPreview&)=delete;DownloadPreview& operator=(const DownloadPreview&)=delete;
 Json snapshot()const;
 void stop()noexcept;
};
// Bounded metadata-only work. Workers own their state, never a dialog or Manager.
class BatchDownloadPreview {
 struct State;std::shared_ptr<State> state;
public:
 BatchDownloadPreview(std::vector<std::string> addresses,Headers headers,Json preferences,std::vector<std::string> protectedRowHeaders={});
 ~BatchDownloadPreview();
 BatchDownloadPreview(const BatchDownloadPreview&)=delete;
 BatchDownloadPreview& operator=(const BatchDownloadPreview&)=delete;
 std::vector<std::pair<size_t,Json>> take();
};
}
