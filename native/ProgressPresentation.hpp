#pragma once
#include <string>
namespace udm {
enum class ProgressPresentation { Normal, Minimized, Hidden, Tray };
inline ProgressPresentation progressPresentation(const std::string& mode,bool queue,bool queueMinimized,bool explicitOpen){
 if(explicitOpen)return ProgressPresentation::Normal;
 if(queue)return queueMinimized?ProgressPresentation::Tray:ProgressPresentation::Normal;
 if(mode=="Minimize to system tray")return ProgressPresentation::Tray;
 if(mode=="Don't show")return ProgressPresentation::Hidden;
 return mode=="Minimized"?ProgressPresentation::Minimized:ProgressPresentation::Normal;
}
}