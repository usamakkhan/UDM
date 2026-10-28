#pragma once
#include "Core.hpp"
#include "MimeFilenameData.hpp"
namespace udm {
inline std::string previewResponseFilename(const std::string& address,const std::string& declared,const std::string& mime,bool correctExtension){
 auto name=declared.empty()?safeName(unescape(Url(address).path)):declared;
 if(!correctExtension)return name;
 const auto dot=name.rfind('.');const auto extension=dot==std::string::npos?std::string{}:lower(name.substr(dot+1));
 const bool weak=extension.empty()||extension=="bin"||extension=="dat"||extension=="tmp"||extension=="download";
 const bool endpoint=declared.empty()&&(extension=="php"||extension=="asp"||extension=="aspx"||extension=="ashx"||extension=="jsp"||extension=="cgi"||extension=="do"||extension=="action");
 // A specific declared name retains priority. Generic MIME is not a file type.
 if(!weak&&!endpoint)return name;
 if(mime=="application/octet-stream"||mime=="binary/octet-stream"||mime=="application/binary"||mime=="application/x-binary"||mime=="application/download"||mime=="application/x-download"||mime=="application/force-download"||mime=="application/x-force-download"||mime=="application/unknown"||mime=="unknown/unknown")return name;
 const auto inferred=mimeFilenameExtension(mime);if(inferred.empty())return name;
 auto stem=dot==std::string::npos?name:name.substr(0,dot);if(stem.empty())stem="download";
 return safeName(stem+"."+std::string(inferred));
}
}
