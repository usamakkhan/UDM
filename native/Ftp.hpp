#pragma once
#include "Core.hpp"
#include "ZipPreview.hpp"
namespace udm {
Json previewFtp(const std::string&,const Headers&,const Json&,const Cancel&);
ZipSource ftpZipSource(const std::string&,const Headers&,const Json&,const Cancel&,uint64_t& received);
// Produces the same durable part files used by the HTTP publication pipeline.
void transferFtp(Manager&,JobPtr,const std::shared_ptr<Cancel>&,const Json&,
 const Headers&,const std::string&,const fs::path&,JobPtr,const std::shared_ptr<Rate>&);
}
