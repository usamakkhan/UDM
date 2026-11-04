#pragma once
#include "Core.hpp"

namespace udm {
// Persist only user-entered Find text and options, not matched/decrypted data.
inline Json normalizedFindQuery(const Json& source=Json::object()){
 auto text=str(source,"Text");
 try{wide(text);if(text.find_first_of("\r\n")!=std::string::npos||text.find('\0')!=std::string::npos)text.clear();}catch(const std::exception&){text.clear();}
 bool filename=yes(source,"FileName",true);const bool description=yes(source,"Description"),address=yes(source,"Address");
 if(!filename&&!description&&!address)filename=true;
 return {{"Text",std::move(text)},{"FileName",filename},{"Description",description},{"Address",address},{"MatchCase",yes(source,"MatchCase")},{"WholeString",yes(source,"WholeString")}};
}
inline void validateFindQuery(const Json& query){if(query!=normalizedFindQuery(query))throw std::runtime_error("Invalid saved Find text or options.");}
}
