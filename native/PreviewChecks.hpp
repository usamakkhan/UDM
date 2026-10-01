#pragma once
#include "DownloadPreview.hpp"
static void previewChecks(){
 Json data={{"Url","https://files.example.test/file.zip"},{"Status","Paused"},{"Received",0},{"Segments",Json::array()}};
 check(canPreviewDownload(data),"Paused direct files allow independent metadata preview");data["Url"]="ftp://files.example.test/file.zip";check(canPreviewDownload(data),"FTP metadata preview is eligible without opening a data transfer");data["Url"]="https://files.example.test/file.zip";
 for(const auto* status:{"Downloading","Verifying","Complete","Queued"}){auto active=data;active["Status"]=status;check(!canPreviewDownload(active),"Metadata preview cannot race an active or queued transfer");}
 for(const auto* field:{"ProtectedRequest","SourceUrl","ProtectedAdaptive","DuplicateOf"}){auto excluded=data;excluded[field]="fixture";check(!canPreviewDownload(excluded),"Metadata preview excludes forms, captured media and duplicate decisions");}
 for(const auto* field:{"RequiresRequestCapture","RequiresMediaCapture"}){auto excluded=data;excluded[field]=true;check(!canPreviewDownload(excluded),"Metadata preview cannot bypass required browser recapture");}
 auto partial=data;partial["Received"]=1;check(!canPreviewDownload(partial),"Metadata lookup cannot replace a partial download's generation size");partial=data;partial["Segments"]=Json::array({{{"Start",0},{"Done",0}}});check(!canPreviewDownload(partial),"A saved segment plan disables independent metadata probing");partial=data;partial["OfflineProject"]=Json::object();check(!canPreviewDownload(partial),"Offline website projects never trigger file metadata requests");
 check(previewMimeType(" Video/MP4; charset=UTF-8 ")=="video/mp4","Server MIME type is normalized independently from a filename extension");
 for(const auto* invalid:{"text/plain\r\nInjected: yes","text/plain/extra","<html>","text/"})check(previewMimeType(invalid).empty(),"Invalid MIME text is omitted from the native dialog");
 DownloadPreview omitted(partial,defaultSettings());check(str(omitted.snapshot(),"Status")=="Unavailable","Excluded previews complete synchronously without a worker or network access");
}
