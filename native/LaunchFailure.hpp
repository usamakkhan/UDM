#pragma once
#include "Launch.hpp"
namespace udm {
// Parse only the error-presentation hint before full parsing can throw. Values
// that happen to spell /n or --silent are data, never quiet-mode switches.
inline bool launchSilentFailureRequested(const std::vector<std::wstring>& args){
 bool silent=false;
 for(size_t i=0;i<args.size();++i){const auto& arg=args[i];
  if(arg==L"/n"||arg==L"--silent")silent=true;
  else if(arg==L"--data-dir"||arg==L"--instance-tag"||arg==L"--wait-process"||arg==L"--add"||arg==L"/d"||arg==L"--shortcut"||arg==L"/u"||arg==L"--folder"||arg==L"/p"||arg==L"--name"||arg==L"/f"){if(i+1<args.size())++i;}
 }
 return silent;
}
// GUI-subsystem executables can still inherit redirected stderr. Report there
// when available, without creating a console, log file or replacement dialog.
inline bool writeLaunchFailure(const std::string& error)noexcept{
 try{const auto handle=GetStdHandle(STD_ERROR_HANDLE);if(!handle||handle==INVALID_HANDLE_VALUE)return false;auto message=std::string("UDM: ")+error+"\r\n";size_t offset=0;while(offset<message.size()){DWORD count=0;const auto chunk=static_cast<DWORD>(std::min<size_t>(message.size()-offset,MAXDWORD));if(!WriteFile(handle,message.data()+offset,chunk,&count,nullptr)||!count)return false;offset+=count;}return true;}catch(...){return false;}
}
}
