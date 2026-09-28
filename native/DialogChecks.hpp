#pragma once
#include "DialogGeometry.hpp"
static void dialogChecks(const udm::fs::path& root){
 using namespace udm;
 for(int dpi:{96,120,144,192}){
  HDC dc=CreateCompatibleDC(nullptr);HFONT font=CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");
  if(!dc||!font)throw std::runtime_error("Cannot create dialog font fixture.");auto units=DialogUnits::measure(dc,font);auto previous=SelectObject(dc,font);
  bool fit=true;for(auto button:std::vector<std::pair<std::wstring,int>>{{L"Start Download",74},{L"Download Later",74},{L"Open with...",61},{L"Hide tab",65},{L"Pause",56},{L"Cancel",50},{L"Retry",56}}){SIZE size{};GetTextExtentPoint32W(dc,button.first.c_str(),(int)button.first.size(),&size);fit&=size.cx+MulDiv(8,dpi,96)<=units.px(button.second);}
  check(fit,("Reference action captions fit Tahoma 8 at "+std::to_string(dpi)+" DPI with button padding").c_str());
  bool liveFit=true;for(const std::wstring caption:{L"Stop and save",L"Recording...",L"Saving..."}){SIZE extent{};GetTextExtentPoint32W(dc,caption.c_str(),(int)caption.size(),&extent);liveFit&=extent.cx+MulDiv(8,dpi,96)<=units.px(56);}check(liveFit,("Live progress captions fit the existing action button at "+std::to_string(dpi)+" DPI").c_str());
  TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);check(metrics.tmHeight<=units.py(9)&&metrics.tmHeight+MulDiv(4,dpi,96)<=units.py(14),("Reference row and input heights contain the actual Windows font at "+std::to_string(dpi)+" DPI").c_str());
  const auto a=units.rect(7,111,338,10),b=units.rect(15,126,93,15),c=units.rect(7,168,338,79),d=units.rect(0,0,353,250);
  check(a.bottom<b.top&&b.bottom<c.top&&c.right<=d.right&&c.bottom<d.bottom,("Converted progress bar, actions and connections remain separate at "+std::to_string(dpi)+" DPI").c_str());
  for(const auto& item:std::vector<std::pair<std::wstring,int>>{{L"Show warning before stopping downloads",209},{L"If existing file is complete, show Download complete; otherwise resume it.",272},{L"Add the duplicate and overwrite the existing file",272},{L"Remember my selection and do not show this dialog again.\r\nChange this later in UDM Options > Downloads.",260}}){RECT area{0,0,units.px(item.second)-MulDiv(18,dpi,96),0};DrawTextW(dc,item.first.c_str(),-1,&area,DT_CALCRECT|DT_WORDBREAK);check(area.bottom<=units.py(item.second==209||item.first.find(L"Add the duplicate")==0?10:18),("Quota/duplicate caption fits its reference rectangle at "+std::to_string(dpi)+" DPI").c_str());}
  std::wstring warning=L"The download limit of 95.37 TB every 168 hour(s) has been reached.\r\n\r\nDownload limit reached. Resuming automatically in 168 hr 59 min 59 sec.\r\nYou can change the limit in Options > Connection.";RECT area{0,0,units.px(325),0};DrawTextW(dc,warning.c_str(),-1,&area,DT_CALCRECT|DT_WORDBREAK);check(area.bottom<=units.py(60),("Quota warning text fits its reference dialog at "+std::to_string(dpi)+" DPI").c_str());
  SelectObject(dc,previous);DeleteObject(font);DeleteDC(dc);
 }
 Manager manager(root/L"dialog-model-state");auto job=manager.add("https://example.test/limited.bin");
 setProgressLimit(manager,job,256,true);check(num(job->data,"LimitKbps")==256&&!job->sessionLimit,"Remembered progress speed limit saves the record and removes a temporary override");
 auto hash=fileHash(manager.root/L"state.json");setProgressLimit(manager,job,128,false);check(job->sessionLimit==128&&num(job->data,"LimitKbps")==256&&fileHash(manager.root/L"state.json")==hash,"Temporary progress speed limit changes neither persisted rate nor saved history");
 manager.resume(job);check(!job->sessionLimit&&num(job->data,"LimitKbps")==256,"Stop/resume discards the temporary limiter and restores remembered settings");manager.pause(job);
 auto before=job->data;rejects([&]{setProgressLimit(manager,job,-1,true);},"Limiter rejects a negative rate");rejects([&]{setProgressLimit(manager,job,1000001,true);},"Limiter rejects a rate above the supported ceiling");check(before==job->data,"Rejected limiter edits leave all download metadata unchanged");
 setProgressLimit(manager,job,96,false);auto folder=manager.root;manager.root=root/L"dialog-save-blocker";writeBytes(manager.root,Bytes{1});rejects([&]{setProgressLimit(manager,job,64,true);},"Remembered limiter reports failed persistence");manager.root=folder;
 check(job->sessionLimit==96&&num(job->data,"LimitKbps")==256,"Failed remembered limiter save rolls back both stored and temporary limits");
 setProgressLimit(manager,job,0,true);{Manager reopened(folder);check(num(reopened.jobs.at(0)->data,"LimitKbps")==0&&!reopened.jobs.at(0)->sessionLimit,"Disabling a remembered limiter stays disabled after process restart");}
 auto orphan=std::make_shared<Job>(job->data);rejects([&]{setProgressLimit(manager,orphan,12,true);},"Limiter cannot mutate a detached record");
 auto prefs=manager.state["Settings"];prefs["ProgressCompletionTab"]=false;prefs["ProgressHideTabButtons"]=false;manager.setSettings(prefs);{Manager reopened(folder);check(!yes(reopened.state["Settings"],"ProgressCompletionTab",true)&&!yes(reopened.state["Settings"],"ProgressHideTabButtons",true),"Completion tab and Hide tab controls retain their preferences after restart");}
}
