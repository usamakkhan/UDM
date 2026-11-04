#pragma once
#include "Core.hpp"
#include <algorithm>
#include <cwchar>
#include <mbstring.h>
#include <string_view>

namespace udm {
// The reference list getter supplies 256 UTF-16 units including the terminator.
// Bound comparison keys only; keep full metadata and display text intact.
inline std::wstring referenceWideSortKey(const std::string& value){
 auto key=wide(value);
 const auto end=key.find(L'\0');
 key.resize(std::min<size_t>(255,end==std::wstring::npos?key.size():end));
 return key;
}
inline int emptyTextOrder(bool leftEmpty,bool rightEmpty){
 return leftEmpty==rightEmpty?0:leftEmpty?1:-1;
}
inline int compareOptionalWideText(const std::wstring& left,const std::wstring& right){
 const int empty=emptyTextOrder(left.empty(),right.empty());
 return empty?empty: ::_wcsnicmp(left.c_str(),right.c_str(),std::max(left.size(),right.size()));
}
// CStringA list retrieval in the reference uses the Windows ANSI code page.
// This temporary key never replaces the stored/displayed Unicode value.
inline std::string referenceAnsiSortKey(std::string_view value){
 auto text=wide(std::string(value));
 const auto end=text.find(L'\0');
 if(end!=std::wstring::npos)text.resize(end);
 if(text.empty())return {};
 const int count=WideCharToMultiByte(CP_ACP,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
 if(!count)throw std::runtime_error("Cannot prepare list text comparison.");
 std::string key(count,'\0');
 if(WideCharToMultiByte(CP_ACP,0,text.data(),static_cast<int>(text.size()),key.data(),count,nullptr,nullptr)!=count)throw std::runtime_error("Cannot prepare list text comparison.");
 return key;
}
struct ReferenceTextCompareError {};
inline int compareOptionalAnsiText(const std::string& left,const std::string& right,bool byteFallback){
 const int empty=emptyTextOrder(left.empty(),right.empty());
 if(empty)return empty;
 if(byteFallback)return ::_stricmp(left.c_str(),right.c_str());
 const int result=::_mbsicmp(reinterpret_cast<const unsigned char*>(left.c_str()),reinterpret_cast<const unsigned char*>(right.c_str()));
 if(result==_NLSCMPERROR)throw ReferenceTextCompareError{};
 return result;
}
}
