#pragma once
#include "Core.hpp"
namespace udm {
inline std::string contentDispositionName(const std::string& header,const std::string& fallback){
 if(header.size()>8192||header.find_first_of("\r\n")!=std::string::npos)return fallback;
 std::map<std::string,std::string> parameters;size_t pos=header.find(';');
 while(pos!=std::string::npos&&pos<header.size()){
  ++pos;while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;
  auto equals=header.find('=',pos),separator=header.find(';',pos);if(equals==std::string::npos||(separator!=std::string::npos&&separator<equals)){pos=separator;continue;}
  auto key=lower(trim(header.substr(pos,equals-pos)));pos=equals+1;while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;
  std::string value;
  if(pos<header.size()&&header[pos]=='"'){
   ++pos;bool closed=false;while(pos<header.size()){char c=header[pos++];if(c=='"'){closed=true;break;}if(c=='\\'&&pos<header.size())c=header[pos++];value+=c;}
   if(!closed)return fallback;while(pos<header.size()&&isspace(static_cast<unsigned char>(header[pos])))++pos;if(pos<header.size()&&header[pos]!=';')return fallback;
  }else{auto end=header.find(';',pos);value=trim(header.substr(pos,end==std::string::npos?end:end-pos));pos=end;}
  if((key=="filename"||key=="filename*")&&!parameters.emplace(key,value).second)return fallback;
 }
 auto valid=[](const std::string& value){return !value.empty()&&value.find('\0')==std::string::npos&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0)>0;};
 auto extended=parameters.find("filename*");if(extended!=parameters.end()){
  auto value=extended->second;auto first=value.find('\''),second=first==std::string::npos?first:value.find('\'',first+1);
  if(first!=std::string::npos&&second!=std::string::npos&&lower(value.substr(0,first))=="utf-8"){
   auto encoded=value.substr(second+1);bool escapes=true;for(size_t i=0;i<encoded.size();++i)if(encoded[i]=='%'){if(i+2>=encoded.size()||!isxdigit(static_cast<unsigned char>(encoded[i+1]))||!isxdigit(static_cast<unsigned char>(encoded[i+2]))){escapes=false;break;}i+=2;}
   if(escapes){auto decoded=unescape(encoded);if(valid(decoded))return safeName(decoded);}
  }
 }
 auto plain=parameters.find("filename");if(plain!=parameters.end()&&valid(plain->second))return safeName(plain->second);
 return fallback;
}
}
