#pragma once
#include "Core.hpp"
namespace udm {
Json scannerPreset(const std::string& program);
Json scannerExitResult(const Json&,DWORD code);
void validateScannerSettings(const Json&,bool requireExecutable=false);
std::wstring scannerCommand(const Json&,const fs::path&);
Json runScanner(const Json&,const fs::path&,const Cancel&);
std::string scannerSummary(const Json& record);
bool scannerAllowsCompletion(const Json& record);
void recoverScanner(Json& record);
}
