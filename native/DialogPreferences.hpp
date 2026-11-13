#pragma once
// Included inside udm, after Form.
inline const std::array<const char*,6>& progressPreferenceKeys(){static const std::array<const char*,6> keys={"ProgressStartMode","ProgressSpeedTab","ProgressCompletionTab","ProgressHideTabButtons","ProgressInformationTab","QueueProgressMinimized"};return keys;}
inline Json progressPreferenceValues(const Json& source){
 auto mode=str(source,"ProgressStartMode","Show normally sized");if(mode=="Normal size")mode="Show normally sized";if(mode=="Minimized")mode="Show minimized";
 return {{"ProgressStartMode",mode},{"ProgressSpeedTab",yes(source,"ProgressSpeedTab",true)},{"ProgressCompletionTab",yes(source,"ProgressCompletionTab",true)},{"ProgressHideTabButtons",yes(source,"ProgressHideTabButtons",true)},{"ProgressInformationTab",yes(source,"ProgressInformationTab")},{"QueueProgressMinimized",yes(source,"QueueProgressMinimized",true)}};
}
inline FileInfoDecisionResult applyProgressPreferences(Manager& manager,const Json& patch){
 if(!patch.is_object())throw std::runtime_error("Invalid progress preference changes.");
 for(auto i=patch.begin();i!=patch.end();++i){const auto& keys=progressPreferenceKeys();if(std::find(keys.begin(),keys.end(),i.key())==keys.end())throw std::runtime_error("Unknown progress preference.");
  if(i.key()=="ProgressStartMode"){const std::vector<std::string> modes={"Show normally sized","Small size","Show minimized","Don't show","Minimize to system tray"};if(!i->is_string()||std::find(modes.begin(),modes.end(),i->get<std::string>())==modes.end())throw std::runtime_error("Choose a valid progress start view.");}
  else if(!i->is_boolean())throw std::runtime_error("Choose whether the progress option is enabled.");
 }
 if(patch.empty())return {};
 Lock lock(manager.mutex);auto current=manager.state["Settings"];for(auto i=patch.begin();i!=patch.end();++i)current[i.key()]=i.value();manager.setSettings(current);return {};
}
inline void progressPreferences(CWnd* owner,Json& prefs,Manager* persist=nullptr){
 Form d("Customize download progress dialog",268,145,owner);d.dialogUnits=true;
 FileInfoDecisionTask task;bool handling=false;Json accepted=Json::object();const auto initial=progressPreferenceValues(prefs);std::vector<std::pair<HWND,BOOL>> enablement;
 const auto restore=[&]{for(const auto& item:enablement)if(IsWindow(item.first))::EnableWindow(item.first,item.second);enablement.clear();};
 const auto apply=[&](const Json& patch){for(auto i=patch.begin();i!=patch.end();++i)prefs[i.key()]=i.value();};
 d.cancel=[&]{if(!task.valid()&&!handling)d.close(IDCANCEL);};
 d.init=[&]{
  auto mode=str(prefs,"ProgressStartMode","Show normally sized");
  if(mode=="Normal size")mode="Show normally sized";
  if(mode=="Minimized")mode="Show minimized";
  d.label("Start view:",7,6,178,10);auto state=d.combo({"Show normally sized","Small size","Show minimized","Don't show","Minimize to system tray"},mode,7,18,150);
  d.label("This start view applies to individual downloads. Queue progress starts in the system tray by default. See More for queue settings.",7,34,254,27);
  d.control(L"STATIC","",SS_ETCHEDHORZ,7,66,254,1);
  auto speed=d.check("Show \"Speed Limiter\" tab",yes(prefs,"ProgressSpeedTab",true),7,74,254);
  auto completion=d.check("Show \"Options on completion\" tab",yes(prefs,"ProgressCompletionTab",true),7,88,189);
  auto hide=d.check("Show \"Hide tab\" buttons",yes(prefs,"ProgressHideTabButtons",true),7,101,254);
  auto additional=std::make_shared<Json>(initial);
  d.button("More...",206,124,55,[&,additional]{if(task.valid()||handling)return;Form more("Additional progress settings",415,150,&d);more.init=[&,additional]{auto info=more.check("Show Information tab",yes(*additional,"ProgressInformationTab",false),14,16,387);auto queue=more.check("Start queue progress windows in the system tray",yes(*additional,"QueueProgressMinimized",true),14,48,387);more.accept=[&,info,queue,additional]{(*additional)["ProgressInformationTab"]=more.checked(info);(*additional)["QueueProgressMinimized"]=more.checked(queue);more.close();};more.button("OK",218,109,85,more.accept);more.button("Cancel",314,109,87,[&]{more.close(IDCANCEL);});};more.DoModal();});
  d.accept=[&,state,speed,completion,hide,additional]{
   if(task.valid()||handling)return;
   auto draft=*additional;draft["ProgressStartMode"]=text(state);draft["ProgressSpeedTab"]=d.checked(speed);draft["ProgressCompletionTab"]=d.checked(completion);draft["ProgressHideTabButtons"]=d.checked(hide);
   Json patch=Json::object();for(const auto* key:progressPreferenceKeys())if(draft.at(key)!=initial.at(key))patch[key]=draft.at(key);
   if(!persist||patch.empty()){apply(patch);d.close();return;}
   enablement.clear();EnumChildWindows(d.GetSafeHwnd(),[](HWND child,LPARAM data)->BOOL{wchar_t kind[32]{};GetClassNameW(child,kind,32);if((GetWindowLongPtrW(child,GWL_STYLE)&WS_TABSTOP)||!_wcsicmp(kind,L"BUTTON")||!_wcsicmp(kind,L"EDIT")||!_wcsicmp(kind,L"COMBOBOX"))reinterpret_cast<std::vector<std::pair<HWND,BOOL>>*>(data)->push_back({child,IsWindowEnabled(child)});return TRUE;},reinterpret_cast<LPARAM>(&enablement));
   task.startAction(*persist,[source=persist,patch]{return applyProgressPreferences(*source,patch);},d.taskNotifier());accepted=std::move(patch);for(const auto& item:enablement)::EnableWindow(item.first,FALSE);
  };
  d.defaultButton(d.button("OK",77,124,50,d.accept));d.button("Cancel",139,124,50,d.cancel);
  d.pulse=[&]{if(handling||!task.valid()||!task.ready())return;handling=true;bool saved=false;
   try{task.get();saved=true;}catch(const std::exception& failure){error(&d,failure);}catch(...){error(&d,std::runtime_error("The progress preferences could not be saved. Your entries are still available."));}
   task.release();restore();handling=false;if(saved){apply(accepted);d.close();}accepted=Json::object();
  };
 };d.DoModal();
}
inline void playDownloadSound(const Json& prefs,const char* enabled,const char* key){if(!yes(prefs,enabled,std::string(enabled)=="Sound"))return;auto path=str(prefs,key);if(path.empty())MessageBeep(MB_OK);else PlaySoundW(wide(path).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
