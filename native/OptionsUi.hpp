// Included inside udm after the Options model and supporting dialogs.
class Options:public Form {
 Manager& manager;Json prefs,original;ThemeTabs* tabs=nullptr;std::vector<std::vector<CWnd*>> pages;std::vector<std::function<void()>> bindings,refreshBindings;
 ThemeList *browsers=nullptr,*servers=nullptr,*logins=nullptr,*sounds=nullptr;std::vector<std::string> browserKeys;std::vector<HICON> icons;
 std::function<void()> saveCategory,reloadCategory;static constexpr int ox=8,oy=35;
 CWnd* L(std::string value,int x,int y,int w,int h=9){return label(value,ox+x,oy+y,w,h);}
 CWnd* E(std::string value,int x,int y,int w,int h=14,bool secret=false){return edit(value,ox+x,oy+y,w,h,false,false,secret);}
 CWnd* B(std::string value,int x,int y,int w,std::function<void()> action){return button(value,ox+x,oy+y,w,std::move(action));}
 CWnd* C(std::string value,bool on,int x,int y,int w,int h=10){return check(value,on,ox+x,oy+y,w,h);}
 CWnd* G(std::string value,int x,int y,int w,int h){return control(L"BUTTON",value,BS_GROUPBOX,ox+x,oy+y,w,h);}
 void rule(int x,int y,int w){control(L"STATIC","",SS_ETCHEDHORZ,ox+x,oy+y,w,1);}
 CComboBox* Q(const std::vector<std::string>& values,const std::string& current,int x,int y,int w,bool free=false){return combo(values,current,ox+x,oy+y,w,free);}
 ThemeList* table(int x,int y,int w,int h,const std::vector<std::pair<std::string,int>>& columns,bool checks=false){
  auto list=make<ThemeList>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,ox+x,oy+y,w,h);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|(checks?LVS_EX_CHECKBOXES:0));list->theme();int i=0;for(auto& column:columns)list->InsertColumn(i++,cs(column.first),LVCFMT_LEFT,pixelsX(column.second));return list;
 }
 void heading(const std::string& caption,const wchar_t* icon,int titleX=48,int lineX=35,int lineWidth=225){
  auto image=control(L"STATIC","",SS_ICON,ox+7,oy+4,20,20);auto value=LoadIconW(nullptr,icon);if(value)image->SendMessage(STM_SETICON,(WPARAM)value);L(caption,titleX,7,260-titleX,11);rule(lineX,19,lineWidth);
 }
 void bindFlag(CWnd* control,const char* key,bool invert=false){
  auto previous=std::make_shared<bool>(checked(control));bindings.push_back([this,control,key,invert,previous]{bool value=checked(control);if(value!=*previous){prefs[key]=invert?!value:value;*previous=value;}});
  const bool fallback=invert?!*previous:*previous;refreshBindings.push_back([this,control,key,invert,previous,fallback]{bool value=yes(prefs,key,fallback);value=invert?!value:value;control->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);*previous=value;});
 }
 void bindText(CWnd* control,const char* key,bool number=false,bool secret=false){
  auto previous=std::make_shared<std::string>(text(control));bindings.push_back([this,control,key,number,secret,previous]{auto value=text(control);if(value!=*previous){prefs[key]=number?Json(std::stoll(value)):Json(secret?protect(value):value);*previous=value;}});
  refreshBindings.push_back([this,control,key,number,secret,previous]{auto value=number?std::to_string(num(prefs,key)):secret?reveal(str(prefs,key)):str(prefs,key);if(auto combo=dynamic_cast<CComboBox*>(control)){int index=combo->FindStringExact(-1,cs(value));if(index>=0)combo->SetCurSel(index);else combo->SetWindowText(cs(value));}else control->SetWindowText(cs(value));*previous=text(control);});
 }
 void reload(){for(auto& action:refreshBindings)action();if(reloadCategory)reloadCategory();}
 void collect(){for(auto& apply:bindings)apply();if(saveCategory)saveCategory();}
 void applyOptions(){
  collect();Lock lock(manager.mutex);auto old=manager.state["Settings"],next=mergeOptionsDraft(original,prefs,old);bool started=startupEnabled();manager.setSettings(next);
  try{if(started!=yes(next,"RunAtLogin"))setStartup(yes(next,"RunAtLogin"));}catch(...){manager.setSettings(old);throw;}original=next;prefs=next;reload();
 }
 void page(){const int current=tabs->GetCurSel();for(size_t i=0;i<pages.size();++i)for(auto item:pages[i])item->ShowWindow((int)i==current?SW_SHOW:SW_HIDE);}
 void addressExceptionsDialog(){AddressExceptionsDialog dialog(this,prefs);dialog.DoModal();}
 void refreshBrowsers(){
  auto rows=prefs.value("BrowserCaptureTargets",Json::object());for(auto entry:std::vector<std::pair<std::string,std::string>>{{"chrome.exe","Google Chrome"},{"msedge.exe","Microsoft Edge"},{"firefox.exe","Mozilla Firefox"},{"opera.exe","Opera"},{"brave.exe","Brave"},{"vivaldi.exe","Vivaldi"},{"chromium.exe","Chromium"}})if(!rows.contains(entry.first))rows[entry.first]={{"Name",entry.second},{"Enabled",true}};prefs["BrowserCaptureTargets"]=rows;
  browsers->DeleteAllItems();browserKeys.clear();int row=0;for(auto it=rows.begin();it!=rows.end();++it){browserKeys.push_back(it.key());browsers->InsertItem(row,cs(str(*it,"Name")));browsers->SetCheck(row++,yes(*it,"Enabled",true));}
 }
 void saveBrowsers(){auto rows=prefs.value("BrowserCaptureTargets",Json::object());for(int i=0;i<(int)browserKeys.size();++i)rows[browserKeys[i]]["Enabled"]=browsers->GetCheck(i)!=FALSE;prefs["BrowserCaptureTargets"]=rows;}
 void browserEditor(int page){collect();if(page==0)browserKeysDialog(this,prefs);else if(page==1)browserMenusDialog(this,prefs);else browserPanelsDialog(this,prefs);reload();}
 void generalPage(){
  heading("Browser/System Integration",IDI_APPLICATION,45,35,229);
  L("Extension and native host integration",10,28,190);B("Restart",205,25,50,[this]{applyOptions();restartBrowserHosts();});
  auto startup=C("Launch UDM on startup",yes(prefs,"RunAtLogin"),13,46,241);bindFlag(startup,"RunAtLogin");
  auto clipboard=C("Automatically download URLs placed on clipboard",yes(prefs,"ClipboardMonitor"),13,58,241);bindFlag(clipboard,"ClipboardMonitor");
  auto capture=C("Enable browser download capture",yes(prefs,"BrowserCaptureEnabled",true),13,70,242);bindFlag(capture,"BrowserCaptureEnabled");
  G("Capture downloads from the following browsers:",4,85,261,131);browsers=table(13,96,241,75,{{"Browser",223}},true);refreshBrowsers();bindings.push_back([this]{saveBrowsers();});
  B("Add browser...",183,174,72,[this]{saveBrowsers();CFileDialog picker(TRUE,L"exe",nullptr,OFN_FILEMUSTEXIST,L"Browser programs|*.exe||",this);if(picker.DoModal()!=IDOK)return;fs::path path((LPCWSTR)picker.GetPathName());auto key=lower(utf8(path.filename().wstring()));auto title=utf8(path.stem().wstring());auto rows=prefs.value("BrowserCaptureTargets",Json::object());rows[key]={{"Name",title},{"Enabled",true}};auto next=prefs;next["BrowserCaptureTargets"]=rows;validateBrowserSettings(next);prefs=next;refreshBrowsers();});
  rule(16,192,238);L("Customize keys to prevent or force downloading",10,200,190);B("Keys...",205,197,50,[this]{browserEditor(0);});
  L("Customize UDM browser menus",10,224,190);B("Edit...",205,221,50,[this]{browserEditor(1);});
  L("Customize UDM Download panels in browsers",13,243,188);B("...",205,240,50,[this]{browserEditor(2);});
 }
 void typesPage(){
  heading("Downloaded file types",IDI_INFORMATION,45,35,229);
  L("Automatically start downloading the following file types:",7,27,237,10);auto types=edit(str(prefs,"CaptureExtensions"),ox+7,oy+39,254,36,false,true);bindText(types,"CaptureExtensions");
  B("Default",203,78,50,[types]{types->SetWindowText(cs(str(defaultSettings(),"CaptureExtensions")));});
  L("Don't start downloading automatically from these sites:",7,100,235,10);auto excluded=edit(str(prefs,"CaptureExcludedHosts"),ox+7,oy+112,254,30,false,true);bindText(excluded,"CaptureExcludedHosts");
  L("(separate names by spaces)",7,144,189,10);B("Default",203,145,50,[excluded]{excluded->SetWindowText(L"");});
  L("Don't automatically download from these addresses:",7,164,254,10);B("Edit list...",7,176,91,[this]{addressExceptionsDialog();});
 }
 void refreshServers(){servers->DeleteAllItems();int i=0;for(auto& row:prefs["ServerConnections"]){servers->InsertItem(i,cs(str(row,"Host")));servers->SetItemText(i++,1,cs(std::to_string(num(row,"Connections"))));}}
 void serverRule(bool edit){
  int index=edit?servers->GetNextItem(-1,LVNI_SELECTED):-1;if(edit&&index<0)return;auto rows=prefs["ServerConnections"];Json row=index<0?Json{{"Host",""},{"Connections",8}}:rows[index];
  Form d("Max. connections number for a server",221,89,this);d.dialogUnits=true;d.init=[&]{
   d.label("Server",50,3,150);auto scope=d.combo({"Host"},"Host",7,15,40);scope->EnableWindow(FALSE);auto host=d.edit(str(row,"Host"),50,15,164,13);d.label("Applies to this host and its subdomains.",50,29,164);
   d.control(L"STATIC","Max. connections number",SS_RIGHT,33,49,128,9);auto count=d.combo({"1","2","4","8","16","24","32"},std::to_string(num(row,"Connections",8)),166,47,48,true);
   d.accept=[&,host,count]{auto name=lower(trim(text(host)));auto n=std::stoll(text(count));if(name.empty()||Url("https://"+name+"/").host!=name||n<1||n>32)throw std::runtime_error("Enter a host and 1 to 32 connections.");for(int i=0;i<(int)rows.size();++i)if(i!=index&&str(rows[i],"Host")==name)throw std::runtime_error("This server already has a rule.");Json next={{"Host",name},{"Connections",n}};if(index<0){if(rows.size()>=100)throw std::runtime_error("Use at most 100 server rules.");rows.push_back(next);}else rows[index]=next;prefs["ServerConnections"]=rows;d.close();};
   d.defaultButton(d.button("OK",53,68,50,d.accept));d.button("Cancel",119,68,50,[&]{d.close(IDCANCEL);});
  };d.DoModal();refreshServers();
 }
 void connectionPage(){
  heading("Connections and Limits",IDI_INFORMATION,47);
  G("Max. connections number",7,30,253,124);L("Default max. conn. number",20,42,148,10);auto count=Q({"1","2","4","8","16","24","32"},std::to_string(num(prefs,"Connections",8)),172,40,67,true);bindText(count,"Connections",true);
  rule(13,58,240);L("Exceptions:",15,61,239,10);servers=table(13,71,187,75,{{"Server",137},{"Max.",30}});refreshServers();
  B("New",205,77,50,[this]{serverRule(false);});B("Delete",205,95,50,[this]{auto row=servers->GetNextItem(-1,LVNI_SELECTED);if(row>=0){prefs["ServerConnections"].erase(row);refreshServers();}});B("Edit",205,113,50,[this]{serverRule(true);});
  G("",7,161,253,55);auto enabled=C("Download limits",num(prefs,"QuotaMb")>0,18,161,64);L("Download no more than",14,177,93,10);auto amount=E(std::to_string(std::max<i64>(1,num(prefs,"QuotaMb"))),112,174,24,12);L("MBytes",141,177,72,10);L("every",55,189,52,10);auto hours=E(std::to_string(num(prefs,"QuotaHours",1)),112,187,24,12);L("hours",143,189,69,10);
  auto warn=C("Show warning before stopping downloads",yes(prefs,"WarnQuota",true),18,202,209);bindFlag(warn,"WarnQuota");
  auto active=[this,enabled,amount,hours,warn]{amount->EnableWindow(checked(enabled));hours->EnableWindow(checked(enabled));warn->EnableWindow(checked(enabled));};bind(enabled,active);active();bindings.push_back([this,enabled,amount,hours]{prefs["QuotaMb"]=checked(enabled)?std::stoll(text(amount)):0;prefs["QuotaHours"]=std::stoll(text(hours));});
  auto tls=C("Use TLS 1.3 when supported by Windows",yes(prefs,"UseTls13",true),18,224,237);bindFlag(tls,"UseTls13");
 }
 void savePage();
 void downloadsPage();
 void proxyPage();
 void refreshLogins(){logins->DeleteAllItems();int i=0;for(auto login:prefs["SiteLogins"]){logins->InsertItem(i,cs(siteLoginAddress(login)));logins->SetItemText(i++,1,cs(str(login,"UserName")));}}
 void login(bool editing=false){
  int row=editing?logins->GetNextItem(-1,LVNI_SELECTED):-1;if(editing&&row<0)return;Json existing=row<0?Json::object():prefs["SiteLogins"][row];Form d("Site login",300,94,this);d.dialogUnits=true;d.init=[&]{
   auto scheme=d.combo({"https://"},"https://",7,15,46);scheme->EnableWindow(FALSE);d.label("Server/path",57,5,219);auto address=existing.empty()?"":siteLoginAddress(existing).substr(8);auto site=d.edit(address,56,15,237,12);d.label("Enter a path when different folders use different logins.",57,29,236,19);
   d.label("User",34,56,75,11);auto user=d.edit(str(existing,"UserName"),116,53,82,12);d.label("Password",34,72,75,11);auto password=d.edit(reveal(str(existing,"ProtectedPassword")),116,70,82,12,false,false,true);
   d.accept=[&,site,user,password]{auto value=makeSiteLogin("https://"+trim(text(site)),text(user),text(password));auto list=prefs["SiteLogins"];list.erase(std::remove_if(list.begin(),list.end(),[&](const Json& item){return siteLoginAddress(item)==siteLoginAddress(value)||(!existing.empty()&&siteLoginAddress(item)==siteLoginAddress(existing));}),list.end());list.push_back(value);auto next=prefs;next["SiteLogins"]=list;validateSiteLogins(next);prefs=next;d.close();};
   d.defaultButton(d.button("OK",243,52,50,d.accept));d.button("Cancel",243,73,50,[&]{d.close(IDCANCEL);});
  };d.DoModal();refreshLogins();
 }
 void loginsPage(){heading("User names and passwords for servers/sites",IDI_INFORMATION,42);logins=table(7,28,255,172,{{"HTTPS site / folder",170},{"User name",65}});B("New",13,205,50,[this]{login();});B("Edit",76,205,50,[this]{login(true);});B("Remove",139,205,50,[this]{auto row=logins->GetNextItem(-1,LVNI_SELECTED);if(row>=0){prefs["SiteLogins"].erase(row);refreshLogins();}});refreshLogins();}
 void soundsPage();
 void dialPage();
 void advancedOptions();
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{
  auto event=(NMHDR*)l;if(tabs&&event->hwndFrom==tabs->m_hWnd&&event->code==TCN_SELCHANGE){page();*result=0;return TRUE;}
  try{if(event->code==NM_DBLCLK){if(logins&&event->hwndFrom==logins->m_hWnd){login(true);*result=0;return TRUE;}if(servers&&event->hwndFrom==servers->m_hWnd){serverRule(true);*result=0;return TRUE;}}}catch(const std::exception& e){error(this,e);*result=0;return TRUE;}
  return Form::OnNotify(w,l,result);
 }
public:
 Options(Manager& m,CWnd* parent):Form("UDM Configuration",289,325,parent),manager(m){
  dialogUnits=true;{Lock lock(m.mutex);prefs=m.state["Settings"];}prefs["RunAtLogin"]=startupEnabled();original=prefs;
  init=[this]{
   tabs=make<ThemeTabs>(WS_TABSTOP|TCS_MULTILINE,5,5,279,294);const wchar_t* names[]={L"General",L"File types",L"Connection",L"Save to",L"Downloads",L"Proxy / Socks",L"Sites Logins",L"Dial Up / VPN",L"Sounds"};
   for(int pageId=0;pageId<9;++pageId){tabs->InsertItem(pageId,names[pageId]);const auto before=controls.size();switch(pageId){case 0:generalPage();break;case 1:typesPage();break;case 2:connectionPage();break;case 3:savePage();break;case 4:downloadsPage();break;case 5:proxyPage();break;case 6:loginsPage();break;case 7:dialPage();break;case 8:soundsPage();break;}std::vector<CWnd*> items;for(size_t i=before;i<controls.size();++i)items.push_back(controls[i].get());pages.push_back(std::move(items));}
   accept=[this]{applyOptions();close();};button("More...",8,306,50,[this]{advancedOptions();});defaultButton(button("OK",116,306,50,accept));button("Cancel",172,306,50,[this]{close(IDCANCEL);});button("Apply",228,306,50,[this]{applyOptions();});page();
  };
 }
};
