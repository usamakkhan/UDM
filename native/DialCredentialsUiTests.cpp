// Drives and renders only native windows owned by this isolated test executable.
#define UDM_TOOLBAR_COMPONENT_TEST
#include "App.cpp"
namespace udm {
static Json results=Json::array();static fs::path output;static std::string failure;static std::wstring expectedTitle;static std::function<void(HWND)> action;static ULONGLONG began;
static void expect(bool value,const char* name){results.push_back({{"name",name},{"passed",value}});if(!value)throw std::runtime_error(name);}
static Json mainQueue(Manager& m){for(const auto& q:m.state["Queues"])if(str(q,"Name")=="Main queue")return q;throw std::runtime_error("Missing Main queue fixture");}
static std::wstring windowText(HWND w){wchar_t text[512]{};GetWindowTextW(w,text,512);return text;}
static HWND child(HWND parent,const wchar_t* label){for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(windowText(c)==label&&IsWindowVisible(c))return c;throw std::runtime_error("Missing owned test control: "+utf8(label));}
static std::vector<HWND> children(HWND parent,const wchar_t* type){std::vector<HWND> result;for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){wchar_t name[128]{};GetClassNameW(c,name,128);if(!_wcsicmp(name,type))result.push_back(c);}return result;}
static void press(HWND w,const wchar_t* label){auto c=child(w,label);expect(IsWindowEnabled(c)!=FALSE,"Requested native button is enabled");SendMessageW(c,BM_CLICK,0,0);}
static void selectCombo(HWND parent,HWND control,int index){SendMessageW(control,CB_SETCURSEL,index,0);SendMessageW(parent,WM_COMMAND,MAKEWPARAM(GetDlgCtrlID(control),CBN_SELCHANGE),(LPARAM)control);}
static CRect bounds(HWND w){CRect r;GetWindowRect(w,&r);return r;}
static void capture(HWND w,const wchar_t* name){
 ShowWindow(w,SW_SHOWNOACTIVATE);RedrawWindow(w,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);auto r=bounds(w);CImage img;img.Create(r.Width(),r.Height(),32);auto dc=img.GetDC();RECT fill{0,0,r.Width(),r.Height()};FillRect(dc,&fill,GetSysColorBrush(COLOR_BTNFACE));SendMessageW(w,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);
 for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT)){if(!(GetWindowLongW(c,GWL_STYLE)&WS_VISIBLE))continue;auto cr=bounds(c);auto saved=SaveDC(dc);IntersectClipRect(dc,cr.left-r.left,cr.top-r.top,cr.right-r.left,cr.bottom-r.top);SetViewportOrgEx(dc,cr.left-r.left,cr.top-r.top,nullptr);SendMessageW(c,WM_PRINT,(WPARAM)dc,PRF_NONCLIENT|PRF_CLIENT|PRF_ERASEBKGND|PRF_CHILDREN);RestoreDC(dc,saved);}
 img.ReleaseDC();if(FAILED(img.Save((output/name).c_str())))throw std::runtime_error("Could not render native test dialog");
}
static BOOL CALLBACK observe(HWND w,LPARAM){if(windowText(w)!=expectedTitle)return TRUE;try{ShowWindow(w,SW_SHOWNOACTIVATE);action(w);}catch(const std::exception& e){failure=e.what();PostMessageW(w,WM_CLOSE,0,0);}return FALSE;}
static void CALLBACK onTimer(HWND,UINT,UINT_PTR,DWORD){if(GetTickCount64()-began>12000){failure="Owned dialog test deadline";EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM)->BOOL{PostMessageW(w,WM_CLOSE,0,0);return TRUE;},0);return;}EnumThreadWindows(GetCurrentThreadId(),observe,0);}
static void modal(Form& dialog,const wchar_t* caption,std::function<void(HWND)> callback){expectedTitle=caption;action=std::move(callback);began=GetTickCount64();auto timer=SetTimer(nullptr,0,80,onTimer);dialog.DoModal();KillTimer(nullptr,timer);action={};if(!failure.empty())throw std::runtime_error(failure);}

static HWND editAfter(HWND w,const wchar_t* label){return GetWindow(child(w,label),GW_HWNDNEXT);}
static void dialTab(HWND w){auto tab=children(w,L"SysTabControl32").at(0);TabCtrl_SetCurSel(tab,7);NMHDR notice{tab,(UINT_PTR)GetDlgCtrlID(tab),TCN_SELCHANGE};SendMessageW(w,WM_NOTIFY,notice.idFrom,(LPARAM)&notice);}
static HWND applyButton(HWND w,bool global){std::vector<HWND> found;for(HWND c=GetWindow(w,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))if(IsWindowVisible(c)&&windowText(c)==L"Apply")found.push_back(c);std::sort(found.begin(),found.end(),[](HWND a,HWND b){return bounds(a).top<bounds(b).top;});expect(found.size()==2,"Dial page has separate connection and configuration Apply controls");return found[global?1:0];}
class FixtureCredentials:public DialCredentialStore {
public:
 std::map<std::string,DialCredentialInfo> entries;std::vector<std::pair<DialEntry,DialCredentialChange>> writes;bool fail=false;
 DialCredentialInfo read(const DialEntry& entry)override{return entries.at(entry.phonebook);}
 void write(const DialEntry& entry,const DialCredentialChange& change)override{
  if(fail)throw std::runtime_error("Fixture Windows write failure");validateDialCredentialChange(change);writes.emplace_back(entry,change);
  auto& info=entries.at(entry.phonebook);info.userName=change.userName;
  if(change.passwordEdited){auto value=reveal(change.protectedPassword);info.savedPassword=change.savePassword&&!value.empty();info.sessionPassword=!change.savePassword&&!value.empty();}
  else if(!change.savePassword){info.savedPassword=false;}else if(info.sessionPassword){info.savedPassword=true;info.sessionPassword=false;}
 }
};
class DialCredentialsTestApplication:public CWinApp {
 int code=1;
public:BOOL InitInstance()override{
 CWinApp::InitInstance();INITCOMMONCONTROLSEX common{sizeof(common),ICC_WIN95_CLASSES|ICC_DATE_CLASSES};InitCommonControlsEx(&common);AfxOleInit();int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
 try{
  if(argc!=2)throw std::runtime_error("Pass an isolated output directory");output=fs::path(argv[1]);fs::create_directories(output);Manager manager(output/L"state");
  DialEntry first{"Same VPN",utf8((output/L"first.pbk").wstring())},second{"Same VPN",utf8((output/L"second.pbk").wstring())};
  auto store=std::make_shared<FixtureCredentials>();store->entries[first.phonebook]={"DOMAIN\\alice",true,false};store->entries[second.phonebook]={"bob",false,false};
  auto provider=[=]{return std::vector<DialEntry>{first,second};};
  auto prefs=manager.state["Settings"];prefs["DialEnabled"]=false;prefs["DialEntry"]=first.name;prefs["DialPhonebook"]=first.phonebook;manager.setSettings(prefs);
  {
   auto before=manager.snapshot();Options dialog(manager,nullptr,store,provider);modal(dialog,L"UDM Configuration",[&](HWND w){
    dialTab(w);auto user=editAfter(w,L"User name:"),password=editAfter(w,L"Password:");
    expect(windowText(user)==L"DOMAIN\\alice","Dial page loads the selected Windows identity");
    auto actual=bounds(user);MapWindowPoints(nullptr,w,(LPPOINT)&actual,2);expect(actual==dialog.rect(88,117,117,14),"User-name field uses the reference dialog-unit position and size");
    actual=bounds(password);MapWindowPoints(nullptr,w,(LPPOINT)&actual,2);expect(actual==dialog.rect(88,138,117,14),"Password field uses the reference dialog-unit position and size");
    expect((GetWindowLongW(password,GWL_STYLE)&ES_PASSWORD)!=0&&windowText(password)==L"********","Saved password uses a masked display placeholder");
    expect(SendMessageW(child(w,L"Save password"),BM_GETCHECK,0,0)==BST_CHECKED,"Save password reflects Windows storage state");
    expect(!IsWindowEnabled(applyButton(w,false)),"Connection Apply starts disabled for an unchanged identity");
    SetWindowTextW(user,L"unsaved-user");SetWindowTextW(password,L"not-committed");expect(IsWindowEnabled(applyButton(w,false)),"Editing a credential enables connection Apply");capture(w,L"dial-draft.png");press(w,L"Cancel");
   });expect(store->writes.empty()&&manager.snapshot()==before,"Cancel discards credential drafts and preserves configuration");
  }
  {
   Options dialog(manager,nullptr,store,provider);modal(dialog,L"UDM Configuration",[&](HWND w){
    dialTab(w);auto user=editAfter(w,L"User name:"),password=editAfter(w,L"Password:");auto combo=GetWindow(child(w,L"Connection:"),GW_HWNDNEXT);
    expect(SendMessageW(combo,CB_GETCOUNT,0,0)==2,"Same-name Windows connections are both listed");
    wchar_t label[1024]{};SendMessageW(combo,CB_GETLBTEXT,0,(LPARAM)label);expect(std::wstring(label).find(L"first.pbk")!=std::wstring::npos,"Duplicate connection names display their phone-book identity");
    SetWindowTextW(user,L"DOMAIN\\first-draft");selectCombo(w,combo,1);expect(windowText(user)==L"bob"&&windowText(password).empty(),"Switching connections never carries another connection's credentials");
    SetWindowTextW(user,L"second-draft");selectCombo(w,combo,0);expect(windowText(user)==L"DOMAIN\\first-draft","Switching back restores the unapplied connection draft");
    expect(store->writes.empty(),"Connection selection does not save credentials");
    SendMessageW(applyButton(w,true),BM_CLICK,0,0);
    expect(store->writes.size()==2&&store->entries[first.phonebook].userName=="DOMAIN\\first-draft"&&store->entries[second.phonebook].userName=="second-draft","Configuration Apply commits each changed connection to its own phone book");
    expect(!store->writes[0].second.passwordEdited&&!store->writes[1].second.passwordEdited,"Unchanged password placeholders are never submitted as passwords");
    expect(!IsWindowEnabled(applyButton(w,false)),"Successful Apply clears the selected credential draft");
    capture(w,L"dial-applied.png");press(w,L"Cancel");
   });
  }
  {
   auto before=store->writes.size();Options dialog(manager,nullptr,store,provider);modal(dialog,L"UDM Configuration",[&](HWND w){
    dialTab(w);auto password=editAfter(w,L"Password:");SetWindowTextW(password,L"session-fixture");press(w,L"Save password");SendMessageW(applyButton(w,false),BM_CLICK,0,0);
    expect(store->writes.size()==before+1&&!store->writes.back().second.savePassword&&store->writes.back().second.passwordEdited,"Connection Apply accepts an explicitly unsaved password");
    expect(store->writes.back().second.protectedPassword!="session-fixture"&&reveal(store->writes.back().second.protectedPassword)=="session-fixture","Credential draft passes only Windows-protected password data to its backend");
    expect(!store->entries[first.phonebook].savedPassword&&store->entries[first.phonebook].sessionPassword,"Session-only mode is reflected after Apply");
    expect(windowText(password)==L"********","Applied session password is replaced by its masked placeholder");
    capture(w,L"dial-session.png");press(w,L"Cancel");
   });expect(store->writes.size()==before+1,"Cancel does not repeat or undo an explicitly applied credential update");
   expect(manager.snapshot().dump().find("session-fixture")==std::string::npos,"Dial password is absent from UDM configuration and history");
  }
  {
   Options dialog(manager,nullptr,store,[]{return std::vector<DialEntry>{};});modal(dialog,L"UDM Configuration",[&](HWND w){
    dialTab(w);expect(!IsWindowEnabled(editAfter(w,L"User name:"))&&!IsWindowEnabled(editAfter(w,L"Password:"))&&!IsWindowEnabled(applyButton(w,false)),"Empty Windows connection list disables credential editing");
    capture(w,L"dial-empty.png");press(w,L"Cancel");
   });
  }
  {
   Form form("Credential failure fixture",289,325);form.dialogUnits=true;std::shared_ptr<DialCredentialFields> fields;
   form.init=[&]{auto status=form.label("",21,249,241,26);fields=std::make_shared<DialCredentialFields>(form,8,35,status,store);fields->load(first);form.button("Cancel",172,306,50,[&]{form.close(IDCANCEL);});};
   modal(form,L"Credential failure fixture",[&](HWND w){
    SetWindowTextW(editAfter(w,L"User name:"),L"retry-user");auto before=store->writes.size();store->fail=true;bool rejected=false;try{fields->applyAll();}catch(...){rejected=true;}store->fail=false;
    expect(rejected&&store->writes.size()==before,"Failed credential persistence leaves the draft unapplied");
    fields->applyAll();expect(store->writes.size()==before+1&&store->entries[first.phonebook].userName=="retry-user","Credential draft can be retried after a Windows write failure");
    press(w,L"Cancel");
   });
  }
  code=0;
 }catch(const std::exception& e){failure=e.what();}
 if(argv)LocalFree(argv);if(!output.empty())atomicText(output/L"results.json",Json{{"passed",code==0},{"error",failure},{"checks",results}}.dump(2),false);return FALSE;
 }
 int ExitInstance()override{AfxOleTerm(FALSE);return code;}
};
DialCredentialsTestApplication application;
}
