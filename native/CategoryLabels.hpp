#pragma once
#include <windows.h>
#include <limits>
#include <optional>
#include <string_view>

namespace udm {
// CB_FINDSTRINGEXACT is case-insensitive, requires equal lengths, and does
// not match an empty search. Retain the selector's locale, not an ASCII fold.
inline std::optional<bool> categoryLabelEqual(LCID locale,std::wstring_view existing,std::wstring_view candidate){
 if(candidate.empty()||existing.size()!=candidate.size())return false;
 if(existing.size()>static_cast<size_t>((std::numeric_limits<int>::max)()))return {};
 const auto result=CompareStringW(locale,NORM_IGNORECASE,existing.data(),static_cast<int>(existing.size()),candidate.data(),static_cast<int>(candidate.size()));
 if(!result)return {};
 return result==CSTR_EQUAL;
}
}
