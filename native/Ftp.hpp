#pragma once
#include "Core.hpp"
namespace udm {
// Produces the same durable part files used by the HTTP publication pipeline.
void transferFtp(Manager&,JobPtr,const std::shared_ptr<Cancel>&,const Json&,
 const Headers&,const std::string&,const fs::path&,JobPtr,const std::shared_ptr<Rate>&);
}
