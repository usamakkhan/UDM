#pragma once
#include "Core.hpp"

namespace udm {
// Copy only inputs consumed by list filtering, sorting and rendering. Transfer
// journals, captured plans, POST bodies and private child tracks stay on the job.
// The caller holds Manager::mutex while reading the source record and speed.
struct DownloadListRow {Json data;double speed=0;};
inline constexpr const char* DownloadListFields[]={"Id","FileName","Folder","Category","Status","Size","Received",
 "ConfirmationPending","MediaPhase","PrefetchedFinalSize","LiveRecording",
 "StreamDurationMs","StreamCompletedMs","AdaptiveTotalSegments","AdaptiveCompletedSegments",
 "AdaptiveSourceBytes","AdaptiveRefreshPendingValidation",
 "MediaLengthMs","StreamElapsedSeconds","StreamResumedMs","LastAttempt","LastTransferRate","Added",
 "Description","DownloadPage","Url","QueueMember","Queue","ProjectId","LastHttpStatus"};
inline bool downloadListRecordMatches(const Json& source,const Json& record,bool includeHeaders){
 auto same=[&](const char* field){const auto left=source.find(field),right=record.find(field);return left==source.end()?right==record.end():right!=record.end()&&*left==*right;};
 for(const auto* field:DownloadListFields)if(!same(field))return false;
 return includeHeaders?same("ProtectedHeaders"):!record.contains("ProtectedHeaders");
}
inline std::shared_ptr<const DownloadListRow> downloadListRow(const Json& source,double speed,bool includeHeaders,int queueImage,const std::shared_ptr<const DownloadListRow>& previous){
 if(previous&&previous->speed==speed&&num(previous->data,"ListQueueImage",-1)==queueImage&&downloadListRecordMatches(source,previous->data,includeHeaders))return previous;
 Json record=Json::object();for(const auto* field:DownloadListFields){auto value=source.find(field);if(value!=source.end())record[field]=*value;}
 if(includeHeaders){auto value=source.find("ProtectedHeaders");if(value!=source.end())record["ProtectedHeaders"]=*value;}
 record["ListQueueImage"]=queueImage;return std::make_shared<const DownloadListRow>(DownloadListRow{std::move(record),speed});
}
inline const std::string& downloadListFilename(const Json& record){
 static const std::string empty;const auto value=record.find("FileName");return value!=record.end()&&value->is_string()?value->get_ref<const std::string&>():empty;
}
inline fs::path downloadListTarget(const Json& record){return fs::path(wide(str(record,"Folder")))/wide(str(record,"FileName"));}
}
