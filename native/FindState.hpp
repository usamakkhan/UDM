#pragma once
#include "Core.hpp"
#include <optional>

namespace udm {
inline constexpr size_t FindInputUnits=2047;
// Find reads Windows text units, including a surrogate cut by its fixed buffer.
// JSON Text stays valid UTF-8; the optional bounded unit array preserves exact
// matching/display in that exceptional case instead of changing the needle.
inline Json findTextFields(const std::wstring& value){
 std::wstring printable;printable.reserve(value.size());bool unpaired=false;
 for(size_t at=0;at<value.size();++at){const auto unit=static_cast<unsigned>(value[at]);
  if(unit>=0xd800&&unit<=0xdbff){
   if(at+1<value.size()&&value[at+1]>=0xdc00&&value[at+1]<=0xdfff){printable+=value[at];printable+=value[++at];}
   else{printable+=L'\ufffd';unpaired=true;}
  }else if(unit>=0xdc00&&unit<=0xdfff){printable+=L'\ufffd';unpaired=true;}
  else printable+=value[at];
 }
 Json result={{"Text",utf8(printable)}};
 if(unpaired){
  if(value.size()>FindInputUnits)throw std::runtime_error("Find text exceeds its Windows input limit.");
  result["TextUtf16"]=Json::array();for(const auto unit:value)result["TextUtf16"].push_back(static_cast<unsigned>(unit));
 }
 return result;
}
inline std::wstring findQueryWideText(const Json& source){
 const auto units=source.find("TextUtf16");if(units==source.end())return wide(str(source,"Text"));
 if(!units->is_array()||units->empty()||units->size()>FindInputUnits)throw std::runtime_error("Invalid Find text units.");
 std::wstring value;value.reserve(units->size());
 for(const auto& item:*units){if(!item.is_number_integer())throw std::runtime_error("Invalid Find text unit.");const auto unit=item.get<uint64_t>();if(unit>0xffff)throw std::runtime_error("Invalid Find text unit.");value+=static_cast<wchar_t>(unit);}
 const auto canonical=findTextFields(value);
 if(!canonical.contains("TextUtf16")||str(canonical,"Text")!=str(source,"Text"))throw std::runtime_error("Inconsistent Find text units.");
 return value;
}
// Persist only user-entered Find text and options, not matched/decrypted data.
inline Json normalizedFindQuery(const Json& source=Json::object()){
 Json fields={{"Text",""}};
 try{const auto value=findQueryWideText(source);if(value.find_first_of(L"\r\n")!=std::wstring::npos||value.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Invalid Find text.");fields=findTextFields(value);}catch(const std::exception&){}
 bool filename=yes(source,"FileName",true);const bool description=yes(source,"Description"),address=yes(source,"Address");
 if(!filename&&!description&&!address)filename=true;
 Json result={{"Text",fields["Text"]},{"FileName",filename},{"Description",description},{"Address",address},{"MatchCase",yes(source,"MatchCase")},{"WholeString",yes(source,"WholeString")}};
 if(fields.contains("TextUtf16"))result["TextUtf16"]=std::move(fields["TextUtf16"]);return result;
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
