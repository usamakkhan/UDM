#pragma once
#include "Core.hpp"
#include <optional>

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
// UI-thread session state. The next row is independent of ordinary selection.
class DownloadFindCursor {
 Json query,scope;size_t next=0;int column=-1;bool ascending=true,viewValid=false,queryValid=false;
public:
 bool orderChanged(int nextColumn,bool direction)const{return viewValid&&(column!=nextColumn||ascending!=direction);}
 void updateView(Json nextScope,int nextColumn,bool direction,bool rebuilt,std::optional<size_t> restoredSelection){
  const bool changedScope=!viewValid||scope!=nextScope;
  if(changedScope)next=0;
  else if((rebuilt||orderChanged(nextColumn,direction))&&restoredSelection)next=*restoredSelection+1;
  scope=std::move(nextScope);column=nextColumn;ascending=direction;viewValid=true;
 }
 void prepare(const Json& value){auto normalized=normalizedFindQuery(value);if(!queryValid||query!=normalized)next=0;query=std::move(normalized);queryValid=true;}
 size_t nextRow()const{return next;}
 void found(size_t row){next=row+1;}
 bool wrap(){if(!next)return false;next=0;return true;}
};
}
