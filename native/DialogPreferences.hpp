#pragma once
// Included inside udm, after Form.
inline void progressPreferences(CWnd* owner,Json& prefs){
 Form d("Customize download progress dialog",268,145,owner);d.dialogUnits=true;d.init=[&]{
  d.label("Start view:",7,6,178,10);auto state=d.combo({"Normal size","Small size","Minimized"},str(prefs,"ProgressStartMode","Normal size"),7,18,90);
  d.label("This start view applies to individual downloads. Queue progress starts minimized by default; additional settings are under More.",7,34,254,27);
  d.control(L"STATIC","",SS_ETCHEDHORZ,7,66,254,1);
  auto speed=d.check("Show \"Speed Limiter\" tab",yes(prefs,"ProgressSpeedTab",true),7,74,254);
  auto completion=d.check("Show \"Options on completion\" tab",yes(prefs,"ProgressCompletionTab",true),7,88,189);
  auto hide=d.check("Show \"Hide tab\" buttons",yes(prefs,"ProgressHideTabButtons",true),7,101,254);
  auto additional=std::make_shared<Json>(prefs);
  d.button("More...",206,124,55,[&,additional]{Form more("Additional progress settings",415,150,&d);more.init=[&,additional]{auto info=more.check("Show Information tab",yes(*additional,"ProgressInformationTab",false),14,16,387);auto queue=more.check("Start queue progress windows minimized",yes(*additional,"QueueProgressMinimized",true),14,48,387);more.accept=[&,info,queue,additional]{(*additional)["ProgressInformationTab"]=more.checked(info);(*additional)["QueueProgressMinimized"]=more.checked(queue);more.close();};more.button("OK",218,109,85,more.accept);more.button("Cancel",314,109,87,[&]{more.close(IDCANCEL);});};more.DoModal();});
  d.accept=[&,state,speed,completion,hide,additional]{prefs["ProgressStartMode"]=text(state);prefs["ProgressSpeedTab"]=d.checked(speed);prefs["ProgressCompletionTab"]=d.checked(completion);prefs["ProgressHideTabButtons"]=d.checked(hide);for(auto key:{"ProgressInformationTab","QueueProgressMinimized"})if(additional->contains(key))prefs[key]=(*additional)[key];d.close();};
  d.defaultButton(d.button("OK",77,124,50,d.accept));d.button("Cancel",139,124,50,[&]{d.close(IDCANCEL);});
 };d.DoModal();
}
inline void playDownloadSound(const Json& prefs,const char* enabled,const char* key){if(!yes(prefs,enabled,std::string(enabled)=="Sound"))return;auto path=str(prefs,key);if(path.empty())MessageBeep(MB_OK);else PlaySoundW(wide(path).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}
