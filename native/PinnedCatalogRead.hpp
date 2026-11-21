#pragma once
#include "Core.hpp"

namespace udm {
// The caller owns a synchronous read handle and keeps its write/delete sharing
// exclusion through comparison. Read that exact object; never reopen its name.
inline std::string readPinnedCatalogText(HANDLE file,size_t limit){
 BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER length{},begin{};
 if(GetFileType(file)!=FILE_TYPE_DISK||!GetFileInformationByHandle(file,&info)||
    (info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))||
    !GetFileSizeEx(file,&length)||length.QuadPart<0)
  throw std::runtime_error("Cannot inspect the pinned catalog.");
 if(static_cast<uint64_t>(length.QuadPart)>limit)
  throw std::runtime_error("Catalog exceeds the supported size.");
 if(!SetFilePointerEx(file,begin,nullptr,FILE_BEGIN))
  throw std::runtime_error("Cannot seek the pinned catalog.");
 std::string text(static_cast<size_t>(length.QuadPart),'\0');
 size_t offset=0;
 while(offset<text.size()){
  const auto count=static_cast<DWORD>(std::min<size_t>(text.size()-offset,1048576));DWORD received=0;
  if(!ReadFile(file,text.data()+offset,count,&received,nullptr)||!received||received>count)
   throw std::runtime_error("Cannot read the complete pinned catalog.");
  offset+=received;
 }
 if(text.rfind("\xEF\xBB\xBF",0)==0)text.erase(0,3);
 return text;
}
}
