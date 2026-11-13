// Exercises only the column dialog belonging to this isolated fixture.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static fs::path renderRoot;
static CRect bounds(HWND window){CRect r;::GetWindowRect(window,&r);return r;}
static bool shown(HWND window){return (::GetWindowLongW(window,GWL_STYLE)&WS_VISIBLE)!=0;}
static void captureComponent(HWND window,const fs::path& file){
 ::ShowWindow(window,SW_SHOWNOACTIVATE);::RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);
 auto r=bounds(window);CImage image;image.Create(r.Width(),r.Height(),32);auto dc=image.GetDC();
 RECT all{0,0,r.Width(),r.Height()};FillRect(dc,&all,GetSysColorBrush(COLOR_BTNFACE));
 SendMessageW(window,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(auto child=::GetWindow(window,GW_CHILD);child;child=::GetWindow(child,GW_HWNDNEXT)){
  if(!shown(child))continue;auto c=bounds(child);auto save=SaveDC(dc);
  IntersectClipRect(dc,c.left-r.left,c.top-r.top,c.right-r.left,c.bottom-r.top);
  SetViewportOrgEx(dc,c.left-r.left,c.top-r.top,nullptr);
  SendMessageW(child,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,save);
 }
 image.ReleaseDC();if(FAILED(image.Save(file.c_str())))throw std::runtime_error("Could not render own test window");
}
static Json results=Json::array();static std::string failure;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static std::wstring label(HWND w){wchar_t text[256]{};GetWindowTextW(w,text,256);return text;}
static HWND named(HWND w,const wchar_t* name){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(label(c)==name)return c;throw std::runtime_error("Missing fixture button");}
static HWND typed(HWND w,const wchar_t* type){for(auto c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))return c;}throw std::runtime_error("Missing fixture control");}
static void click(HWND w,const wchar_t* name){auto c=named(w,name);expect(IsWindowEnabled(c)!=FALSE,"Requested column-dialog action is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void select(HWND list,int row){LVITEMW item{};item.stateMask=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,-1,(LPARAM)&item);if(row>=0){item.state=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,row,(LPARAM)&item);}}
static BOOL CALLBACK observe(HWND w,LPARAM){if(label(w)!=L"Find file")return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="Find dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){ frame.ShowWindow(SW_SHOWNOACTIVATE);
  auto key=[&](UINT code,bool control=false,bool shift=false){
  BYTE saved[256]{},state[256]{};if(!GetKeyboardState(saved))throw std::runtime_error("Read fixture keyboard state");
  state[VK_CONTROL]=control?0x80:0;state[VK_SHIFT]=shift?0x80:0;
  if(!SetKeyboardState(state))throw std::runtime_error("Set fixture keyboard state");
  MSG message{};message.hwnd=frame.table.GetSafeHwnd();message.message=WM_KEYDOWN;message.wParam=code;
  BOOL handled=frame.PreTranslateMessage(&message);SetKeyboardState(saved);return handled!=FALSE;
 };
 auto open=[&](std::function<void(HWND)> callback){action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);bool handled=key('F',true);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);expect(handled,"Ctrl+F dispatch opens the actual Find dialog");};
 auto checked=[](HWND w,const wchar_t* name){return SendMessageW(named(w,name),BM_GETCHECK,0,0)==BST_CHECKED;};
 auto a=frame.manager.add("https://fixture.example/one","","alpha-match.bin");
 auto b=frame.manager.add("https://fixture.example/two","","beta-other.bin");
 auto c=frame.manager.add("https://fixture.example/three","","gamma-match.bin");
 a->data["Category"]="Documents";b->data["Category"]="Other";c->data["Category"]="Music";
 b->data["Description"]="unique description";
 frame.sortColumn=0;frame.ascending=true;frame.refresh();select(frame.table.GetSafeHwnd(),-1);
 auto selected=[&](JobPtr job){auto chosen=frame.selected();return chosen.size()==1&&chosen.front()==job;};
 open([&](HWND w){
  expect(::GetFocus()==typed(w,L"Edit"),"Find initially focuses its text field");expect(checked(w,L"File name or part of the name"),"Find defaults to file-name matching");
  expect(!checked(w,L"Description or part of the description")&&!checked(w,L"Site name/Download link/Parent web page/Referer"),"Other Find fields are initially unchecked");
  expect(!checked(w,L"Match case")&&!checked(w,L"Match whole string only"),"Find defaults to case-insensitive substring search");
  SetWindowTextW(typed(w,L"Edit"),L"match");click(w,L"Find");
 });
 expect(selected(a)&&frame.visible.size()==2&&frame.search.IsWindowVisible(),"Find filters the list and selects the first matching row");
 expect(frame.preferences()["FindQuery"]==frame.findQuery,"Accepted Find query is saved in settings");
 expect(key(VK_F3),"F3 is handled by the main-window keyboard dispatcher");expect(selected(c),"Find Next skips intervening nonmatching rows");
 expect(key(VK_F3),"Repeated F3 is handled");expect(selected(a),"Find Next wraps from last matching row to first");
 select(frame.table.GetSafeHwnd(),1);expect(key(VK_F3)&&selected(c),"Manual selection does not replace the stored Find cursor");
 select(frame.table.GetSafeHwnd(),0);expect(key(VK_F3)&&selected(a),"Find wraps from its stored cursor after another manual selection");
 auto query=frame.findQuery;
 open([&](HWND w){expect(label(typed(w,L"Edit"))==L"match","Find reopens with the prior search text");SetWindowTextW(typed(w,L"Edit"),L"discarded");click(w,L"Match case");click(w,L"Cancel");});
 expect(frame.findQuery==query&&selected(a),"Cancel preserves the previous query and selected result");
 expect(frame.preferences()["FindQuery"]==query,"Cancel preserves the saved Find query");
 open([&](HWND w){SetWindowTextW(typed(w,L"Edit"),L"unique description");click(w,L"File name or part of the name");click(w,L"Description or part of the description");click(w,L"Match whole string only");click(w,L"Find");});
 expect(selected(b),"Description-only whole-string search selects the matching record");
 open([&](HWND w){expect(checked(w,L"Description or part of the description")&&checked(w,L"Match whole string only")&&!checked(w,L"File name or part of the name"),"Find retains chosen fields and whole-string option");
  for(UINT dpi:{96u,144u,192u}){
   auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CRect client;GetClientRect(w,&client);
   for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!shown(child))continue;auto rect=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&rect,2);expect(rect.left>=0&&rect.top>=0&&rect.right<=client.right&&rect.bottom<=client.bottom,"Find controls fit inside client area at tested DPI");}
   captureComponent(w,renderRoot/wide("find-"+std::to_string(dpi)+".png"));
  }click(w,L"Cancel");
 });
 frame.findQuery={{"Text","match"},{"FileName",true}};frame.filter="category:Music";frame.applyFindFilter();frame.refresh();select(frame.table.GetSafeHwnd(),-1);
 frame.findNext();expect(selected(c)&&frame.visible.size()==1,"Find filter and Find Next respect the current category");frame.findNext();expect(selected(c),"Single visible match wraps to itself without leaving the category");
 frame.filter="all";frame.command(CMD_QUICKFILTER);expect(key('F',true,true),"Ctrl+Shift+F is handled separately from Find");expect(::GetFocus()==frame.search.GetSafeHwnd(),"Ctrl+Shift+F focuses the quick filter");frame.search.SetWindowTextW(L"gamma");frame.searchFind.SendMessage(BM_CLICK);expect(selected(c)&&frame.visible.size()==1,"Submitted search applies the draft text to the list filter");
 {
  auto captured=frame.manager.add("https://fixture.example/cache-marker","","cache.bin");
  captured->data["ProtectedAdaptive"]=protect(Json{{"type","hls"},{"manifestUrl","https://fixture.example/playlist"},{"unused",std::string(3*1024*1024,'p')}}.dump());
  frame.findQuery={{"Text","cache-marker"},{"FileName",false},{"Address",true}};frame.applyFindFilter();frame.refresh();
  expect(frame.visible.size()==1&&frame.visible[0]==captured,"Main-window address filtering finds the large protected capture");
  {const auto before=captured->data;const auto start=std::chrono::steady_clock::now();
  for(int i=0;i<8;++i)frame.refresh();
  const auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/8;
  expect(frame.visible.size()==1&&frame.visible[0]==captured&&captured->data==before,"Repeated real list refreshes retain the match without mutating capture data");
  results.back()["timing"]={{"scope","Current main-window refresh with a three-MiB protected capture; no earlier UI or IDM baseline"},{"frames",8},{"msPerFrame",elapsed}};}
  // Release the initial immutability snapshot before later benchmark phases.
  captured->data["Url"]="https://fixture.example/changed";frame.refresh();
  expect(frame.visible.empty(),"Changed capture address invalidates a previously positive main-window filter result");
  captured->data["Url"]="https://fixture.example/cache-marker-again";frame.refresh();
  expect(frame.visible.size()==1&&frame.visible[0]==captured,"Changed address invalidates a negative main-window filter result");
  frame.findQuery["Text"]="playlist";frame.applyFindFilter();frame.refresh();
  expect(frame.visible.size()==1&&frame.visible[0]==captured,"A new query searches the retained protected playlist outside the job lock");
  const auto beforeFind=captured->data;
  frame.findNext();expect(selected(captured),"Find Next selects the capture matched through its protected playlist");
  frame.findQuery=normalizedFindQuery(frame.findQuery);
  // Freeze the 7.52 handler here for an owned-window diagnostic comparison.
  // The real current path remains frame.findNext(), not a model of that path.
  const auto priorFindNext=[&]{
   if(str(frame.findQuery,"Text").empty()||(!yes(frame.findQuery,"FileName",true)&&!yes(frame.findQuery,"Description")&&!yes(frame.findQuery,"Address")))return;
   frame.refresh();const DownloadSearch matcher(frame.findQuery);frame.findCursor.prepare(frame.findQuery);
   const auto expectedFilter=normalizedFindQuery(frame.findQuery);const bool reuse=frame.findFilterCache.isFor(expectedFilter);
   do{for(size_t index=frame.findCursor.nextRow();index<frame.visible.size();++index){Json record;std::optional<bool> matched;
     {Lock lock(frame.manager.mutex);if(reuse)matched=frame.findFilterCache.cached(frame.visible[index],frame.visible[index]->data,matcher);if(!matched.has_value())record=matcher.snapshot(frame.visible[index]->data);}
     const bool found=matched.has_value()?*matched:reuse?frame.findFilterCache.matches(frame.visible[index],std::move(record),matcher,&expectedFilter):matcher.matches(record);
     if(found){frame.table.SetItemState(-1,0,LVIS_SELECTED|LVIS_FOCUSED);frame.table.SetFocus();frame.table.EnsureVisible((int)index,TRUE);frame.table.SetItemState((int)index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);frame.findCursor.found(index);return;}
    }
   }while(frame.findCursor.wrap());
   throw std::runtime_error("Prior handler lost the protected fixture match");
  };
  std::vector<double> priorSamples,currentSamples;
  const auto measure=[&](bool prior){const auto start=std::chrono::steady_clock::now();for(int i=0;i<8;++i){if(prior)priorFindNext();else frame.findNext();}return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/8;};
  for(int trial=0;trial<7;++trial){if(trial%2){currentSamples.push_back(measure(false));priorSamples.push_back(measure(true));}else{priorSamples.push_back(measure(true));currentSamples.push_back(measure(false));}}
  auto oldSorted=priorSamples,newSorted=currentSamples;std::sort(oldSorted.begin(),oldSorted.end());std::sort(newSorted.begin(),newSorted.end());
  expect(selected(captured)&&frame.visible.size()==1,"Both Find Next paths preserve selection and wrapping on the same protected capture");
  results.back()["timing"]={{"scope","7.52 Find Next handler replay versus actual current handler in the same owned window; shared current refresh/backend, not old executable or IDM timing"},{"framesPerTrial",8},{"priorMsPerFrame",priorSamples},{"currentMsPerFrame",currentSamples},{"priorMedianMs",oldSorted[3]},{"currentMedianMs",newSorted[3]}};
  expect(captured->data==beforeFind,"Find Next leaves protected capture and transport state intact");
  const auto protectedQuery=frame.findQuery;
  frame.findQuery=normalizedFindQuery(Json{{"Text","alpha"},{"FileName",true}});frame.applyFindFilter();frame.refresh();frame.findNext();
  std::vector<double> plainPrior,plainCurrent;
  for(int trial=0;trial<7;++trial){if(trial%2){plainCurrent.push_back(measure(false));plainPrior.push_back(measure(true));}else{plainPrior.push_back(measure(true));plainCurrent.push_back(measure(false));}}
  auto plainOldSorted=plainPrior,plainNewSorted=plainCurrent;std::sort(plainOldSorted.begin(),plainOldSorted.end());std::sort(plainNewSorted.begin(),plainNewSorted.end());
  expect(selected(a)&&frame.visible.size()==1,"Both Find Next paths preserve ordinary filename selection and wrapping");
  results.back()["timing"]={{"scope","7.52 handler replay versus actual current handler for an ordinary filename in the same owned window; not old executable or IDM timing"},{"framesPerTrial",8},{"priorMsPerFrame",plainPrior},{"currentMsPerFrame",plainCurrent},{"priorMedianMs",plainOldSorted[3]},{"currentMedianMs",plainNewSorted[3]}};
  frame.findQuery=protectedQuery;frame.applyFindFilter();frame.refresh();frame.findNext();
  expect(selected(captured),"Returning from filename Find to protected address Find invalidates query-specific results");

  const auto acceptedQuery=frame.findQuery;frame.findQuery["FileName"]=false;frame.findQuery["Description"]=false;frame.findQuery["Address"]=false;
  // Close an unexpected modal on regression so this assertion can report it.
  const auto guard=SetTimer(nullptr,0,75,[](HWND,UINT,UINT_PTR,DWORD){EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{if(label(w)==L"Find file"){failure="Disabled-field Find Next opened a dialog";PostMessageW(w,WM_COMMAND,IDOK,0);}return TRUE;},0);});
  frame.findNext();KillTimer(nullptr,guard);
  expect(failure.empty()&&selected(captured)&&frame.visible.size()==1,"Find Next with all fields disabled returns without changing the view or opening a dialog");
  frame.findQuery=acceptedQuery;
  frame.showSearchBar(false);frame.refresh();
  expect(!frame.findFilterActive&&frame.visible.size()==4,"Hiding the search bar restores the unfiltered view");
  frame.findNext();expect(selected(captured),"Find Next still matches protected metadata when filtering is hidden");
  const auto hiddenQuery=normalizedFindQuery(frame.findQuery);const DownloadSearch hiddenMatcher(hiddenQuery);
  expect(frame.findFilterCache.isFor(hiddenQuery),"Hiding filtering preserves the saved query's cache identity");
  {Lock lock(frame.manager.mutex);expect(frame.findFilterCache.cached(a,a->data,hiddenMatcher)==std::optional<bool>(false)&&frame.findFilterCache.cached(captured,captured->data,hiddenMatcher)==std::optional<bool>(true),"Unfiltered Find retains source-verified negative and positive results");}
  std::vector<double> hiddenPrior,hiddenCurrent;
  for(int trial=0;trial<7;++trial){if(trial%2){hiddenCurrent.push_back(measure(false));hiddenPrior.push_back(measure(true));}else{hiddenPrior.push_back(measure(true));hiddenCurrent.push_back(measure(false));}}
  auto hiddenOldSorted=hiddenPrior,hiddenNewSorted=hiddenCurrent;std::sort(hiddenOldSorted.begin(),hiddenOldSorted.end());std::sort(hiddenNewSorted.begin(),hiddenNewSorted.end());
  expect(selected(captured)&&frame.visible.size()==4,"Both unfiltered Find Next paths preserve selection and wrapping without filtering the list");
  results.back()["timing"]={{"scope","7.52 unfiltered Find Next handler replay versus actual current handler in the same owned window; shared current refresh/backend, not old executable or IDM timing"},{"framesPerTrial",8},{"priorMsPerFrame",hiddenPrior},{"currentMsPerFrame",hiddenCurrent},{"priorMedianMs",hiddenOldSorted[3]},{"currentMedianMs",hiddenNewSorted[3]}};
  frame.findQuery=normalizedFindQuery(Json{{"Text","alpha"},{"FileName",true}});frame.refresh();frame.findNext();
  std::vector<double> hiddenPlainPrior,hiddenPlainCurrent;
  for(int trial=0;trial<7;++trial){if(trial%2){hiddenPlainCurrent.push_back(measure(false));hiddenPlainPrior.push_back(measure(true));}else{hiddenPlainPrior.push_back(measure(true));hiddenPlainCurrent.push_back(measure(false));}}
  auto hiddenPlainOld=hiddenPlainPrior,hiddenPlainNew=hiddenPlainCurrent;std::sort(hiddenPlainOld.begin(),hiddenPlainOld.end());std::sort(hiddenPlainNew.begin(),hiddenPlainNew.end());
  expect(selected(a)&&!frame.findFilterActive&&frame.visible.size()==4,"Ordinary filename Find remains unfiltered and selects the same record");
  results.back()["timing"]={{"scope","7.52 unfiltered filename handler replay versus actual current handler; shared current refresh/backend, not old executable or IDM timing"},{"framesPerTrial",8},{"priorMsPerFrame",hiddenPlainPrior},{"currentMsPerFrame",hiddenPlainCurrent},{"priorMedianMs",hiddenPlainOld[3]},{"currentMedianMs",hiddenPlainNew[3]}};
  frame.findQuery=protectedQuery;frame.refresh();frame.findNext();expect(selected(captured),"Changing a hidden search from filename to protected address invalidates prior results");
  const auto originalThird=c->data;
  c->data["Url"]="https://fixture.example/playlist-fallback";
  captured->data["ProtectedAdaptive"]=protect(Json{{"type","hls"},{"manifestUrl","https://fixture.example/different-source"}}.dump());
  frame.findCursor=DownloadFindCursor{};frame.findNext();
  expect(selected(c),"A changed unfiltered source rejects its stale positive result and finds the next matching record");
  {Lock lock(frame.manager.mutex);expect(frame.findFilterCache.cached(captured,captured->data,hiddenMatcher)==std::optional<bool>(false),"Unfiltered matching replaces the changed capture's cached positive with a verified negative");}
  captured->data=beforeFind;frame.findCursor=DownloadFindCursor{};frame.findNext();
  expect(selected(captured),"Restoring the protected source invalidates its negative result and selects it again");
  frame.filter="category:Music";frame.refresh();frame.findNext();
  expect(selected(c)&&frame.visible.size()==1&&frame.findFilterCache.entryCount()==1,"Changing category evicts hidden-search entries outside the new candidate cohort");
  {Lock lock(frame.manager.mutex);expect(!frame.findFilterCache.cached(captured,captured->data,hiddenMatcher).has_value(),"The out-of-category protected snapshot is no longer retained");}
  c->data=originalThird;frame.filter="all";frame.refresh();frame.findNext();
  expect(selected(captured)&&frame.visible.size()==4&&captured->data==beforeFind,"Returning to All Downloads recomputes missing results without changing capture data");
  {
   const auto savedQuery=frame.findQuery,savedRecord=a->data;
   const auto longText=std::wstring(FindInputUnits,L'\u00e9');
   a->data["Description"]=utf8(longText);
   frame.findQuery=normalizedFindQuery(Json{{"Text",utf8(longText)},{"FileName",false},{"Description",true},{"Address",false},{"WholeString",true}});
   frame.applyFindFilter();frame.refresh();frame.findNext();
   expect(selected(a)&&frame.visible.size()==1&&findQueryWideText(frame.findQuery).size()==FindInputUnits,"Maximum-length valid Unicode description query selects its exact match");
   const auto longQuery=frame.findQuery;std::vector<double> longPrior,longCurrent;
   for(int trial=0;trial<7;++trial){if(trial%2){longCurrent.push_back(measure(false));longPrior.push_back(measure(true));}else{longPrior.push_back(measure(true));longCurrent.push_back(measure(false));}}
   auto oldLong=longPrior,newLong=longCurrent;std::sort(oldLong.begin(),oldLong.end());std::sort(newLong.begin(),newLong.end());
   expect(selected(a)&&frame.findQuery==longQuery,"Repeated hot lookups leave the canonical Unicode query intact");
   results.back()["timing"]={{"scope","7.52 cached handler replay versus actual current handler for a 2047-unit valid Unicode description query; same owned window/current backend, not old executable or IDM timing"},{"framesPerTrial",8},{"priorMsPerFrame",longPrior},{"currentMsPerFrame",longCurrent},{"priorMedianMs",oldLong[3]},{"currentMedianMs",newLong[3]}};
   a->data=savedRecord;frame.findQuery=savedQuery;frame.showSearchBar(false);frame.refresh();frame.findNext();
   expect(selected(captured)&&!frame.findFilterActive,"Returning from a long description query restores hidden protected-address Find");
  }


 }

 }
};class FindDialogApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];renderRoot=output;fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};FindDialogApplication application;
}
