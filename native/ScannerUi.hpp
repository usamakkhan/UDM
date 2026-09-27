// Included inside udm, after Form.
inline bool scannerSettings(CWnd* owner,Json& prefs){
 Form d("Virus checking settings",550,342,owner);bool accepted=false;
 d.init=[&]{
  d.label("Scanner executable (leave blank to turn off automatic checking)",14,15,522);
  auto program=d.edit(str(prefs,"ScanProgram"),14,42,424);
  d.button("Browse...",449,41,87,[&,program]{CFileDialog chooser(TRUE,L"exe",wide(text(program)).c_str(),OFN_FILEMUSTEXIST,L"Programs|*.exe||",&d);if(chooser.DoModal()==IDOK)program->SetWindowText(chooser.GetPathName());});
  d.label("Command-line parameters",14,78,522);auto args=d.edit(str(prefs,"ScanArguments","\"{file}\""),14,104,522);
  d.label("Use {file} or [File] for the downloaded file. UDM quotes the path automatically. If no placeholder is present, the file path is appended.",14,139,522,42);
  d.label("Maximum wait (seconds, 1-3600)",14,195,356);auto timeout=d.edit(std::to_string(num(prefs,"ScanTimeoutSeconds",300)),397,190,139);
  d.label("UDM records the exit code; its meaning depends on your scanner. A timeout stops monitoring, not your antivirus. Files remain saved and are not quarantined by UDM.",14,232,522,53);
  d.accept=[&,program,args,timeout]{auto next=prefs;next["ScanProgram"]=trim(text(program));next["ScanArguments"]=text(args);next["ScanTimeoutSeconds"]=std::stoll(text(timeout));validateScannerSettings(next,!str(next,"ScanProgram").empty());prefs=std::move(next);accepted=true;d.close();};
  d.defaultButton(d.button("OK",348,303,88,d.accept));d.button("Cancel",448,303,88,[&]{d.close(IDCANCEL);});
 };d.DoModal();return accepted;
}
inline void fileScanner(CWnd* owner,Manager& manager,JobPtr job){
 Form d("Virus check",540,317,owner);
 d.init=[&]{
  d.label("File",14,16,60);d.edit(utf8(job->target().wstring()),74,12,452,23,true);
  auto result=d.edit("",14,51,512,123,true,true);
  d.label("Exit codes are reported by your scanner. Code 0 alone does not prove a file is safe. UDM does not quarantine files. Review your antivirus for detection details.",14,187,512,47);
  d.button("Settings...",14,240,112,[&]{Json prefs;{Lock lock(manager.mutex);prefs=manager.state["Settings"];}if(scannerSettings(&d,prefs))manager.setSettings(prefs);});
  auto scan=d.button("Scan file",137,240,109,[&]{manager.scanAgain(job);});
  auto stop=d.button("Stop waiting",257,240,125,[&]{manager.pause(job);});
  d.defaultButton(d.button("Close",421,278,105,[&]{d.close();}));
  d.pulse=[&,result,scan,stop]{Json record;bool active;{Lock lock(manager.mutex);record=job->data;active=manager.isActive(job);}
   auto value=scannerSummary(record);auto info=record.value("ScanResult",Json::object());
   if(!str(info,"Program").empty())value+="\r\nProgram: "+str(info,"Program");
   if(info.contains("StartedUtc"))value+="\r\nStarted: "+str(info,"StartedUtc");if(info.contains("FinishedUtc"))value+="\r\nFinished: "+str(info,"FinishedUtc");
   result->SetWindowText(cs(value));scan->EnableWindow(!active&&str(record,"Status")=="Complete"&&fs::is_regular_file(job->target()));stop->EnableWindow(active&&str(info,"Status")=="Running");
  };d.pulse();
 };d.DoModal();
}
