#pragma once
#include "Core.hpp"
namespace udm {
bool canPreviewDownload(const Json&);
std::string previewMimeType(const std::string&);
// Informational metadata only: never a resume validator or an instruction to rename.
Json probeDownload(const Json&,const Json&,const Cancel&);
class DownloadPreview {
 std::shared_ptr<Cancel> cancel=std::make_shared<Cancel>();
 mutable std::mutex mutex;Json result;std::thread worker;
public:
 DownloadPreview(Json download,Json preferences,int timeoutMs=10000);
 ~DownloadPreview();
 DownloadPreview(const DownloadPreview&)=delete;DownloadPreview& operator=(const DownloadPreview&)=delete;
 Json snapshot()const;
 void stop()noexcept;
};
}
