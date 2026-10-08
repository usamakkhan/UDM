#pragma once
#include "Core.hpp"
namespace udm {
inline std::string audioLanguageTag(std::string language){
 if(language.empty())return {};auto primary=lower(language.substr(0,language.find('-')));if(primary.size()==3)return primary;
 wchar_t iso[16]{};if(GetLocaleInfoEx(wide(language).c_str(),LOCALE_SISO639LANGNAME2,iso,16)||GetLocaleInfoEx(wide(primary).c_str(),LOCALE_SISO639LANGNAME2,iso,16)){auto result=lower(utf8(iso));if(result.size()==3)return result;}return {};
}
inline void appendAudioMetadata(std::vector<std::wstring>& args,const Json& plan){
 auto name=str(plan,"audioName");if(!name.empty())args.insert(args.end(),{L"-metadata:s:a:0",L"title="+wide(name),L"-metadata:s:a:0",L"handler_name="+wide(name)});
 auto language=audioLanguageTag(str(plan,"audioLanguage"));if(!language.empty())args.insert(args.end(),{L"-metadata:s:a:0",L"language="+wide(language)});
}
}
