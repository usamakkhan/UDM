#pragma once
#include "Core.hpp"
namespace udm {
std::string grabberLocalReference(const fs::path& document,const fs::path& destination);
std::string rewriteGrabberDocument(const std::string&,const std::string&,bool,const std::map<std::string,std::string>&);
std::string grabberLinksSignature(const Manager&,const Json&);
bool grabberLinkDocument(const Json&);
// Converts a verified completed file. Original bytes and a recovery journal are
// retained independently from the transfer's temporary segment directory.
bool convertGrabberFile(Manager&,JobPtr,const Json& project,const std::map<std::string,fs::path>& files,const Cancel&);
}
