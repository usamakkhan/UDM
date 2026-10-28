// Drives and renders only native windows owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static Json mainQueue(Manager& m){for(const auto& q:m.state["Queues"])if(str(q,"Name")=="Main queue")return q;throw std::runtime_error("Missing Main queue fixture");}
static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(IsWindowVisible(c)){auto value=windowText(c);value.erase(std::remove(value.begin(),value.end(),L'&'),value.end());if(value==label)return c;}throw std::runtime_error("Missing owned test control: "+utf8(label));}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void recordTabOrder(HWND w,const wchar_t* name){
 auto tree=children(w,L"SysTreeView32").at(0),next=tree;Json order=Json::array();
 for(int i=0;i<64;++i){next=GetNextDlgTabItem(w,next,FALSE);if(!next||next==tree)break;wchar_t type[64]{};GetClassNameW(next,type,64);auto area=bounds(next);MapWindowPoints(nullptr,w,(POINT*)&area,2);order.push_back({{"caption",utf8(windowText(next))},{"class",utf8(type)},{"x",area.left},{"y",area.top}});}
 atomicText(output/name,order.dump(2),false);
}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);auto invoke=action;invoke(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}

static void rejectPartialCount(HWND editor,HWND count,const wchar_t* accept=L"OK"){
 const auto priorTitle=expectedTitle;const auto priorAction=action;bool warned=false;
 SetWindowTextW(count,L"8abc");expectedTitle=L"UDM";action=[&](HWND warning){warned=true;press(warning,L"OK");};
 press(editor,accept);expectedTitle=priorTitle;action=priorAction;
 expect(warned&&IsWindow(editor),"Invalid count shows warning and keeps server editor open for correction");
}


static HWND editAfterLabel(HWND w,const wchar_t* label){
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label){auto edit=GetWindow(c,GW_HWNDNEXT);wchar_t type[64]{};GetClassNameW(edit,type,64);if(!_wcsicmp(type,L"Edit"))return edit;}
 throw std::runtime_error("Missing scheduler edit after label: "+utf8(label));
}
class SchedulerNumberUiApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{if(argc!=2||fs::exists(argv[1]))throw std::runtime_error("Fresh fixture required");output=argv[1];fs::create_directories(output);Manager manager(output/L"state");Scheduler dialog(manager,nullptr);
 modal(dialog,L"Scheduler",[&](HWND w){
  struct Field{const wchar_t* label;const char* key;const wchar_t* good;};
  for(const auto& field:std::vector<Field>{{L"Download","Parallel",L"4"},{L"Connection retries (blank = Options)","Retries",L"3"},{L"Number of retries for each file if downloading failed:","FileRetries",L"2"},{L"Wait between file retries (seconds)","RetryDelaySeconds",L"45"},{L"Countdown (seconds)","FinishDelaySeconds",L"60"}}){
   auto editor=editAfterLabel(w,field.label);auto before=manager.state["Queues"];
   rejectPartialCount(w,editor,L"Apply");
   expect(manager.state["Queues"]==before,"Malformed scheduler field leaves every applied queue unchanged");
   SetWindowTextW(editor,field.good);press(w,L"Apply");
   expect(num(mainQueue(manager),field.key)==std::stoll(field.good),"Corrected scheduler number applies exactly");
  }
  SetWindowTextW(editAfterLabel(w,L"Connection retries (blank = Options)"),L"");press(w,L"Apply");
  expect(mainQueue(manager).at("Retries").is_null(),"Blank scheduler retries retain Options default");
  press(w,L"Close");
 });
 // IDM's Scheduler tree carries a "Download limits" node whose page edits the quota (gui-parity-2026-10-18, section 7).
 Scheduler limits(manager,nullptr);
 modal(limits,L"Scheduler",[&](HWND w){
  auto tree=children(w,L"SysTreeView32").at(0);HTREEITEM node=nullptr;for(auto item=TreeView_GetRoot(tree);item;item=TreeView_GetNextSibling(tree,item)){wchar_t text[128]{};TVITEMW info{};info.mask=TVIF_TEXT|TVIF_HANDLE;info.hItem=item;info.pszText=text;info.cchTextMax=128;TreeView_GetItem(tree,&info);if(!wcscmp(text,L"Download limits"))node=item;}
  expect(node!=nullptr,"Scheduler tree lists a Download limits node beside the queues");
  TreeView_SelectItem(tree,node);
  Json quotaTabOrder=Json::array();auto next=tree;for(int i=0;i<32;++i){next=GetNextDlgTabItem(w,next,FALSE);if(!next||next==tree)break;wchar_t type[64]{};GetClassNameW(next,type,64);quotaTabOrder.push_back({{"caption",utf8(windowText(next))},{"class",utf8(type)}});}atomicText(output/L"quota-tab-order.json",quotaTabOrder.dump(2),false);
  expect(windowText(child(w,L"Download limits"))==L"Download limits"&&!IsWindowVisible(children(w,L"SysTabControl32").at(0)),"Selecting Download limits shows its page instead of the queue tabs");
  for(auto label:{L"Download no more than",L"every"})expect((GetWindowLongW(child(w,label),GWL_STYLE)&SS_TYPEMASK)==SS_RIGHT,"Quota field labels use IDM's right-aligned static style");
  capture(w,L"scheduler-download-limits.png");
  expect(!IsWindowEnabled(child(w,L"Delete"))&&!IsWindowEnabled(child(w,L"Start now"))&&!IsWindowEnabled(child(w,L"Stop")),"Queue actions are disabled while Download limits is selected");
  HWND enable=nullptr;for(auto c:children(w,L"Button"))if(windowText(c)==L"Download limits")enable=c;expect(enable!=nullptr,"Download limits page has its enable check");auto amount=editAfterLabel(w,L"Download no more than"),hours=editAfterLabel(w,L"every");
  expect(!IsWindowEnabled(amount)&&SendMessageW(enable,BM_GETCHECK,0,0)==BST_UNCHECKED,"Limits start disabled with the amount field inactive");
  expect(GetNextDlgTabItem(w,tree,FALSE)==enable&&GetNextDlgTabItem(w,enable,TRUE)==tree,"Quota page follows the tree in forward and reverse Tab order");
  expect(GetNextDlgTabItem(w,enable,FALSE)==child(w,L"New queue"),"Disabled quota fields are skipped before bottom actions");
  SendMessageW(enable,BM_CLICK,0,0);expect(IsWindowEnabled(amount)&&IsWindowEnabled(hours),"Enabling the limit activates its fields");
  auto warning=child(w,L"Show warning before stopping downloads");
  std::vector<HWND> quotaOrder{tree,enable,amount,hours,warning,child(w,L"New queue")};
  for(size_t i=1;i<quotaOrder.size();++i)expect(GetNextDlgTabItem(w,quotaOrder[i-1],FALSE)==quotaOrder[i]&&GetNextDlgTabItem(w,quotaOrder[i],TRUE)==quotaOrder[i-1],"Enabled quota controls follow reading order in both directions");
  SetWindowTextW(amount,L"250");SetWindowTextW(hours,L"2");press(w,L"Apply");
  expect(num(manager.state["Settings"],"QuotaMb")==250&&num(manager.state["Settings"],"QuotaHours")==2,"Apply saves the download limit to the shared Options quota");
  TreeView_SelectItem(tree,TreeView_GetRoot(tree));
  expect(IsWindowVisible(children(w,L"SysTabControl32").at(0))&&IsWindowEnabled(child(w,L"Start now")),"Returning to a queue restores the tabs and queue actions");
  press(w,L"Close");
 });
 expect(num(manager.state["Settings"],"QuotaMb")==250,"Closing the Scheduler keeps the applied limit");
 {Manager reopened(manager.root);auto q=mainQueue(reopened);expect(num(q,"Parallel")==4&&q.at("Retries").is_null()&&num(q,"FileRetries")==2&&num(q,"RetryDelaySeconds")==45&&num(q,"FinishDelaySeconds")==60&&num(reopened.state["Settings"],"QuotaMb")==250,"Corrected scheduler settings and the download limit survive catalog restart");}
 // Exercise real native segmented time controls with isolated schedules.
 {Lock lock(manager.mutex);for(auto& q:manager.state["Queues"])if(str(q,"Name")=="Main queue"){q["Scheduled"]=true;q["RunOnce"]=false;q["StartMinute"]=620;q["StopMinute"]=1440;q["DailyStopEnabled"]=true;}}
 Scheduler times(manager,nullptr);
 modal(times,L"Scheduler",[&](HWND w){
  std::vector<HWND> clocks;for(auto c:children(w,L"SysDateTimePick32"))if((GetWindowLongW(c,GWL_STYLE)&DTS_TIMEFORMAT)==DTS_TIMEFORMAT)clocks.push_back(c);
  expect(clocks.size()==2,"Scheduler has native start and stop time pickers");
  auto getMinute=[](HWND c){SYSTEMTIME t{};if(DateTime_GetSystemtime(c,&t)!=GDT_VALID)throw std::runtime_error("Invalid time fixture");return t.wHour*60+t.wMinute;};
  auto setMinute=[](HWND c,int minute,int second=0){SYSTEMTIME t{};GetLocalTime(&t);t.wHour=(WORD)(minute/60);t.wMinute=(WORD)(minute%60);t.wSecond=(WORD)second;t.wMilliseconds=0;DateTime_SetSystemtime(c,GDT_VALID,&t);};
  for(auto c:clocks)expect((GetWindowLongW(c,GWL_STYLE)&DTS_UPDOWN)&&IsWindowEnabled(c)&&!children(c,UPDOWN_CLASSW).empty(),"Time picker has native spin buttons and is enabled for daily schedules");
  expect(getMinute(clocks[0])==620&&getMinute(clocks[1])==0,"Saved daily times load into the native picker");
  recordTabOrder(w,L"schedule-daily-tab-order.json");
  auto checkOrder=[&](const std::vector<HWND>& order){for(size_t i=1;i<order.size();++i)expect(GetNextDlgTabItem(w,order[i-1],FALSE)==order[i]&&GetNextDlgTabItem(w,order[i],TRUE)==order[i-1],"Schedule controls follow forward and reverse reading order");};
  checkOrder({child(w,L"Start download on UDM startup"),child(w,L"Start download at"),clocks[0],child(w,L"Daily"),child(w,L"Sunday"),child(w,L"Monday"),child(w,L"Tuesday"),child(w,L"Wednesday"),child(w,L"Thursday"),child(w,L"Friday"),child(w,L"Saturday"),child(w,L"Stop download at"),clocks[1],child(w,L"Number of retries for each file if downloading failed:")});
  checkOrder({child(w,L"New queue"),child(w,L"Delete"),child(w,L"Start now"),child(w,L"Stop"),child(w,L"Help"),child(w,L"Apply"),child(w,L"Close")});
  press(w,L"Apply");expect(num(mainQueue(manager),"StopMinute")==1440,"Unedited midnight preserves the daily end-of-day boundary");
  setMinute(clocks[0],23*60+15,25);setMinute(clocks[1],6*60+45,40);press(w,L"Apply");
  expect(num(mainQueue(manager),"StartMinute")==1395&&num(mainQueue(manager),"StopMinute")==405&&num(mainQueue(manager),"StartSecond")==25&&num(mainQueue(manager),"StopSecond")==40,"Native time edits persist an overnight schedule exactly");
  SetFocus(clocks[0]);SendMessageW(clocks[0],WM_KEYDOWN,VK_HOME,0);auto before=getMinute(clocks[0]);SendMessageW(clocks[0],WM_KEYDOWN,VK_UP,0);
  expect(getMinute(clocks[0])!=before,"Arrow key edits the selected native time segment");setMinute(clocks[0],1395);
  press(w,L"Start download at");expect(!IsWindowEnabled(clocks[0]),"Disabling scheduled start disables its time picker");
  setMinute(clocks[1],17*60+30,35);press(w,L"Stop download at");press(w,L"Apply");
  expect(yes(mainQueue(manager),"StopWithoutStartEnabled")&&num(mainQueue(manager),"StopWithoutStartMinute")==1050&&num(mainQueue(manager),"StopWithoutStartSecond")==35,"Independent stop collects the native time");
  press(w,L"Start download at");press(w,L"Once at");setMinute(clocks[0],10*60+25,45);press(w,L"Apply");
  auto once=toLocal(parseDate(mainQueue(manager).value("StartOnceUtc",Json())));
  expect(yes(mainQueue(manager),"RunOnce")&&once.wHour==10&&once.wMinute==25&&once.wSecond==45,"Run-once date combines with the selected native time");
  recordTabOrder(w,L"schedule-once-tab-order.json");
  HWND onceDate=nullptr;for(auto control:children(w,L"SysDateTimePick32"))if(IsWindowVisible(control)&&(GetWindowLongW(control,GWL_STYLE)&DTS_LONGDATEFORMAT)==DTS_LONGDATEFORMAT)onceDate=control;
  expect(onceDate!=nullptr,"Run-once navigation has a visible native date");
  checkOrder({child(w,L"Start download at"),clocks[0],child(w,L"Once at"),onceDate,child(w,L"Stop download at")});
  expect(IsWindowVisible(child(w,L"Stop download at"))&&!IsWindowEnabled(clocks[1]),"Run-once schedule exposes its stop checkbox beside the time");
  press(w,L"Stop download at");expect(IsWindowEnabled(clocks[1]),"Run-once stop checkbox enables the native time picker");setMinute(clocks[1],18*60+30,15);press(w,L"Apply");
  auto stopOnce=toLocal(parseDate(mainQueue(manager).value("StopOnceUtc",Json())));expect(stopOnce.wHour==18&&stopOnce.wMinute==30&&stopOnce.wSecond==15,"Schedule-page stop checkbox saves a run-once deadline with seconds");
  auto tab=children(w,L"SysTabControl32").at(0);auto selectTab=[&](int index){TabCtrl_SetCurSel(tab,index);NMHDR changed{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,changed.idFrom,(LPARAM)&changed);};
  selectTab(2);auto advancedStop=child(w,L"Stop on");expect(SendMessageW(advancedStop,BM_GETCHECK,0,0)==BST_CHECKED,"Advanced stop date mirrors the Schedule checkbox");press(w,L"Stop on");selectTab(0);
  expect(SendMessageW(child(w,L"Stop download at"),BM_GETCHECK,0,0)==BST_UNCHECKED&&!IsWindowEnabled(clocks[1]),"Advanced stop toggle synchronizes the Schedule checkbox and time");
  press(w,L"Apply");expect(mainQueue(manager).at("StopOnceUtc").is_null(),"Disabling run-once stop removes its deadline");
  press(w,L"Stop download at");press(w,L"Apply");
  capture(w,L"scheduler-native-times.png");press(w,L"Close");
 });
 {Manager reopened(manager.root);auto once=toLocal(parseDate(mainQueue(reopened).value("StartOnceUtc",Json())));expect(once.wHour==10&&once.wMinute==25&&once.wSecond==45,"Native scheduler time survives catalog restart");auto stop=toLocal(parseDate(mainQueue(reopened).value("StopOnceUtc",Json())));expect(stop.wHour==18&&stop.wMinute==30&&stop.wSecond==15,"Schedule-page run-once stop survives catalog restart");}
 Scheduler files(manager,nullptr);
 modal(files,L"Scheduler",[&](HWND w){
  auto tab=children(w,L"SysTabControl32").at(0);TabCtrl_SetCurSel(tab,1);NMHDR notice{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,(LPARAM)&notice);
  auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));expect(form!=nullptr,"Files-page geometry uses the actual dialog font metrics");
  auto match=[&](HWND control,int x,int y,int width,int height){auto actual=bounds(control);MapWindowPoints(nullptr,w,(POINT*)&actual,2);return actual==form->rect(x,y,width,height);};
  expect(match(children(w,L"SysListView32").at(0),154,53,264,181),"File list matches IDM dialog 307 geometry at the page offset");
  expect(match(child(w,L"Move up"),175,240,15,14),"Move-up control matches IDM's compact resource dimensions");
  expect(match(child(w,L"Move down"),155,240,15,14),"Move-down control matches IDM's compact resource dimensions");
  expect(match(child(w,L"Remove from queue"),193,240,15,14),"Remove control matches IDM's compact resource dimensions");
  capture(w,L"scheduler-files.png");press(w,L"Close");
 });
 Scheduler accessible(manager,nullptr);
 modal(accessible,L"Scheduler",[&](HWND w){
  for(const auto& label:std::vector<std::pair<const wchar_t*,const wchar_t*>>{{L"New queue",L"&New queue"},{L"Delete",L"&Delete"},{L"Start now",L"&Start now"},{L"Stop",L"S&top"},{L"Help",L"&Help"},{L"Apply",L"&Apply"},{L"Close",L"&Close"}})expect(windowText(child(w,label.first))==label.second,"Scheduler bottom action has the IDM reference mnemonic");
  std::vector<HWND> dates;for(auto c:children(w,L"SysDateTimePick32"))if((GetWindowLongW(c,GWL_STYLE)&DTS_TIMEFORMAT)!=DTS_TIMEFORMAT)dates.push_back(c);
  expect(dates.size()==2&&(GetWindowLongW(dates[0],GWL_STYLE)&DTS_LONGDATEFORMAT)==DTS_LONGDATEFORMAT,"Start date uses the reference native long-date picker");
  SYSTEMTIME chosen{};DateTime_GetSystemtime(dates[0],&chosen);wchar_t expectedDate[256]{};GetDateFormatEx(LOCALE_NAME_USER_DEFAULT,DATE_LONGDATE,&chosen,nullptr,expectedDate,256,nullptr);
  expect(windowText(dates[0])==expectedDate,"Start date follows the user's native long-date format");
  auto mnemonic=[&](wchar_t key){BYTE saved[256]{},keys[256]{};expect(GetKeyboardState(saved)!=FALSE,"Read owned dialog keyboard state");memcpy(keys,saved,sizeof(keys));keys[VK_MENU]=0x80;expect(SetKeyboardState(keys)!=FALSE,"Set owned dialog Alt modifier");SetFocus(dates[0]);MSG message{};message.hwnd=GetFocus();message.message=WM_SYSCHAR;message.wParam=key;message.lParam=1|(1L<<29);auto handled=static_cast<CWnd*>(&accessible)->PreTranslateMessage(&message);if(!handled)DispatchMessageW(&message);SetKeyboardState(saved);};
  auto parallelEdit=editAfterLabel(w,L"Download");SetWindowTextW(parallelEdit,L"5");mnemonic(L'a');
  expect(num(mainQueue(manager),"Parallel")==5,"Alt+A activates Apply through native dialog mnemonic handling");
  capture(w,L"scheduler-date-shortcuts.png");
  auto fields=children(w,L"SysDateTimePick32");std::vector<SYSTEMTIME> values;for(auto field:fields){SYSTEMTIME value{};DateTime_GetSystemtime(field,&value);values.push_back(value);}const auto priorTheme=uiDark;
  for(UINT dpi:{96u,120u,144u,192u})for(bool dark:{false,true}){
   auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);uiDark=dark;accessible.refreshTheme();CRect client;GetClientRect(w,&client);
   for(size_t i=0;i<fields.size();++i){SYSTEMTIME value{};DateTime_GetSystemtime(fields[i],&value);expect(sameScheduleMinute(value,values[i])&&value.wSecond==values[i].wSecond,"DPI and theme changes preserve native date/time values");}
   for(HWND control=GetWindow(w,GW_CHILD);control;control=GetWindow(control,GW_HWNDNEXT))if(IsWindowVisible(control)){auto area=bounds(control);MapWindowPoints(nullptr,w,(POINT*)&area,2);expect(area.left>=0&&area.top>=0&&area.right<=client.right&&area.bottom<=client.bottom,"Visible Scheduler controls stay within the client at tested DPI");}
   std::vector<CRect> bottom;for(const auto* label:{L"New queue",L"Delete",L"Start now",L"Stop",L"Help",L"Apply",L"Close"})bottom.push_back(bounds(child(w,label)));
   for(size_t i=0;i<bottom.size();++i)for(size_t j=i+1;j<bottom.size();++j){CRect overlap;expect(!overlap.IntersectRect(bottom[i],bottom[j]),"Scheduler bottom actions do not overlap at tested DPI");}
   capture(w,wide(std::string("scheduler-")+(dark?"dark-":"light-")+std::to_string(dpi)+".png").c_str());
  }
  uiDark=priorTheme;auto suggested=bounds(w);auto originalDpi=GetDpiForWindow(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(originalDpi,originalDpi),(LPARAM)&suggested);accessible.refreshTheme();mnemonic(L'c');
 });
 expect(!IsWindow(accessible.GetSafeHwnd()),"Alt+C closes the Scheduler through native mnemonic handling");
 Scheduler arrows(manager,nullptr);
 modal(arrows,L"Scheduler",[&](HWND w){
  auto once=child(w,L"Once at"),daily=child(w,L"Daily"),startup=child(w,L"Start download on UDM startup");
  press(w,L"Once at");auto startupBefore=SendMessageW(startup,BM_GETCHECK,0,0);
  auto arrow=[&](HWND from,WPARAM key){SetFocus(from);MSG message{};message.hwnd=from;message.message=WM_KEYDOWN;message.wParam=key;message.lParam=1;if(!static_cast<CWnd*>(&arrows)->PreTranslateMessage(&message))DispatchMessageW(&message);};
  arrow(once,VK_DOWN);
  expect(SendMessageW(daily,BM_GETCHECK,0,0)==BST_CHECKED&&SendMessageW(once,BM_GETCHECK,0,0)==BST_UNCHECKED,"Down arrow selects Daily in the native timing radio group");
  press(w,L"Apply");expect(yes(mainQueue(manager),"Scheduled")&&!yes(mainQueue(manager),"RunOnce"),"Arrow-selected Daily persists the daily scheduling mode");
  arrow(daily,VK_UP);
  expect(SendMessageW(once,BM_GETCHECK,0,0)==BST_CHECKED&&SendMessageW(daily,BM_GETCHECK,0,0)==BST_UNCHECKED,"Up arrow selects Once in the native timing radio group");
  expect(SendMessageW(startup,BM_GETCHECK,0,0)==startupBefore,"Timing radio navigation leaves the separate startup control unchanged");
  press(w,L"Apply");expect(yes(mainQueue(manager),"RunOnce")&&!yes(mainQueue(manager),"Scheduled"),"Arrow-selected Once persists the run-once scheduling mode");
  press(w,L"Close");
 });
 code=0;
 }catch(const std::exception& e){failure=e.what();}if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};SchedulerNumberUiApplication application;
}
