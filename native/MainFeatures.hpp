 void openBrowserIntegration(){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  browserIntegration(this,manager,browserIntegrationObserver);
#else
  browserIntegration(this,manager);
#endif
 }
// Included inside MainWindow: persisted desktop interaction preferences.
#define UDM_APPEARANCE_REPAINT_AVAILABLE
 Json renderedAppearance=Json::object();const Json* appearanceReadSnapshot=nullptr;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void()> appearanceRenderObserver;
#endif
 static constexpr int columnCount=12;
 static constexpr int referenceColumnDpi=120;
 static constexpr int referenceColumnWidths[]={188,25,94,81,94,119,81,250};
 static constexpr int visibleDefaultColumns=static_cast<int>(std::size(referenceColumnWidths));
 static constexpr int defaultColumnWidths[columnCount]={150,20,75,65,75,95,65,200,132,220,200,200};
 static int scaledColumnWidth(int index,int width,int dpi){
  return index<visibleDefaultColumns&&width==defaultColumnWidths[index]?
         MulDiv(referenceColumnWidths[index],dpi,referenceColumnDpi):MulDiv(width,dpi,96);
 }
 static bool isReferenceColumnWidth(int index,int pixels,int dpi){
  return index<visibleDefaultColumns&&pixels==scaledColumnWidth(index,defaultColumnWidths[index],dpi);
 }
 static int logicalColumnWidth(int index,int pixels,int dpi){
  return isReferenceColumnWidth(index,pixels,dpi)?defaultColumnWidths[index]:MulDiv(pixels,96,dpi);
 }
 const wchar_t* columnName(int index)const{static const wchar_t* names[]={L"File Name",L"Q",L"Size",L"Status",L"Time left",L"Transfer rate",L"Last Try Date",L"Description",L"Date Added",L"Save To",L"Referer",L"Parent web page"};return names[index];}
 Json preferences(){if(optionsAppearanceSnapshot)return *optionsAppearanceSnapshot;Lock lock(manager.mutex);return manager.state["Settings"];}
 // Render helpers may read a borrowed UI-owned snapshot. Ordinary preference
 // commands deliberately keep their separate live settings path.
 Json appearancePreferences(){return appearanceReadSnapshot?*appearanceReadSnapshot:preferences();}
 Json repaintAppearancePreferences(){if(appearanceReadSnapshot)return *appearanceReadSnapshot;if(optionsAppearanceSnapshot)return *optionsAppearanceSnapshot;std::unique_lock lock(manager.mutex,std::try_to_lock);return lock.owns_lock()?manager.state["Settings"]:renderedAppearance;}
 void preference(const char* key,const Json& value){auto p=preferences();p[key]=value;manager.setSettings(p);}
 Json listLayout(){Json widths=Json::array(),order=Json::array();int indices[columnCount]{};table.GetColumnOrderArray(indices,columnCount);for(int i=0;i<columnCount;++i){widths.push_back(logicalColumnWidth(i,table.GetColumnWidth(i),windowDpi()));order.push_back(indices[i]);}return {{"Widths",widths},{"Order",order},{"SortColumn",sortColumn},{"Ascending",ascending}};}
 void applyListLayout(const Json& layout){if(layout.contains("Widths")&&layout["Widths"].is_array()&&layout["Widths"].size()==columnCount)for(int i=0;i<columnCount;++i){int width=std::clamp<int>((int)layout["Widths"][i],0,1200);table.SetColumnWidth(i,scaledColumnWidth(i,width,windowDpi()));}if(layout.contains("Order")&&layout["Order"].is_array()&&layout["Order"].size()==columnCount){int order[columnCount]{};std::set<int> seen;for(int i=0;i<columnCount;++i){order[i]=layout["Order"][i];if(order[i]<0||order[i]>=columnCount||!seen.insert(order[i]).second)return;}table.SetColumnOrderArray(columnCount,order);}sortColumn=(int)std::clamp<i64>(num(layout,"SortColumn",-1),-1,columnCount-1);ascending=yes(layout,"Ascending",true);table.sortIndicator(sortColumn,ascending);}
 void restoreLayout(){applyListLayout(preferences().value("ListLayout",Json::object()));}
 void rememberLayout(){
  if(!table.GetSafeHwnd())return;
  // Read thread-owned Windows controls before taking the shared catalog lock.
  auto next=listLayout();
  // The controls retain the newest layout while Manager is busy. Its next
  // timer captures it again; Exit owns a separate final-layout merge.
  std::unique_lock lock(manager.mutex,std::try_to_lock);if(!lock.owns_lock())return;auto& saved=manager.state["Settings"]["ListLayout"];
  if(saved!=next)saved=std::move(next);
 }
 void customizeColumns(){
  struct ColumnsForm:Form{using Form::Form;std::function<void(NMHDR*)> changed;BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{if(changed)changed(reinterpret_cast<NMHDR*>(l));return Form::OnNotify(w,l,result);}};
  static_assert(columnCount==ColumnSettingsCount);ColumnSettingsTask task;bool loaded=false;CWnd* retry=nullptr;std::function<void()> beginRead;
  ColumnsForm d("Columns",236,177,this);d.dialogUnits=true;d.pulsePeriod=40;std::vector<int> order(columnCount),widths(columnCount);std::vector<bool> shown(columnCount);table.GetColumnOrderArray(order.data(),columnCount);for(int i=0;i<columnCount;++i){widths[i]=table.GetColumnWidth(i)>0?logicalColumnWidth(i,table.GetColumnWidth(i),windowDpi()):defaultColumnWidths[i];shown[i]=table.GetColumnWidth(i)>0;}
  CListCtrl* list=nullptr;CWnd *width=nullptr,*up=nullptr,*down=nullptr,*showColumn=nullptr,*hideColumn=nullptr;
  auto syncVisibility=[&]{if(!list||!showColumn||!hideColumn)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);const bool selected=row>=0&&row<columnCount;showColumn->EnableWindow(selected&&!list->GetCheck(row));hideColumn->EnableWindow(selected&&list->GetCheck(row));};
  bool syncingWidth=false;
  auto syncSelection=[&]{if(!list||!width||!up||!down)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);const bool selected=row>=0&&row<columnCount;width->EnableWindow(selected);up->EnableWindow(selected&&row>0);down->EnableWindow(selected&&row+1<columnCount);syncingWidth=true;width->SetWindowText(cs(selected?std::to_string(widths[order[row]]):std::string()));syncingWidth=false;};
  d.changed=[&](NMHDR* h){if(!loaded||task.valid())return;if(list&&h&&h->hwndFrom==list->m_hWnd&&h->code==LVN_ITEMCHANGED){const auto item=reinterpret_cast<NMLISTVIEW*>(h);if((item->uChanged&LVIF_STATE)&&((item->uOldState^item->uNewState)&LVIS_SELECTED))syncSelection();syncVisibility();}};
  auto collect=[&]{for(int row=0;row<columnCount;++row)shown[order[row]]=list->GetCheck(row)!=FALSE;};
  auto fill=[&](int selection){list->DeleteAllItems();for(int row=0;row<columnCount;++row){int index=order[row];list->InsertItem(row,columnName(index));list->SetCheck(row,shown[index]);}if(selection>=0)list->SetItemState(selection,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);};
  auto storeWidth=[&](bool validate){
   if(syncingWidth||!loaded||task.valid()||!list||!width)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);if(row<0)return;
   const auto value=trim(text(width));const int minimum=order[row]==0?40:1;
   if(!std::regex_match(value,std::regex("[0-9]{1,4}"))){if(validate)throw std::runtime_error("Enter a whole-number column width.");return;}
   const int n=std::stoi(value);if(n<minimum||n>1200){if(validate)throw std::runtime_error("Use "+std::to_string(minimum)+" to 1200 pixels.");return;}
   widths[order[row]]=n;
  };
  d.init=[&]{
   d.SetIcon(nullptr,TRUE);d.SetIcon(nullptr,FALSE);d.resizeClient(236,177);
   auto instructions=d.label("Check the columns that you would like visible in this list. Use the Move Up and Move Down buttons to reorder the columns however you like.",7,7,222,25);
   list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL|LVS_NOCOLUMNHEADER,7,36,139,87);
   list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);list->InsertColumn(0,L"Column",LVCFMT_LEFT,d.pixelsX(119));
   auto move=[&](int step){int row=list->GetNextItem(-1,LVNI_SELECTED);if(row<0||row+step<0||row+step>=columnCount)return;storeWidth(true);collect();std::swap(order[row],order[row+step]);fill(row+step);};
   up=d.button("Move Up",158,36,71,[move]{move(-1);});down=d.button("Move Down",158,54,71,[move]{move(1);});
   showColumn=d.button("Show",158,72,71,[&]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0)list->SetCheck(row,TRUE);syncVisibility();});
   hideColumn=d.button("Hide",158,90,71,[&]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0)list->SetCheck(row,FALSE);syncVisibility();});
   auto reset=d.button("Reset",158,108,71,[&]{for(int i=0;i<columnCount;++i){order[i]=i;widths[i]=defaultColumnWidths[i];shown[i]=i<visibleDefaultColumns;}fill(0);});
   d.control(L"STATIC","The selected column should be",SS_RIGHT,7,132,123,10);
   width=d.edit("",136,130,20);width->SendMessage(EM_SETLIMITTEXT,4);d.bindChange(width,[&]{storeWidth(false);});d.label("pixels wide",159,132,66,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,7,149,222,1);
   auto ok=d.button("OK",107,155,50,[]{ });d.defaultButton(ok);
   d.cancel=[&]{if(task.saving())return;task.cancelRead();d.close(IDCANCEL);};auto cancel=d.button("Cancel",179,155,50,d.cancel);
   const auto working=[&,ok,cancel,reset](bool busy){for(auto control:std::vector<CWnd*>{list,width,up,down,showColumn,hideColumn,reset,ok})control->EnableWindow(loaded&&!busy);cancel->EnableWindow(!task.saving());if(loaded&&!busy){syncSelection();syncVisibility();}};
   const auto start=[&](std::optional<ColumnSettingsDraft> draft){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    task.start(manager,std::move(draft),d.taskNotifier(),columnSettingsObserver);
#else
    task.start(manager,std::move(draft),d.taskNotifier());
#endif
   };
   d.accept=[&,working,start,instructions]{if(!loaded||task.valid())return;storeWidth(true);collect();auto layout=listLayout();layout["Order"]=order;for(int i=0;i<columnCount;++i)layout["Widths"][i]=shown[i]?widths[i]:0;
    try{start(ColumnSettingsDraft{std::move(layout),widths});retry->ShowWindow(SW_HIDE);instructions->SetWindowText(L"Saving column settings...");working(true);}catch(const std::exception&){instructions->SetWindowText(L"Could not save columns. Your edits are kept; try OK again.");working(false);}
   };d.bind(ok,d.accept);
   beginRead=[&,start,working,instructions]{retry->ShowWindow(SW_HIDE);instructions->SetWindowText(L"Reading column settings...");try{start({});working(true);}catch(const std::exception&){instructions->SetWindowText(L"Column settings unavailable. Please retry.");working(false);retry->ShowWindow(SW_SHOW);}};
   retry=d.button("Retry",7,155,75,[&]{if(!loaded&&!task.valid())beginRead();});retry->ShowWindow(SW_HIDE);
   d.pulse=[&,working,instructions]{if(!task.ready())return;auto result=task.take();if(result.saved){applyListLayout(result.layout);d.close();return;}if(result.cancelled){d.cancel();return;}if(!result.error.empty()){working(false);instructions->SetWindowText(loaded?L"Could not save columns. Your edits are kept; try OK again.":L"Column settings unavailable. Please retry.");if(!loaded)retry->ShowWindow(SW_SHOW);return;}
    if(result.widths.is_array()&&result.widths.size()==columnCount)for(int i=0;i<columnCount;++i)if(!shown[i]){const auto& value=result.widths[i];if(value.is_number_integer()&&value>=(i==0?40:1)&&value<=1200)widths[i]=value.get<int>();}
    loaded=true;fill(-1);instructions->SetWindowText(L"Check the columns that you would like visible in this list. Use the Move Up and Move Down buttons to reorder the columns however you like.");working(false);
   };working(true);beginRead();
  };d.DoModal();auto remaining=task.finish();if(remaining.saved&&GetSafeHwnd())applyListLayout(remaining.layout);
 }

 void startGlobalLimitCommand(bool enabled){
  std::vector<BOOL> buttons;buttons.reserve(ToolbarCommandCount);for(int i=0;i<ToolbarCommandCount;++i)buttons.push_back(toolbar.IsButtonEnabled(toolbarCommand(i)));
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  globalLimitCommandTask.startToggle(manager,enabled,maintenanceSignal->notifier(),globalLimitCommandObserver);
#else
  globalLimitCommandTask.startToggle(manager,enabled,maintenanceSignal->notifier());
#endif
  globalLimitToolbarState=std::move(buttons);showStatus("Saving Speed Limiter settings...");updateToolbarCommands();
 }
 bool pollGlobalLimitCommand(){
  if(!globalLimitCommandTask.valid()){if(globalLimitClosePending){globalLimitClosePending=false;OnClose();return true;}return false;}if(!globalLimitCommandTask.ready())return true;
  auto result=globalLimitCommandTask.take();const bool close=globalLimitClosePending;globalLimitClosePending=false;
  for(int i=0;i<ToolbarCommandCount&&i<(int)globalLimitToolbarState.size();++i)toolbar.EnableButton(toolbarCommand(i),globalLimitToolbarState[i]);globalLimitToolbarState.clear();
  if(!result.saved){quitting=false;recoveryOnClose=false;showStatus("Could not change Speed Limiter settings. Please try On or Off again.");return true;}
  // This acknowledged result is sufficient to update the existing native menu;
  // acquiring Manager again would reintroduce a delay after a successful save.
  if(auto tasks=menu.GetSubMenu(2))if(auto limiter=tasks->GetSubMenu(12))limiter->CheckMenuRadioItem(CMD_LIMIT_ON,CMD_LIMIT_OFF,num(result.settings,"LimitKbps")>0?CMD_LIMIT_ON:CMD_LIMIT_OFF,MF_BYCOMMAND);
  showStatus("");if(close){globalLimitClosePending=true;try{maintenanceSignal->notifier()();}catch(...){/* The main timer retries an undelivered close decision. */}}return true;
 }
 void globalLimitSettingsDialog(){
  GlobalLimitSettingsTask task;Form d("Speed Limiter settings",302,89,this);d.dialogUnits=true;d.pulsePeriod=40;bool loaded=false;std::function<void()> read;CWnd* retry=nullptr;
  d.init=[&]{
   auto caption=d.control(L"STATIC","Maximum total download speed",SS_RIGHT,7,7,170,9);
   auto value=d.edit("",183,4,33,14);value->ModifyStyle(ES_LEFT,ES_RIGHT|ES_NUMBER);value->SendMessage(EM_SETLIMITTEXT,7);d.label("KBytes/sec",224,7,65);
   const auto warning=L"Note: Very low speeds can cause servers to disconnect. Avoid using Speed Limiter with servers that do not support resuming downloads.";
   auto message=d.label("Reading Speed Limiter settings...",7,23,288,26);
   auto startup=d.check("Always turn on Speed Limiter on UDM startup",false,7,52,288);
   auto ok=d.button("OK",189,68,50,[]{});d.defaultButton(ok);
   d.cancel=[&]{if(task.saving())return;task.cancelRead();d.close(IDCANCEL);};auto cancel=d.button("Cancel",245,68,50,d.cancel);
   const auto working=[&,value,startup,ok,cancel](bool busy){for(auto control:{value,startup,ok})control->EnableWindow(loaded&&!busy);cancel->EnableWindow(!task.saving());};
   const auto start=[&](std::optional<GlobalLimitSettingsDraft> draft){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
    task.start(manager,std::move(draft),d.taskNotifier(),globalLimitSettingsObserver);
#else
    task.start(manager,std::move(draft),d.taskNotifier());
#endif
   };
   d.accept=[&,value,startup,message,working,start]{if(!loaded||task.valid())return;const auto raw=trim(text(value));if(raw.empty()||raw.size()>7||raw.find_first_not_of("0123456789")!=std::string::npos)throw std::runtime_error("Enter a speed limit from 1 to 1,000,000 KB/s.");GlobalLimitSettingsDraft draft{std::stoll(raw),d.checked(startup)};validateGlobalLimitSettings(draft);
    try{start(draft);message->SetWindowText(L"Saving Speed Limiter settings...");working(true);}catch(const std::exception&){message->SetWindowText(L"Could not save settings. Your edits are kept; try OK again.");working(false);}
   };d.bind(ok,d.accept);
   read=[&,message,working,start]{retry->ShowWindow(SW_HIDE);message->SetWindowText(L"Reading Speed Limiter settings...");try{start({});working(true);}catch(const std::exception&){message->SetWindowText(L"Settings unavailable. Please retry.");working(false);retry->ShowWindow(SW_SHOW);}};
   retry=d.button("Retry",7,68,65,[&]{if(!loaded&&!task.valid())read();});retry->ShowWindow(SW_HIDE);
   d.pulse=[&,caption,value,startup,message,warning,working]{if(!task.ready())return;auto result=task.take();if(result.saved){d.close();return;}if(result.cancelled){d.cancel();return;}if(!result.error.empty()){working(false);message->SetWindowText(loaded?L"Could not save settings. Your edits are kept; try OK again.":L"Settings unavailable. Please retry.");if(!loaded)retry->ShowWindow(SW_SHOW);return;}
    caption->SetWindowText(str(result.settings,"GlobalLimitMode")=="Each download"?L"Maximum download speed for one file":L"Maximum total download speed");value->SetWindowText(cs(std::to_string(rememberedGlobalLimit(result.settings))));startup->SendMessage(BM_SETCHECK,yes(result.settings,"GlobalLimitOnStartup")?BST_CHECKED:BST_UNCHECKED);loaded=true;message->SetWindowText(warning);working(false);
   };working(true);read();
  };d.DoModal();task.finish();
 }
#include "AppearanceCommandUi.hpp"
 void chooseFont(){LOGFONTW value{};font.GetLogFont(&value);CFontDialog d(&value,CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_LIMITSIZE,nullptr,this);d.m_cf.nSizeMin=6;d.m_cf.nSizeMax=18;if(d.DoModal()!=IDOK)return;d.GetCurrentFont(&value);startAppearanceCommand({AppearanceChange::Font,utf8(value.lfFaceName),std::clamp((int)(std::abs(value.lfHeight)/scale),8,24),(int)value.lfWeight});}
 void refreshSystemTheme(){const auto settings=repaintAppearancePreferences();uiDark=useDarkUi(yes(settings,"DarkMode"),windowsHighContrast());themeFrame(m_hWnd);menuTheme.apply(m_hWnd,menu.GetSafeHmenu(),uiDark,windowDpi());table.theme(true);themeScrollbars(tree.GetSafeHwnd());status.Invalidate();tree.SetBkColor(uiDark?CategoryPaneFrame::darkTree:uiBackground());tree.SetTextColor(uiDark?RGB(255,255,255):uiForeground());refreshOpenForms();if(basket&&basket->GetSafeHwnd())basket->refreshTheme();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);renderedAppearance["DarkMode"]=yes(settings,"DarkMode");}
 afx_msg void OnSysColorChange(){CFrameWnd::OnSysColorChange();if(table.GetSafeHwnd())refreshSystemTheme();}
 afx_msg void OnSettingChange(UINT flags,LPCTSTR section){CFrameWnd::OnSettingChange(flags,section);listOrderCache.clear();invalidateDownloadStatusLocale();for(auto& row:rowSnapshots)row.reset();if(table.GetSafeHwnd())refreshSystemTheme();}
#include "MainFont.hpp"
 void applyAppearanceSnapshot(Json p){
  struct Restore{const Json*& slot;const Json* previous;~Restore(){slot=previous;}} restore{appearanceReadSnapshot,appearanceReadSnapshot};appearanceReadSnapshot=&p;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
  if(appearanceRenderObserver)appearanceRenderObserver();
#endif
  uiDark=useDarkUi(yes(p,"DarkMode"),windowsHighContrast());themeFrame(m_hWnd);menuTheme.apply(m_hWnd,menu.GetSafeHmenu(),uiDark,windowDpi());applyMainFont(p);rebuildToolbar();table.SetExtendedStyle(table.GetExtendedStyle()|LVS_EX_GRIDLINES);table.theme(true);themeScrollbars(tree.GetSafeHwnd());status.Invalidate();tree.SetBkColor(uiDark?CategoryPaneFrame::darkTree:uiBackground());tree.SetTextColor(uiDark?RGB(255,255,255):uiForeground());tree.SetItemHeight((SHORT)px(std::max(17,(int)num(p,"FontHeight",11)+5)));updateTray();CRect area;GetClientRect(&area);layout(area.Width(),area.Height());RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);refreshOpenForms();if(basket&&basket->GetSafeHwnd())basket->refreshTheme();renderedAppearance=std::move(p);}
 void applyAppearance(){applyAppearanceSnapshot(preferences());}
 void applyRenderedAppearance(){applyAppearanceSnapshot(repaintAppearancePreferences());}
 bool updateTray(){auto choice=str(appearancePreferences(),"TrayIcon","Color");tray.hIcon=classicTrayStyle(choice)?classicTrayIcon():AfxGetApp()->LoadIcon(1);if(!tray.hIcon)tray.hIcon=AfxGetApp()->LoadIcon(1);if(choice=="Hidden"){Shell_NotifyIconW(NIM_DELETE,&tray);return true;}const bool added=Shell_NotifyIconW(NIM_ADD,&tray)!=FALSE;const bool modified=Shell_NotifyIconW(NIM_MODIFY,&tray)!=FALSE;return added||modified;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiDark?mainDarkSurface:GetSysColor(COLOR_BTNFACE));return TRUE;}
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* window,UINT type){if(uiDark&&window==&categoryHeading){static CBrush headingBrush(mainDarkSurface);dc->SetTextColor(RGB(255,255,255));dc->SetBkColor(mainDarkSurface);return (HBRUSH)headingBrush.GetSafeHandle();}if(uiDark&&(type==CTLCOLOR_STATIC||type==CTLCOLOR_EDIT)){dc->SetTextColor(uiForeground());dc->SetBkColor(uiBackground());return (HBRUSH)uiBrush().GetSafeHandle();}return CFrameWnd::OnCtlColor(dc,window,type);}
