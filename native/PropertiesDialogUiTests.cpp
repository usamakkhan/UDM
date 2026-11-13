// Exercises only the properties dialog belonging to this isolated fixture.
#include <afxwin.h>
#include <shellapi.h>
#include <atomic>
static HANDLE propertiesShellEntered=CreateEventW(nullptr,TRUE,FALSE,nullptr);
static HANDLE propertiesShellRelease=CreateEventW(nullptr,TRUE,FALSE,nullptr);
static std::atomic<DWORD> propertiesShellThread{0};
static std::atomic<unsigned> propertiesShellCalls{0};
static std::atomic<bool> propertiesShellReturned{false};
static DWORD_PTR WINAPI propertiesShellInfo(LPCWSTR path,DWORD attributes,SHFILEINFOW* info,UINT size,UINT flags){
 if((flags&SHGFI_TYPENAME)&&(!wcscmp(path,L"file.udmtypeprobe")||!wcscmp(path,L"fixture.udmtypeprobe"))){
  propertiesShellThread=GetCurrentThreadId();++propertiesShellCalls;SetEvent(propertiesShellEntered);
  const auto released=WaitForSingleObject(propertiesShellRelease,5000);
  propertiesShellReturned=true;
  if(released!=WAIT_OBJECT_0)return 0;
  info->hIcon=CopyIcon(LoadIconW(nullptr,IDI_INFORMATION));
  wcscpy_s(info->szTypeName,L"UDM delayed type");
  return 1;
 }
 return SHGetFileInfoW(path,attributes,info,size,flags);
}
#define SHGetFileInfoW propertiesShellInfo
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
#undef SHGetFileInfoW
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
static void click(HWND w,const wchar_t* name){auto c=named(w,name);expect(IsWindowEnabled(c)!=FALSE,"Requested properties-dialog action is enabled");SendMessageW(c,BM_CLICK,0,0);}
struct EnableProbe {
 std::map<HWND,unsigned> changes;
 static LRESULT CALLBACK observe(HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
  auto& probe=*reinterpret_cast<EnableProbe*>(data);if(message==WM_ENABLE)++probe.changes[w];
  return DefSubclassProc(w,message,wp,lp);
 }
 explicit EnableProbe(const std::vector<HWND>& fields){for(auto field:fields){changes[field]=0;if(!SetWindowSubclass(field,observe,1,reinterpret_cast<DWORD_PTR>(this)))throw std::runtime_error("Cannot observe own Properties control");}}
 ~EnableProbe(){for(const auto& item:changes)if(IsWindow(item.first))RemoveWindowSubclass(item.first,observe,1);}
};
struct LatePropertiesUpdate {
 HWND field;bool fired=false;std::function<void()> apply;
 static LRESULT CALLBACK observe(HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
  auto& update=*reinterpret_cast<LatePropertiesUpdate*>(data);
  if(message==WM_GETTEXT&&!update.fired){update.fired=true;update.apply();}
  return DefSubclassProc(w,message,wp,lp);
 }
 LatePropertiesUpdate(HWND control,std::function<void()> action):field(control),apply(std::move(action)){if(!SetWindowSubclass(field,observe,2,reinterpret_cast<DWORD_PTR>(this)))throw std::runtime_error("Cannot observe own acceptance boundary");}
 ~LatePropertiesUpdate(){if(IsWindow(field))RemoveWindowSubclass(field,observe,2);}
};
static void select(HWND list,int row){LVITEMW item{};item.stateMask=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,-1,(LPARAM)&item);if(row>=0){item.state=LVIS_SELECTED|LVIS_FOCUSED;SendMessageW(list,LVM_SETITEMSTATE,row,(LPARAM)&item);}}
static BOOL CALLBACK observe(HWND w,LPARAM){if(!IsWindowEnabled(w)||(label(w)!=L"File Properties"&&label(w)!=L"Audio metadata"))return TRUE;try{auto once=std::move(action);if(once)once(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>15000){failure="Properties dialog deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
class ToolbarComponentTest {
public:static void run(MainWindow& frame){
 frame.ShowWindow(SW_SHOWNOACTIVATE);
 auto job=frame.manager.add("https://fixture.example/download","","fixture.bin");job->data["Description"]="original";
 auto open=[&](std::function<void(HWND)> callback,bool doubleClick=false,int menuChoice=0,AsyncFileStatus* locationProbe=nullptr){action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,75,tick);if(doubleClick){auto at=std::find(frame.visible.begin(),frame.visible.end(),job);expect(at!=frame.visible.end(),"Completed download is visible before double-click");NMITEMACTIVATE notification{};notification.hdr.hwndFrom=frame.table.GetSafeHwnd();notification.hdr.code=NM_DBLCLK;notification.iItem=(int)std::distance(frame.visible.begin(),at);LRESULT result=0;expect(frame.OnNotify(0,(LPARAM)&notification,&result)!=FALSE,"Main-window double-click notification is handled");}else fileProperties(&frame,frame.manager,job,[&](CMenu& menu,CPoint,CWnd*){expect(menu.GetMenuItemCount()==(hasAudioMetadata(job->data)?8:7),"Properties More menu contains all six actions and separator");if(menuChoice==5)expect((menu.GetMenuState(5,MF_BYCOMMAND)&MF_GRAYED)==0,"Inactive ordinary download enables Show password");return menuChoice;},locationProbe);KillTimer(nullptr,timer);if(!failure.empty())throw std::runtime_error(failure);};
 auto edits=[](HWND w){std::vector<HWND> list;for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){wchar_t type[80]{};GetClassNameW(child,type,80);if(!_wcsicmp(type,L"Edit"))list.push_back(child);}return list;};
 {
  const auto originalName=job->data["FileName"];
  job->data["FileName"]="fixture.udmtypeprobe";
  auto original=job->snapshot();
  std::function<void(HWND)> waitForLookup;
  waitForLookup=[&](HWND w){
   if(!IsWindowVisible(w)||WaitForSingleObject(propertiesShellEntered,0)!=WAIT_OBJECT_0){action=waitForLookup;return;}
   expect(propertiesShellThread!=GetCurrentThreadId(),"Properties resolves Shell details on a worker thread");
   expect(!propertiesShellReturned&&IsWindowEnabled(named(w,L"OK")),"Properties is visible and actionable while Shell lookup is blocked");
   expect(label(named(w,L"File"))==L"File","Pending Shell details have a generic type label");
   expect(edits(w).size()==6,"All Properties edit controls exist before Shell lookup finishes");
   SendMessageW(w,WM_CLOSE,0,0);
  };
  open(waitForLookup);
  expect(job->snapshot()==original,"Closing Properties during Shell lookup preserves saved metadata");
  std::function<void(HWND)> waitForDetails;
  waitForDetails=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);
   bool ready=false;for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT))ready|=label(child)==L"UDM delayed type";
   if(!ready){action=waitForDetails;return;}
   expect(propertiesShellCalls==1,"Reopening Properties shares the pending Shell request");
   expect(IsWindowEnabled(named(w,L"OK")),"Late file details preserve the actionable Properties dialog");
   auto fields=edits(w);expect(label(fields[2])==L"typed while loading","Late file details preserve user edits");
   SendMessageW(w,WM_CLOSE,0,0);
  };
  std::function<void(HWND)> reopenWhenVisible;
  reopenWhenVisible=[&](HWND w){
   if(!IsWindowVisible(w)){action=reopenWhenVisible;return;}
   expect(!propertiesShellReturned&&IsWindowEnabled(named(w,L"OK")),"Properties can reopen before the shared Shell lookup returns");
   SetWindowTextW(edits(w)[2],L"typed while loading");
   SetEvent(propertiesShellRelease);action=waitForDetails;
  };
  open(reopenWhenVisible);
  expect(job->snapshot()==original,"Late Shell completion does not commit canceled edits");
  open([&](HWND w){
   expect(named(w,L"UDM delayed type")!=nullptr&&propertiesShellCalls==1,"Properties reuses cached file details on the next opening");
   SendMessageW(w,WM_CLOSE,0,0);
  });
  job->data["FileName"]=originalName;
 }
 // Initialize timer-maintained fields before comparing complete saved records.
 frame.manager.tick();
 auto before=job->snapshot();
 open([&](HWND w){auto fields=edits(w);expect(fields.size()==6,"Properties exposes destination, address, description, Referer, login and password");expect(!IsWindowEnabled(named(w,L"Open")),"Incomplete file cannot be opened");expect(IsWindowEnabled(named(w,L"&Browse...")),"Inactive destination can be browsed");expect(SendMessageW(fields[5],EM_GETPASSWORDCHAR,0,0)!=0,"Password is masked by default");SetWindowTextW(fields[2],L"discarded");SendMessageW(w,WM_CLOSE,0,0);});
 expect(job->snapshot()==before,"Closing Properties without OK preserves the entire record");
 {
  const auto original=job->snapshot();
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[4],L"origin-bound-user");SetWindowTextW(fields[5],L"origin-bound-password");
   LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);job->data["Url"]="https://another-fixture.example/download";job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(Headers{{"Authorization","Bearer other-origin"},{"Cookie","fixture=other-origin"}})).dump());});
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));bool rejected=false;
   try{form->accept();}catch(const std::exception& e){rejected=std::string(e.what()).find("another site")!=std::string::npos;}
   expect(update.fired&&rejected&&IsWindow(w),"A late cross-origin target change rejects staged sensitive request edits without closing Properties");
   expect(headerValue(readHeaders(job->data),"Authorization")=="Bearer other-origin"&&headerValue(readHeaders(job->data),"Cookie")=="fixture=other-origin","Rejected cross-origin edits preserve the current target's session");
   expect(label(fields[4])==L"origin-bound-user"&&label(fields[5])==L"origin-bound-password","Origin rejection preserves the typed credential draft");
   SendMessageW(w,WM_CLOSE,0,0);
  });job->data=original;
 }
 for(unsigned edited=0;edited<4;++edited){
  const auto original=job->snapshot();Headers initialHeaders{{"Authorization","Bearer fixture-old"},{"Cookie","fixture=old"},{"Referer","https://fixture.example/old-source"}};
  job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(initialHeaders)).dump());std::string latestProtected;
  open([&](HWND w){
   auto fields=edits(w);if(edited&1){SetWindowTextW(fields[4],L"chosen-user");SetWindowTextW(fields[5],L"chosen-password");}if(edited&2)SetWindowTextW(fields[3],L"https://fixture.example/chosen-source");
   LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);Headers latest{{"Authorization","Bearer fixture-new"},{"Cookie","fixture=new"},{"Referer","https://fixture.example/new-source"}};latestProtected=protect(legacyDictionary(Json(latest)).dump());job->data["ProtectedHeaders"]=latestProtected;});
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();expect(update.fired,"A session-header refresh ran after Properties staged its request edits");
  });
  auto committed=readHeaders(job->data);expect(headerValue(committed,"Cookie")=="fixture=new","Request edits preserve a cookie refreshed after acceptance staging");
  expect((edited&1)?basicLogin(committed)==std::make_pair(std::string("chosen-user"),std::string("chosen-password")):headerValue(committed,"Authorization")=="Bearer fixture-new","Explicit login edits own Authorization while untouched login retains refreshed authentication");
  expect(headerValue(committed,"Referer")==((edited&2)?"https://fixture.example/chosen-source":"https://fixture.example/new-source"),"Referer edits own only their header and retain other refreshed request state");
  if(!edited)expect(str(job->data,"ProtectedHeaders")==latestProtected,"Untouched request headers retain their exact refreshed protected representation");job->data=original;
 }
 for(const bool captured:{false,true}){
  const auto original=job->snapshot();const auto source=job->target(),destination=source.parent_path()/L"session-properties-move.bin";fs::create_directories(source.parent_path());atomicText(source,"fixture bytes",false);
  job->data["Status"]="Complete";job->data["QueueMember"]=false;job->data["Size"]=13;job->data["Received"]=13;if(captured)job->data["SourceUrl"]="https://fixture.example/watch";
  job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(Headers{{"Cookie","fixture=old"},{"Referer","https://fixture.example/old-source"}})).dump());
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[0],destination.c_str());SetWindowTextW(fields[3],L"https://fixture.example/chosen-source");
   LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(Headers{{"Authorization","Bearer fixture-new"},{"Cookie","fixture=new"},{"Referer","https://fixture.example/new-source"}})).dump());});
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();expect(update.fired,"Completed-file relocation receives a session refresh after request staging");
  });
  const auto committed=readHeaders(job->data);expect(headerValue(committed,"Cookie")=="fixture=new"&&headerValue(committed,"Authorization")=="Bearer fixture-new"&&headerValue(committed,"Referer")=="https://fixture.example/chosen-source","A completed request edit merges with the current session after relocation");
  expect(job->target()==destination&&!fs::exists(source)&&readText(destination)=="fixture bytes","Session-header reconciliation preserves completed-file relocation bytes");fs::remove(destination);job->data=original;frame.manager.save();
 }
 {
  const auto original=job->snapshot();const auto originalTarget=job->target();
  const auto external=originalTarget.parent_path()/L"external-location"/L"external-name.bin",explicitTarget=originalTarget.parent_path()/L"explicit-location"/L"explicit-name.bin";
  const auto changeDestination=[&]{job->data["Folder"]=utf8(external.parent_path().wstring());job->data["FileName"]=utf8(external.filename().wstring());};
  open([&](HWND w){changeDestination();dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();});
  expect(job->target()==external,"An untouched incomplete destination preserves an external folder and filename change before refresh");job->data=original;
  for(const bool edited:{false,true}){
   open([&](HWND w){
    auto fields=edits(w);if(edited)SetWindowTextW(fields[0],explicitTarget.c_str());
    LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);changeDestination();});
    dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
    expect(update.fired,"A late destination update ran at the real acceptance boundary");
   });
   expect(job->target()==(edited?explicitTarget:external),"Destination commit keeps late untouched locations while explicit paths take precedence");job->data=original;
  }
  open([&](HWND w){
   auto fields=edits(w);changeDestination();SendMessageW(w,WM_TIMER,7,0);
   expect(label(fields[0])==external.wstring(),"Untouched incomplete destination display follows external folder and filename changes");
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
  });
  expect(job->target()==external,"Accepting a refreshed destination retains the live location");job->data=original;
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[0],L"");SetWindowTextW(fields[2],L"retained after invalid path");
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));bool rejected=false;
   try{form->accept();}catch(const std::exception&){rejected=true;}
   expect(rejected&&IsWindow(w)&&job->target()==originalTarget,"An explicitly cleared destination is validated instead of treated as untouched");
   expect(label(fields[2])==L"retained after invalid path","Destination validation retains the remaining draft");
   SetWindowTextW(fields[0],explicitTarget.c_str());form->accept();
  });
  expect(job->target()==explicitTarget&&str(job->data,"Description")=="retained after invalid path","A corrected explicit destination saves with its retained draft");job->data=original;
 }
 for(unsigned edited=0;edited<4;++edited){
  const auto original=job->snapshot();
  open([&](HWND w){
   auto fields=edits(w);
   if(edited&1)SetWindowTextW(fields[1],L"https://fixture.example/explicit-commit");
   if(edited&2)SetWindowTextW(fields[2],L"explicit commit description");
   LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);job->data["Url"]="https://fixture.example/late-commit";job->data["Description"]="late commit description";});
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
   expect(update.fired,"The external metadata update ran after acceptance staging and before configure");
  });
  expect(str(job->data,"Url")== ((edited&1)?"https://fixture.example/explicit-commit":"https://fixture.example/late-commit")&&str(job->data,"Description")== ((edited&2)?"explicit commit description":"late commit description"),"Commit preserves late untouched metadata while explicit edits take precedence");
  job->data=original;
 }
 for(const bool captured:{false,true}){
  const auto original=job->snapshot();const auto source=job->target(),destination=source.parent_path()/L"late-properties-move.bin";
  fs::create_directories(source.parent_path());atomicText(source,"fixture bytes",false);
  job->data["Status"]="Complete";job->data["QueueMember"]=false;job->data["Size"]=13;job->data["Received"]=13;
  if(captured)job->data["SourceUrl"]="https://fixture.example/watch";
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[0],destination.c_str());
   LatePropertiesUpdate update(fields[0],[&]{Lock lock(frame.manager.mutex);job->data["Url"]="https://fixture.example/late-moved";job->data["Description"]="late moved description";});
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
   expect(update.fired,"The completed-file update ran before the physical relocation and metadata commit");
  });
  expect(job->target()==destination&&!fs::exists(source)&&readText(destination)=="fixture bytes","Late metadata reconciliation preserves the physical completed-file move");
  expect(str(job->data,"Url")=="https://fixture.example/late-moved"&&str(job->data,"Description")=="late moved description","Completed and captured-media commits preserve late untouched metadata after relocation");
  fs::remove(destination);job->data=original;frame.manager.save();
 }
 {
  const auto original=job->snapshot();
  open([&](HWND w){
   job->data["Url"]="https://fixture.example/refreshed-before-save";job->data["Description"]="changed outside Properties";
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
  });
  expect(str(job->data,"Url")=="https://fixture.example/refreshed-before-save"&&str(job->data,"Description")=="changed outside Properties","Untouched address and description preserve external changes even before the timer refresh");
  job->data=original;
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[2],L"typed description");SendMessageW(fields[1],EM_SETSEL,8,15);job->data["Url"]="https://fixture.example/refreshed-visible";job->data["Description"]="external description";
   SendMessageW(w,WM_TIMER,7,0);
   expect(label(fields[1])==L"https://fixture.example/refreshed-visible","Untouched address follows the live refreshed URL");
   DWORD start=0,end=0;SendMessageW(fields[1],EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));expect(start==8&&end==15,"Updating an untouched address preserves its selection offsets");
   expect(label(fields[2])==L"typed description","An external description cannot overwrite a typed draft");
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
  });
  expect(str(job->data,"Url")=="https://fixture.example/refreshed-visible"&&str(job->data,"Description")=="typed description","Saving combines the current untouched URL with the typed description");
  job->data=original;
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[1],L"https://fixture.example/typed-address");SendMessageW(fields[1],EM_SETSEL,8,15);
   job->data["Url"]="https://fixture.example/external-address";job->data["Description"]="visible external description";
   SendMessageW(w,WM_TIMER,7,0);
   expect(label(fields[1])==L"https://fixture.example/typed-address","An external URL cannot overwrite a typed address");
   expect(label(fields[2])==L"visible external description","Untouched description follows live metadata");
   DWORD start=0,end=0;SendMessageW(fields[1],EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));expect(start==8&&end==15,"Live metadata refresh preserves selection in a typed address");
   dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
  });
  expect(str(job->data,"Url")=="https://fixture.example/typed-address"&&str(job->data,"Description")=="visible external description","Explicit URL edits win while untouched descriptions retain live values");
  job->data=original;
 }
 for(const bool captured:{false,true}){
  const auto original=job->snapshot();if(captured)job->data["SourceUrl"]="https://fixture.example/watch";
  open([&](HWND w){
   auto fields=edits(w);EnableProbe probe(fields);
   SetWindowTextW(fields[2],L"retained selection");SendMessageW(fields[2],EM_SETSEL,2,8);
   for(int i=0;i<5;++i)SendMessageW(w,WM_TIMER,7,0);
   for(auto field:fields)expect(probe.changes[field]==0,"Stable Properties controls do not toggle enabled state during timer refresh");
   if(captured)expect(!IsWindowEnabled(fields[4])&&!IsWindowEnabled(fields[5]),"Captured-media credentials stay disabled between timer refreshes");
   job->data["OriginalResponse"]={{"State","Receiving"}};job->data["Status"]="Downloading";
   SendMessageW(w,WM_TIMER,7,0);for(int i=0;i<5;++i)SendMessageW(w,WM_TIMER,7,0);
   for(size_t i=0;i<fields.size();++i)expect(probe.changes[fields[i]]==(captured&&i>=4?0u:1u),"A background start changes each eligible field state only once");
   job->data.erase("OriginalResponse");job->data["Status"]="Paused";
   SendMessageW(w,WM_TIMER,7,0);for(int i=0;i<5;++i)SendMessageW(w,WM_TIMER,7,0);
   for(size_t i=0;i<fields.size();++i)expect(probe.changes[fields[i]]==(captured&&i>=4?0u:2u),"A background stop restores eligible fields once without credential flapping");
   DWORD start=0,end=0;SendMessageW(fields[2],EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));
   expect(label(fields[2])==L"retained selection"&&start==2&&end==8,"Lifecycle refresh preserves typed text and its selection");
   SendMessageW(w,WM_CLOSE,0,0);
  });
  job->data=original;
 }
 open([&](HWND w){auto fields=edits(w);SetWindowTextW(fields[2],L"saved description");SetWindowTextW(fields[3],L"https://fixture.example/source");SetWindowTextW(fields[4],L"fixture-user");SetWindowTextW(fields[5],L"fixture-password");click(w,L"OK");});
 expect(str(job->data,"Description")=="saved description","OK saves description");auto headers=readHeaders(job->data);expect(headerValue(headers,"Referer")=="https://fixture.example/source","OK saves Referer");expect(basicLogin(headers)==std::make_pair(std::string("fixture-user"),std::string("fixture-password")),"OK saves protected basic authentication");
 auto saved=job->snapshot();
 {
  const std::wstring oversized=L"https://fixture.example/"+std::wstring(16400,L'x');
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[2],L"retained after header correction");SetWindowTextW(fields[3],oversized.c_str());SetWindowTextW(fields[4],L"corrected-user");SetWindowTextW(fields[5],L"corrected-password");
   expect(GetWindowTextLengthW(fields[3])>16384,"The real Referer control contains the rejected header value");
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));
   for(int attempt=0;attempt<2;++attempt){bool rejected=false;try{form->accept();}catch(const std::exception& e){rejected=std::string(e.what())=="Invalid request header.";}expect(rejected&&IsWindow(w),"Repeated invalid request input keeps Properties open and remains rejected");expect(job->snapshot()==saved,"Failed header validation does not save credentials or other metadata");}
   SetWindowTextW(fields[3],L"https://fixture.example/corrected");click(w,L"OK");
  });
  expect(basicLogin(readHeaders(job->data))==std::make_pair(std::string("corrected-user"),std::string("corrected-password")),"Correcting the rejected header preserves the typed login in the real dialog");
  expect(headerValue(readHeaders(job->data),"Referer")=="https://fixture.example/corrected"&&str(job->data,"Description")=="retained after header correction","One successful Save commits the corrected Referer and retained description");
  job->data=saved;frame.manager.save();
  open([&](HWND w){auto fields=edits(w);SetWindowTextW(fields[3],oversized.c_str());SetWindowTextW(fields[4],L"discarded-corrected-user");auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));bool rejected=false;try{form->accept();}catch(const std::exception&){rejected=true;}expect(rejected,"Cancel scenario first rejects its invalid request edit");SendMessageW(w,WM_CLOSE,0,0);});
  expect(job->snapshot()==saved,"Cancel after request validation failure preserves the original download");
  open([&](HWND w){auto fields=edits(w);SetWindowTextW(fields[1],L"not a URL");SetWindowTextW(fields[4],L"later-validation-user");SetWindowTextW(fields[5],L"later-validation-password");auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));bool rejected=false;try{form->accept();}catch(const std::exception&){rejected=true;}expect(rejected&&job->snapshot()==saved,"A later metadata validation failure leaves the saved request unchanged");SetWindowTextW(fields[1],L"https://fixture.example/download");click(w,L"OK");});
  expect(basicLogin(readHeaders(job->data))==std::make_pair(std::string("later-validation-user"),std::string("later-validation-password")),"Correcting a later metadata error preserves the already staged credentials");
  job->data=saved;frame.manager.save();
 }
 open([&](HWND w){auto fields=edits(w);click(w,L"More...");expect(SendMessageW(fields[5],EM_GETPASSWORDCHAR,0,0)==0,"More Show password reveals the current password");expect(label(fields[5])==L"fixture-password","Revealing password preserves its contents");click(w,L"More...");expect(SendMessageW(fields[5],EM_GETPASSWORDCHAR,0,0)!=0,"More Show password toggles masking back on");SendMessageW(w,WM_CLOSE,0,0);},false,5);
 expect(job->snapshot()==saved,"Password visibility does not modify saved metadata");
 open([&](HWND w){auto fields=edits(w);SetWindowTextW(fields[2],L"discarded menu description");SetWindowTextW(fields[4],L"discarded-user");SetWindowTextW(fields[5],L"discarded-secret");click(w,L"More...");},false,6);
 if(job->snapshot()!=saved)atomicText(renderRoot/L"cancel-difference.json",Json{{"before",saved},{"after",job->snapshot()}}.dump(2),false);
 expect(job->snapshot()==saved,"More Cancel edits discards description and credential changes together");
 job->data["AudioMetadata"]={{"Title","Fixture title"},{"Artist","Fixture artist"},{"Album","Fixture album"}};
 auto tagged=job->snapshot();
 open([&](HWND w){
  action=[&](HWND audio){
   expect(label(audio)==L"Audio metadata","Properties More opens audio metadata");auto fields=edits(audio);expect(fields.size()==3,"Audio metadata has three fields");
   expect(label(fields[0])==L"Fixture title"&&label(fields[1])==L"Fixture artist"&&label(fields[2])==L"Fixture album","Audio tag fields display saved values");
   for(auto field:fields)expect((GetWindowLongW(field,GWL_STYLE)&ES_READONLY)!=0,"Audio metadata fields are read-only");
   captureComponent(audio,renderRoot/L"audio-metadata.png");click(audio,L"Close");
  };
  click(w,L"More...");SendMessageW(w,WM_CLOSE,0,0);
 },false,7);
 expect(job->snapshot()==tagged,"Viewing audio metadata preserves the download record");job->data.erase("AudioMetadata");
 {
  const auto original=job->snapshot();
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[2],L"retained while queued download starts");
   job->data["OriginalResponse"]={{"State","Receiving"}};job->data["Status"]="Downloading";job->data["Size"]=100;job->data["Received"]=25;
   auto refreshed=readHeaders(job->data);setHeader(refreshed,"Authorization","Bearer fixture-refreshed-session");setHeader(refreshed,"Referer","https://fixture.example/refreshed-source");job->data["ProtectedHeaders"]=protect(legacyDictionary(Json(refreshed)).dump());
   // Invoke the same acceptance callback before the periodic UI refresh.
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));expect(form!=nullptr,"Properties exposes its real acceptance callback");
   bool rejected=false;try{form->accept();}catch(const std::exception& e){rejected=std::string(e.what()).find("Your edits are still in this dialog")!=std::string::npos;}
   expect(rejected&&IsWindow(w),"Saving after a background start retains the dialog and draft instead of committing stale properties");
   SendMessageW(w,WM_TIMER,7,0);
   for(auto field:fields)expect(!IsWindowEnabled(field),"An open editable Properties dialog disables edits when the download starts");
   expect(!IsWindowEnabled(named(w,L"&Browse...")),"Starting the download disables destination browsing in the same dialog");
   expect(label(named(w,L"Downloading (25.00% complete)"))==L"Downloading (25.00% complete)","Properties status follows current transfer progress");
   job->data.erase("OriginalResponse");job->data["Status"]="Paused";SendMessageW(w,WM_TIMER,7,0);
   for(auto field:fields)expect(IsWindowEnabled(field)!=FALSE,"Stopping the download re-enables the existing Properties fields");
   expect(label(fields[2])==L"retained while queued download starts","A start/stop transition preserves the typed description");
   click(w,L"OK");
  });
  expect(str(job->data,"Description")=="retained while queued download starts","The retained draft saves after the job stops");
  expect(headerValue(readHeaders(job->data),"Authorization")=="Bearer fixture-refreshed-session"&&headerValue(readHeaders(job->data),"Referer")=="https://fixture.example/refreshed-source","Untouched login and Referer controls cannot overwrite refreshed request credentials");
  job->data=original;frame.manager.save();
 }
 {
  const auto original=job->snapshot();const auto source=job->target();fs::create_directories(source.parent_path());atomicText(source,"fixture bytes",false);
  job->data["OriginalResponse"]={{"State","Receiving"}};
  open([&](HWND w){
   auto fields=edits(w);job->data.erase("OriginalResponse");job->data["Status"]="Complete";job->data["Size"]=13;job->data["Received"]=13;job->data["QueueMember"]=false;
   SendMessageW(w,WM_TIMER,7,0);
   expect(IsWindowEnabled(fields[2])!=FALSE&&IsWindowEnabled(named(w,L"&Move")),"Finishing an active download enables completed Properties actions without reopening");
   expect((GetWindowLongW(fields[0],GWL_STYLE)&ES_READONLY)!=0,"An unchanged destination becomes read-only after completion");
   expect(label(named(w,L"Complete"))==L"Complete","Properties shows the completed state in the same window");
   captureComponent(w,renderRoot/L"properties-live-complete.png");
   SetWindowTextW(fields[2],L"edited after completion");dynamic_cast<Form*>(CWnd::FromHandlePermanent(w))->accept();
  });
  expect(str(job->data,"Description")=="edited after completion"&&str(job->data,"Status")=="Complete"&&!yes(job->data,"QueueMember"),"A dialog opened while active saves completed metadata without restoring old queue membership");
  expect(readText(source)=="fixture bytes","Completed metadata editing preserves the published bytes");
  fs::remove(source);job->data=original;frame.manager.save();
 }
 {
  const auto original=job->snapshot();const auto source=job->target(),destination=source.parent_path()/L"finished-during-properties.bin";fs::create_directories(source.parent_path());atomicText(source,"fixture bytes",false);
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[0],destination.c_str());SetWindowTextW(fields[2],L"saved with physical move");
   job->data["Status"]="Complete";job->data["Size"]=13;job->data["Received"]=13;job->data["QueueMember"]=false;SendMessageW(w,WM_TIMER,7,0);
   expect(label(fields[0])==destination.wstring(),"Completion does not overwrite an already typed destination");
   expect((GetWindowLongW(fields[0],GWL_STYLE)&ES_READONLY)==0,"A pending destination remains editable for the explicit save");
   click(w,L"OK");
  });
  expect(job->target()==destination&&fs::is_regular_file(destination)&&!fs::exists(source),"Saving a destination entered before completion physically moves the completed file");
  expect(readText(destination)=="fixture bytes"&&str(job->data,"Description")=="saved with physical move","The completion transition preserves file bytes and saves the remaining draft");
  fs::remove(destination);job->data=original;frame.manager.save();
 }
 {
  const auto original=job->snapshot();const auto source=job->target(),occupied=source.parent_path()/L"occupied-properties.bin",destination=source.parent_path()/L"retried-properties-move.bin";
  fs::create_directories(source.parent_path());atomicText(source,"fixture bytes",false);atomicText(occupied,"keep occupied bytes",false);
  open([&](HWND w){
   auto fields=edits(w);SetWindowTextW(fields[0],occupied.c_str());SetWindowTextW(fields[2],L"retained after failed move");job->data["Status"]="Complete";job->data["Size"]=13;job->data["Received"]=13;job->data["QueueMember"]=false;SendMessageW(w,WM_TIMER,7,0);
   auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));bool rejected=false;try{form->accept();}catch(const std::exception&){rejected=true;}
   expect(rejected&&IsWindow(w),"A failed completion-transition move retains Properties for correction");
   expect(job->target()==source&&readText(source)=="fixture bytes"&&readText(occupied)=="keep occupied bytes","A destination collision preserves the published file and the existing destination");
   expect(label(fields[2])==L"retained after failed move"&&str(job->data,"Description")!=utf8(label(fields[2])),"A failed move preserves the unsaved draft without committing its metadata");
   SetWindowTextW(fields[0],destination.c_str());click(w,L"OK");
  });
  expect(job->target()==destination&&readText(destination)=="fixture bytes"&&str(job->data,"Description")=="retained after failed move","Correcting the destination retries the move and saves the retained draft");
  fs::remove(destination);fs::remove(occupied);job->data=original;frame.manager.save();
 }
 for(const auto& captureState:{"Reserved","Receiving"}){
 job->data["OriginalResponse"]={{"State",captureState}};
 try{open([&](HWND w){for(auto field:edits(w))expect(IsWindowEnabled(field)==FALSE,"Active download Properties fields cannot be edited");expect(!IsWindowEnabled(named(w,L"&Browse...")),"Active download cannot change destination");click(w,L"OK");});}catch(...){job->data.erase("OriginalResponse");throw;}
 job->data.erase("OriginalResponse");auto afterActive=job->snapshot();auto beforeActive=saved;afterActive.erase("PeakSpeed");beforeActive.erase("PeakSpeed");expect(afterActive==beforeActive,"Accepting active Properties preserves saved metadata apart from timer-maintained peak speed");
 }
 fs::create_directories(job->target().parent_path());atomicText(job->target(),"fixture bytes",false);job->data["Status"]="Complete";job->data["Size"]=13;job->data["Received"]=13;job->data["QueueMember"]=false;
 {
  struct LocationGate {
   Handle entered{CreateEventW(nullptr,TRUE,FALSE,nullptr)},release{CreateEventW(nullptr,TRUE,FALSE,nullptr)};
   std::atomic<DWORD> thread{0};std::atomic<unsigned> calls{0};std::atomic<int> mode{0};
   std::atomic<bool> returned{false};
  };
  static LocationGate gate;
  AsyncFileStatus locations([](const fs::path& target,std::error_code& error){
   gate.thread=GetCurrentThreadId();++gate.calls;SetEvent(gate.entered.h);
   const auto released=WaitForSingleObject(gate.release.h,5000);gate.returned=true;
   if(released!=WAIT_OBJECT_0){error=std::make_error_code(std::errc::timed_out);return fs::file_status(fs::file_type::none);}
   if(gate.mode==1){error=std::make_error_code(std::errc::no_such_file_or_directory);return fs::file_status(fs::file_type::not_found);}
   if(gate.mode==2){error=std::make_error_code(std::errc::permission_denied);return fs::file_status(fs::file_type::none);}
   if(gate.mode==3)return fs::file_status(fs::file_type::directory);
   return fs::status(target,error);
  });
  const auto original=job->snapshot();
  std::function<void(HWND)> waitForProbe;
  waitForProbe=[&](HWND w){
   if(!IsWindowVisible(w)||WaitForSingleObject(gate.entered.h,0)!=WAIT_OBJECT_0){action=waitForProbe;return;}
   expect(gate.thread!=GetCurrentThreadId(),"Properties checks a completed file location on a worker thread");
   expect(!gate.returned&&IsWindowEnabled(named(w,L"OK")),"Properties is actionable while the completed-file status probe is blocked");
   expect(label(edits(w)[0])==job->target().wstring(),"Properties shows the saved destination while file status is pending");
   SendMessageW(w,WM_CLOSE,0,0);
  };
  open(waitForProbe,false,0,&locations);
  expect(job->snapshot()==original,"Closing during a location probe preserves completed download metadata");
  std::function<void(HWND)> waitForPresent,waitForMissing,waitForError,waitForDirectory,waitForRestore;
  waitForRestore=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);const auto status=locations.observe({job->target()});
   if(!status||!status->regular(job->target())){action=waitForRestore;return;}
   expect(label(edits(w)[0])==job->target().wstring(),"Restoring a completed file keeps its destination visible");
   SendMessageW(w,WM_CLOSE,0,0);
  };
  waitForDirectory=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);const auto status=locations.observe({job->target()});
   if(!status||!status->files.count(job->target())||status->files.at(job->target())!=fs::file_type::directory){action=waitForDirectory;return;}
   expect(!status->regular(job->target())&&!status->missing(job->target()),"A directory is distinguished from a regular file and a missing path");
   expect(label(edits(w)[0])==job->target().wstring(),"An existing directory does not produce a moved-file message");
   gate.mode=0;action=waitForRestore;
  };
  waitForError=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);const auto status=locations.observe({job->target()});
   if(!status||!status->files.count(job->target())||status->files.at(job->target())!=fs::file_type::none){action=waitForError;return;}
   SendMessageW(w,WM_TIMER,7,0);
   expect(!status->regular(job->target())&&!status->missing(job->target()),"A failed location probe remains distinct from a missing file");
   expect(label(edits(w)[0])==job->target().wstring(),"A location lookup error restores the saved destination text");
   gate.mode=3;action=waitForDirectory;
  };
  waitForMissing=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);
   if(label(edits(w)[0])!=L"The file has been moved."){action=waitForMissing;return;}
   expect(true,"A confirmed missing file updates the destination in the same Properties window");
   expect(label(edits(w)[2])==L"location probe edit","Location status updates preserve edited description text");
   gate.mode=2;action=waitForError;
  };
  waitForPresent=[&](HWND w){
   SendMessageW(w,WM_TIMER,7,0);const auto status=locations.observe({job->target()});
   if(!status||!status->regular(job->target())){action=waitForPresent;return;}
   expect(label(edits(w)[0])==job->target().wstring(),"A successful completed-file probe retains the saved destination");
   gate.mode=1;action=waitForMissing;
  };
  std::function<void(HWND)> reopen;
  reopen=[&](HWND w){
   if(!IsWindowVisible(w)){action=reopen;return;}
   expect(!gate.returned&&gate.calls==1,"Properties reopens using the pending file-location request");
   SetWindowTextW(edits(w)[2],L"location probe edit");SetEvent(gate.release.h);action=waitForPresent;
  };
  open(reopen,false,0,&locations);
  expect(job->snapshot()==original,"Location refresh and canceled edits preserve completed metadata");
 }
 auto other=frame.manager.add("https://fixture.example/other","","other.bin");frame.refresh();auto otherAt=std::find(frame.visible.begin(),frame.visible.end(),other);select(frame.table.GetSafeHwnd(),(int)std::distance(frame.visible.begin(),otherAt));expect(frame.selected().size()==1&&frame.selected().front()==other,"Another row is selected before completed-file double-click");
 open([&](HWND w){auto fields=edits(w);expect(label(fields[1])==L"https://fixture.example/download","Completed-file double-click opens the clicked record's Properties");expect(IsWindowEnabled(named(w,L"Open")),"Complete file can be opened");expect(IsWindowEnabled(named(w,L"&Move")),"Complete file exposes Move");expect((GetWindowLongW(fields[0],GWL_STYLE)&ES_READONLY)!=0,"Complete destination is read-only until Move");expect(label(fields[2])==L"saved description","Properties reopens with saved description");
 auto link=dynamic_cast<StaticHyperlink*>(CWnd::FromHandlePermanent(named(w,L"https://fixture.example/source")));auto form=dynamic_cast<Form*>(CWnd::FromHandlePermanent(w));expect(link&&form,"Source-page control is the native Static hyperlink implementation");int activations=0;form->bind(link,[&]{activations++;});link->SetFocus();expect(::GetFocus()==link->GetSafeHwnd(),"Source-page Static link accepts keyboard focus");
 MSG key{};key.hwnd=link->GetSafeHwnd();key.message=WM_KEYDOWN;key.wParam=VK_RETURN;expect(link->PreTranslateMessage(&key)&&activations==1,"Enter activates the source-page link once");key.lParam=1L<<30;expect(link->PreTranslateMessage(&key)&&activations==1,"Held Enter does not repeatedly activate the link");key.message=WM_KEYUP;key.lParam=0;expect(link->PreTranslateMessage(&key)&&activations==1,"Enter release does not activate twice");key.message=WM_KEYDOWN;key.wParam=VK_SPACE;expect(link->PreTranslateMessage(&key)&&activations==2,"Space activates the source-page link");
 link->EnableWindow(FALSE);link->PreTranslateMessage(&key);expect(activations==2,"Disabled source-page link ignores keyboard activation");link->EnableWindow(TRUE);SendMessageW(link->GetSafeHwnd(),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(2,2));SendMessageW(link->GetSafeHwnd(),WM_LBUTTONUP,0,MAKELPARAM(2,2));expect(activations==3,"Native Static mouse notification activates the source-page link once");
 auto accessibility=LoadLibraryExW(L"oleacc.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);expect(accessibility!=nullptr,"Load Windows accessibility provider for Properties");
 using AccessibleObject=HRESULT(WINAPI*)(HWND,DWORD,REFIID,void**);auto accessibleObject=reinterpret_cast<AccessibleObject>(GetProcAddress(accessibility,"AccessibleObjectFromWindow"));expect(accessibleObject!=nullptr,"Windows provides link accessibility inspection");
 CComPtr<IAccessible> accessible;expect(SUCCEEDED(accessibleObject(link->GetSafeHwnd(),static_cast<DWORD>(OBJID_CLIENT),__uuidof(IAccessible),(void**)&accessible)),"Source link exposes accessibility object");
 CComVariant self(CHILDID_SELF),linkRole;expect(SUCCEEDED(accessible->get_accRole(self,&linkRole))&&linkRole.vt==VT_I4&&linkRole.lVal==ROLE_SYSTEM_LINK,"Source page is exposed as an accessible link");
 CComBSTR accessibleName;expect(SUCCEEDED(accessible->get_accName(self,&accessibleName))&&accessibleName&&std::wstring(accessibleName)==L"https://fixture.example/source","Accessible source link retains its URL name");
 CComBSTR defaultAction;expect(SUCCEEDED(accessible->get_accDefaultAction(self,&defaultAction))&&defaultAction&&std::wstring(defaultAction)==L"Open","Accessible source link describes its Open action");
 expect(SUCCEEDED(accessible->accDoDefaultAction(self))&&activations==4,"Accessible default action activates source link once");
 link->EnableWindow(FALSE);CComVariant disabled;expect(SUCCEEDED(accessible->get_accState(self,&disabled))&&disabled.vt==VT_I4&&(disabled.lVal&STATE_SYSTEM_UNAVAILABLE),"Disabled source link reports unavailable state");
 expect(FAILED(accessible->accDoDefaultAction(self))&&activations==4,"Disabled source link rejects accessible activation");link->EnableWindow(TRUE);accessible.Release();FreeLibrary(accessibility);
 const auto reference=Json::parse(R"ref([{"id":1325,"x":7,"y":36,"width":44,"height":9,"title":"Type:","class":130},{"id":1093,"x":58,"y":36,"width":253,"height":9,"title":"Static","class":130},{"id":1299,"x":7,"y":48,"width":43,"height":9,"title":"Status:","class":130},{"id":1098,"x":58,"y":48,"width":253,"height":9,"title":"Static","class":130},{"id":1324,"x":7,"y":60,"width":43,"height":9,"title":"Size:","class":130},{"id":1095,"x":58,"y":60,"width":253,"height":9,"title":"Static","class":130},{"id":1323,"x":7,"y":77,"width":45,"height":9,"title":"Save To:","class":130},{"id":1096,"x":58,"y":74,"width":197,"height":14,"title":"","class":129},{"id":1082,"x":261,"y":74,"width":50,"height":14,"title":"&Browse...","class":128},{"id":1619,"x":7,"y":96,"width":44,"height":9,"title":"Address:","class":130},{"id":1097,"x":58,"y":93,"width":253,"height":14,"title":"","class":129},{"id":1298,"x":7,"y":115,"width":47,"height":9,"title":"Description:","class":130},{"id":1099,"x":58,"y":112,"width":253,"height":14,"title":"","class":129},{"id":1741,"x":7,"y":136,"width":304,"height":9,"title":"The web page from which this file was obtained:","class":130},{"id":1734,"x":7,"y":149,"width":304,"height":9,"title":"Unknown","class":130},{"id":1570,"x":7,"y":171,"width":47,"height":9,"title":"Referer:","class":130},{"id":1569,"x":58,"y":167,"width":253,"height":14,"title":"","class":129},{"id":1322,"x":23,"y":188,"width":50,"height":9,"title":"Login","class":130},{"id":1033,"x":80,"y":185,"width":126,"height":13,"title":"","class":129},{"id":1289,"x":21,"y":205,"width":52,"height":9,"title":"Password","class":130},{"id":1041,"x":80,"y":203,"width":126,"height":13,"title":"","class":129},{"id":1164,"x":172,"y":227,"width":50,"height":14,"title":"Open","class":128},{"id":1092,"x":261,"y":227,"width":50,"height":14,"title":"OK","class":128}])ref");
 for(UINT dpi:{96u,120u,144u,168u,192u}){auto suggested=bounds(w);SendMessageW(w,WM_DPICHANGED,MAKEWPARAM(dpi,dpi),(LPARAM)&suggested);CRect client;GetClientRect(w,&client);for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){if(!shown(child))continue;auto rect=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&rect,2);expect(rect.left>=0&&rect.top>=0&&rect.right<=client.right&&rect.bottom<=client.bottom,"Properties controls fit inside client area at tested DPI");}CFont referenceFont;referenceFont.CreateFontW(-MulDiv(8,dpi,72),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Tahoma");auto dc=GetDC(w);auto units=DialogUnits::measure(dc,(HFONT)referenceFont.GetSafeHandle());ReleaseDC(w,dc);
 LOGFONTW actualFont{};GetObjectW((HFONT)SendMessageW(fields[2],WM_GETFONT,0,0),sizeof(actualFont),&actualFont);expect(actualFont.lfHeight==-MulDiv(8,dpi,72)&&!wcscmp(actualFont.lfFaceName,L"Tahoma"),"Properties field uses the reference point size and font at tested DPI");
 for(const auto& ref:reference){auto target=CRect(units.rect((int)ref["x"],(int)ref["y"],(int)ref["width"],(int)ref["height"]));bool matched=false;
 for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){auto actual=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&actual,2);if(actual!=target)continue;
 wchar_t type[80]{};GetClassNameW(child,type,80);const int role=ref["class"];if((role==130&&_wcsicmp(type,L"Static"))||(role==129&&_wcsicmp(type,L"Edit"))||(role==128&&_wcsicmp(type,L"Button")))continue;
 auto title=str(ref,"title");if(title=="Static"||title.empty()||title=="Unknown"||title=="&Browse..."||label(child)==wide(title))matched=true;
 }if(!matched){Json observed=Json::array();for(auto child=GetWindow(w,GW_CHILD);child;child=GetWindow(child,GW_HWNDNEXT)){auto actual=bounds(child);MapWindowPoints(nullptr,w,(POINT*)&actual,2);wchar_t type[80]{};GetClassNameW(child,type,80);observed.push_back({{"title",utf8(label(child))},{"class",utf8(type)},{"rect",{actual.left,actual.top,actual.right,actual.bottom}}});}atomicText(renderRoot/L"geometry-failure.json",Json{{"dpi",dpi},{"units",{units.x,units.y}},{"reference",ref},{"expected",{target.left,target.top,target.right,target.bottom}},{"controls",observed}}.dump(2),false);}expect(matched,"Properties reference geometry, static labels and control classes match");}
 captureComponent(w,renderRoot/wide("properties-"+std::to_string(dpi)+".png"));}SendMessageW(w,WM_CLOSE,0,0);},true);
 expect(frame.selected().size()==1&&frame.selected().front()==job,"Double-click selects only the clicked completed row");
 frame.KillTimer(1);
 job->data["ScanResult"]={{"Status","Pending"}};
 open([&](HWND w){
  const auto button=named(w,L"Open");
  expect(!IsWindowEnabled(button),"File Properties opens with Open disabled while a scan is pending");
  job->data["ScanResult"]={{"Status","Finished"},{"ExitCode",0}};
  SendMessageW(w,WM_TIMER,7,0);
  expect(IsWindowEnabled(button)!=FALSE,"The same Properties window enables Open when the scan finishes");
  const auto target=job->target(),moved=target.parent_path()/L"scanner-wait-fixture.bin";
  fs::rename(target,moved);
  job->data["ScanResult"]={{"Status","Pending"}};
  expect(IsWindowEnabled(button)!=FALSE,"A posted action can predate the Properties timer refresh");
  SendMessageW(w,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(button),BN_CLICKED),(LPARAM)button);
  expect(IsWindow(w)&&fs::exists(moved),"A stale Open command does not use the saved path during a pending scan");
  SendMessageW(w,WM_TIMER,7,0);
  expect(!IsWindowEnabled(button),"The Properties timer disables Open after a new scan starts");
  fs::rename(moved,target);
  SendMessageW(w,WM_CLOSE,0,0);
 });
 job->data.erase("ScanResult");
 }

};class PropertiesDialogApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);fs::path output;std::unique_ptr<Manager> manager;MainWindow* frame=nullptr;
 try{if(argc!=2)throw std::runtime_error("Pass isolated output directory");output=argv[1];renderRoot=output;fs::create_directories(output);manager=std::make_unique<Manager>(output/L"state");auto p=manager->state["Settings"];p["DownloadFolder"]=utf8((output/L"downloads").wstring());p["CategoryFolders"]=false;p["CloseToTray"]=false;p["ClipboardMonitor"]=false;manager->setSettings(p);frame=new MainWindow(*manager);m_pMainWnd=frame;ToolbarComponentTest::run(*frame);code=0;}catch(const std::exception& e){failure=e.what();}
 if(frame){frame->SendMessage(WM_CLOSE);m_pMainWnd=nullptr;}manager.reset();if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;}
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};PropertiesDialogApplication application;
}
