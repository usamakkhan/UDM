#pragma once
#include "Core.hpp"
#include <algorithm>
namespace udm {
// Windows dialog units use the selected font's average alphabet width and height.
// Keep horizontal and vertical conversion separate; DPI alone is insufficient.
struct DialogUnits {
 int x=0,y=0;
 static DialogUnits measure(HDC dc,HFONT font){
  auto old=SelectObject(dc,font);SIZE alphabet{};TEXTMETRICW metrics{};
  const bool ok=GetTextExtentPoint32W(dc,L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",52,&alphabet)&&GetTextMetricsW(dc,&metrics);
  SelectObject(dc,old);if(!ok)throw std::runtime_error("Cannot measure the dialog font.");
  return {(alphabet.cx/26+1)/2,metrics.tmHeight};
 }
 int px(int value)const{return MulDiv(value,x,4);}
 int py(int value)const{return MulDiv(value,y,8);}
 RECT rect(int left,int top,int width,int height)const{return {px(left),py(top),px(left+width),py(top+height)};}
};
inline void setProgressLimit(Manager& manager,JobPtr job,i64 kb,bool remember){
 if(kb<0||kb>1000000)throw std::runtime_error("Use a speed limit from 0 to 1,000,000 KB/s.");
 Lock lock(manager.mutex);if(std::find(manager.jobs.begin(),manager.jobs.end(),job)==manager.jobs.end())throw std::runtime_error("Unknown download.");
 if(!remember){job->sessionLimit=kb;return;}
 const auto before=job->data;const auto temporary=job->sessionLimit;
 job->data["LimitKbps"]=kb;job->sessionLimit.reset();
 try{manager.save();}catch(...){job->data=before;job->sessionLimit=temporary;throw;}
}
}
