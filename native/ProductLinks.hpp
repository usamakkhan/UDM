#pragma once
#include "Core.hpp"
#include <shellapi.h>
namespace udm {
inline constexpr const char* productHome="https://github.com/usamakkhan/UDM";
inline constexpr const char* productSupport="https://github.com/usamakkhan/UDM/issues";
inline constexpr const char* productShareMail="mailto:?subject=UDM%20Download%20Manager&body=Take%20a%20look%20at%20UDM%20Download%20Manager%3A%0D%0Ahttps%3A%2F%2Fgithub.com%2Fusamakkhan%2FUDM";
inline void openProductLink(HWND owner,const std::string& address){
 if(address!=productHome&&address!=productSupport&&address!=productShareMail)throw std::runtime_error("Unknown UDM product link.");
 auto result=ShellExecuteW(owner,L"open",wide(address).c_str(),nullptr,nullptr,SW_SHOWNORMAL);if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open this link. Check your default browser or email app.");
}
inline void copyProductLink(HWND owner,const std::wstring& value){
 auto memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(!memory)throw std::runtime_error("Cannot allocate clipboard text.");
 auto target=GlobalLock(memory);if(!target){GlobalFree(memory);throw std::runtime_error("Cannot prepare clipboard text.");}memcpy(target,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);
 if(!OpenClipboard(owner)){GlobalFree(memory);throw std::runtime_error("The clipboard is busy. Try again.");}
 if(!EmptyClipboard()||!SetClipboardData(CF_UNICODETEXT,memory)){CloseClipboard();GlobalFree(memory);throw std::runtime_error("Windows could not copy the link.");}CloseClipboard();
}
}
