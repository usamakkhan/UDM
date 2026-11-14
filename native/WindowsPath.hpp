#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
namespace udm::windowsPath {
// Logical Windows path equality only: no filesystem identity or URL folding.
inline std::wstring key(const std::filesystem::path& path){auto value=path.lexically_normal().generic_wstring();if(value.find(L'\0')!=std::wstring::npos||value.size()>static_cast<size_t>(std::numeric_limits<int>::max()))throw std::runtime_error("Invalid download path.");return value;}
inline bool equal(const std::wstring& a,const std::wstring& b)noexcept{if(a.empty()||b.empty())return a.empty()&&b.empty();return CompareStringOrdinal(a.data(),static_cast<int>(a.size()),b.data(),static_cast<int>(b.size()),TRUE)==CSTR_EQUAL;}
struct Less {bool operator()(const std::wstring& a,const std::wstring& b)const noexcept{if(a.empty()||b.empty())return a.empty()&&!b.empty();return CompareStringOrdinal(a.data(),static_cast<int>(a.size()),b.data(),static_cast<int>(b.size()),TRUE)==CSTR_LESS_THAN;}};
using Keys=std::set<std::wstring,Less>;
inline bool descendant(const std::wstring& child,std::wstring parent){if(parent.empty())return false;if(parent.back()!=L'/')parent+=L'/';return child.size()>parent.size()&&CompareStringOrdinal(child.data(),static_cast<int>(parent.size()),parent.data(),static_cast<int>(parent.size()),TRUE)==CSTR_EQUAL;}
}
