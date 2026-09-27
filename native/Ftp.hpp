#pragma once
#include "Core.hpp"
namespace udm {
Json previewFtp(const std::string&,const Headers&,const Json&,const Cancel&);
// Produces the same durable part files used by the HTTP publication pipeline.
void transferFtp(Manager&,JobPtr,const std::shared_ptr<Cancel>&,const Json&,
 const Headers&,const std::string&,const fs::path&,JobPtr,const std::shared_ptr<Rate>&);
}
