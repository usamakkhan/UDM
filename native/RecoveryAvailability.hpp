#pragma once
#include "Core.hpp"
namespace udm {
inline std::wstring recoveryBusyName(){return L"Local\\UDM.RecoveryBusy."+wide(pipeName());}
class RecoveryAvailability {
 Handle event;
public:
 RecoveryAvailability(){event.h=CreateEventW(nullptr,TRUE,TRUE,recoveryBusyName().c_str());if(!event)throw std::runtime_error("Cannot publish recovery status.");}
 RecoveryAvailability(const RecoveryAvailability&)=delete;
 RecoveryAvailability& operator=(const RecoveryAvailability&)=delete;
};
inline bool recoveryInProgress(){
 Handle event(OpenEventW(SYNCHRONIZE,FALSE,recoveryBusyName().c_str()));
 if(!event){auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND)return false;throw std::runtime_error("Cannot check UDM recovery status.");}
 return WaitForSingleObject(event.h,0)==WAIT_OBJECT_0;
}
inline Json recoveryBusyReply(){return {{"ok",false},{"recovery",true},{"retryable",true},{"error","UDM backup and recovery is open. Finish recovery, choose Open UDM, then retry this download."}};}
}
