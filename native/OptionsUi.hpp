// Included inside udm after the Options model and supporting dialogs.
class Options:public Form {
 Manager& manager;Json prefs,original;ThemeTabs* tabs=nullptr;std::vector<std::vector<CWnd*>> pages;std::vector<std::function<void()>> bindings,refreshBindings;
 std::shared_ptr<DialCredentialStore> dialStore;std::function<std::vector<DialEntry>()> dialEntriesProvider;std::shared_ptr<DialCredentialFields> dialCredentials;
 ThemeList *browsers=nullptr,*servers=nullptr,*logins=nullptr,*sounds=nullptr;std::vector<std::string> browserKeys;std::vector<HICON> icons;
 CWnd *serverEdit=nullptr,*serverDelete=nullptr,*loginEdit=nullptr,*loginRemove=nullptr;
 std::function<void()> saveCategory,reloadCategory;static constexpr int ox=7,oy=27;
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
 // IDM page header: 40x40 icon, heading text ending at the page's right edge, etched rule beneath (lineWidth 0 draws no rule).
 void heading(const std::string& caption,const wchar_t* icon,int titleX=48,int lineX=40,int lineWidth=256,int titleWidth=0){
  auto image=control(L"STATIC","",SS_ICON,ox+8,oy+4,23,20);auto value=LoadIconW(nullptr,icon);if(value)image->SendMessage(STM_SETICON,(WPARAM)value);L(caption,titleX,7,titleWidth?titleWidth:293-titleX,11);if(lineWidth>0)rule(lineX,19,lineWidth);
 }
 void bindFlag(CWnd* control,const char* key,bool invert=false){
  auto previous=std::make_shared<bool>(checked(control));bindings.push_back([this,control,key,invert,previous]{bool value=checked(control);if(value!=*previous){prefs[key]=invert?!value:value;*previous=value;}});
  const bool fallback=invert?!*previous:*previous;refreshBindings.push_back([this,control,key,invert,previous,fallback]{bool value=yes(prefs,key,fallback);value=invert?!value:value;control->SendMessage(BM_SETCHECK,value?BST_CHECKED:BST_UNCHECKED);*previous=value;});
 }
 void bindText(CWnd* control,const char* key,bool number=false,bool secret=false,std::function<i64(const std::string&)> parseNumber={}){
  auto previous=std::make_shared<std::string>(text(control));bindings.push_back([this,control,key,number,secret,previous,parseNumber]{auto value=text(control);if(value!=*previous){prefs[key]=number?Json(parseNumber?parseNumber(value):std::stoll(value)):Json(secret?protect(value):value);*previous=value;}});
  refreshBindings.push_back([this,control,key,number,secret,previous]{auto value=number?std::to_string(num(prefs,key)):secret?reveal(str(prefs,key)):str(prefs,key);if(auto combo=dynamic_cast<CComboBox*>(control)){int index=combo->FindStringExact(-1,cs(value));if(index>=0)combo->SetCurSel(index);else combo->SetWindowText(cs(value));}else control->SetWindowText(cs(value));*previous=text(control);});
 }
 void reload(){for(auto& action:refreshBindings)action();if(reloadCategory)reloadCategory();}
 void collect(){for(auto& apply:bindings)apply();if(saveCategory)saveCategory();}
 void applyOptions(){
  collect();if(dialCredentials)dialCredentials->validate();Lock lock(manager.mutex);auto old=manager.state["Settings"],next=mergeOptionsDraft(original,prefs,old);bool started=startupEnabled();manager.setSettings(next);
  try{if(started!=yes(next,"RunAtLogin"))setStartup(yes(next,"RunAtLogin"));if(dialCredentials)dialCredentials->applyAll();}catch(...){manager.setSettings(old);if(startupEnabled()!=started)setStartup(started);throw;}original=next;prefs=next;reload();
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
  heading("Browser/System Integration",IDI_APPLICATION,51,40,261);
  L("Extension and native host integration",11,28,216);B("Restart",233,25,57,[this]{applyOptions();restartBrowserHosts();});
  auto startup=C("Launch UDM on startup",yes(prefs,"RunAtLogin"),15,46,274);bindFlag(startup,"RunAtLogin");
  auto clipboard=C("Automatically download URLs placed on clipboard",yes(prefs,"ClipboardMonitor"),15,58,274);bindFlag(clipboard,"ClipboardMonitor");
  auto capture=C("Enable browser download capture",yes(prefs,"BrowserCaptureEnabled",true),15,70,274);bindFlag(capture,"BrowserCaptureEnabled");
  G("Capture downloads from the following browsers:",5,85,297,131);browsers=table(15,96,274,75,{{"Browser",256}},true);refreshBrowsers();bindings.push_back([this]{saveBrowsers();});
  B("Add browser...",208,174,82,[this]{saveBrowsers();CFileDialog picker(TRUE,L"exe",nullptr,OFN_FILEMUSTEXIST,L"Browser programs|*.exe||",this);if(picker.DoModal()!=IDOK)return;fs::path path((LPCWSTR)picker.GetPathName());auto key=lower(utf8(path.filename().wstring()));auto title=utf8(path.stem().wstring());auto rows=prefs.value("BrowserCaptureTargets",Json::object());rows[key]={{"Name",title},{"Enabled",true}};auto next=prefs;next["BrowserCaptureTargets"]=rows;validateBrowserSettings(next);prefs=next;refreshBrowsers();});
  rule(18,192,271);L("Customize keys to prevent or force downloading",11,200,216);B("Keys...",233,197,57,[this]{browserEditor(0);});
  L("Customize UDM browser menus",11,224,216);B("Edit...",233,221,57,[this]{browserEditor(1);});
  L("Customize UDM Download panels in browsers",15,243,214);B("Edit...",233,240,57,[this]{browserEditor(2);});
 }
 void typesPage(){
  heading("Downloaded file types",IDI_INFORMATION,51,40,261);
  L("Automatically start downloading the following file types:",8,27,270,10);auto types=edit(str(prefs,"CaptureExtensions"),ox+8,oy+39,289,36,false,true);bindText(types,"CaptureExtensions");
  B("Default",231,78,57,[types]{types->SetWindowText(cs(str(defaultSettings(),"CaptureExtensions")));});
  L("Don't start downloading automatically from the following sites:",8,100,268,10);auto excluded=edit(str(prefs,"CaptureExcludedHosts"),ox+8,oy+112,289,30,false,true);bindText(excluded,"CaptureExcludedHosts");
  L("(separate names by spaces)",8,144,215,10);B("Default",231,145,57,[excluded]{excluded->SetWindowText(L"");});
  L("Don't start downloading automatically from the following addresses:",8,164,289,10);B("Edit list ...",8,176,104,[this]{addressExceptionsDialog();});B("Site rules...",120,176,104,[this]{CaptureRulesDialog dialog(this,prefs);dialog.DoModal();});
  rule(8,196,289);auto webPage=C("Stop downloads that receive a web page instead of the file",yes(prefs,"StopOnWebPage",true),10,202,287);bindFlag(webPage,"StopOnWebPage");
  auto extensions=C("Add a missing file extension from the downloaded content",yes(prefs,"CorrectFileExtensions",true),10,217,287);bindFlag(extensions,"CorrectFileExtensions");
 }
 void updateServerActions(){bool selected=servers&&servers->GetNextItem(-1,LVNI_SELECTED)>=0;if(serverEdit)serverEdit->EnableWindow(selected);if(serverDelete)serverDelete->EnableWindow(selected);}
 void refreshServers(){servers->DeleteAllItems();int i=0;for(auto& row:prefs["ServerConnections"]){servers->InsertItem(i,cs(connectionRuleLabel(row)));servers->SetItemText(i++,1,cs(std::to_string(num(row,"Connections"))));}updateServerActions();}
 void serverRule(bool edit){
  int index=edit?servers->GetNextItem(-1,LVNI_SELECTED):-1;if(edit&&index<0)return;auto rows=prefs["ServerConnections"];Json row=index<0?Json{{"Host",""},{"Connections",8}}:rows[index];
  Form d("Max. connections number for a server",221,89,this);d.dialogUnits=true;d.init=[&]{
   d.label("Server",50,3,150);auto scope=d.combo({"All","http","https","ftp"},str(row,"Scheme").empty()?"All":str(row,"Scheme"),7,15,40);auto host=d.edit(str(row,"Host"),50,15,164,13);d.label("Use * as a server wildcard.",50,29,164);
   d.control(L"STATIC","Max. connections number",SS_RIGHT,33,49,128,9);auto count=d.combo({"1","2","4","8","16","24","32"},std::to_string(num(row,"Connections",8)),166,47,48,true);
   d.accept=[&,host,count,scope]{auto name=lower(trim(text(host)));auto n=parseConnectionCount(text(count));auto scheme=text(scope)=="All"?"":text(scope);Json next={{"Host",name},{"Scheme",scheme},{"Connections",n}};validateConnectionRule(next);for(int i=0;i<(int)rows.size();++i)if(i!=index&&lower(str(rows[i],"Host"))==name&&str(rows[i],"Scheme")==scheme)throw std::runtime_error("This server already has a rule.");if(index<0){if(rows.size()>=100)throw std::runtime_error("Use at most 100 server rules.");rows.push_back(next);}else rows[index]=next;prefs["ServerConnections"]=rows;d.close();};
   d.defaultButton(d.button("OK",53,68,50,d.accept));d.button("Cancel",119,68,50,[&]{d.close(IDCANCEL);});
  };d.DoModal();refreshServers();
 }
 void connectionPage(){
  heading("Connections and Limits",IDI_INFORMATION,53,40,256);
  G("Max. connections number",8,30,288,124);L("Default max. conn. number",23,42,168,10);auto count=Q({"1","2","4","8","16","24","32"},std::to_string(num(prefs,"Connections",8)),196,40,76,true);bindText(count,"Connections",true,false,parseConnectionCount);
  rule(15,58,273);L("Exceptions:",17,61,272,10);servers=table(15,71,213,75,{{"Server",163},{"Number",40}});refreshServers();refreshBindings.push_back([this]{refreshServers();});
  B("New",233,77,57,[this]{serverRule(false);});serverDelete=B("Delete",233,95,57,[this]{auto row=servers->GetNextItem(-1,LVNI_SELECTED);if(row>=0){prefs["ServerConnections"].erase(row);refreshServers();}});serverEdit=B("Edit",233,113,57,[this]{serverRule(true);});updateServerActions();
  G("",8,161,288,55);auto enabled=C("Download limits",num(prefs,"QuotaMb")>0,20,161,73);L("Download no more than",16,177,106,10);auto amount=E(std::to_string(std::max<i64>(1,num(prefs,"QuotaMb")>0?num(prefs,"QuotaMb"):num(prefs,"QuotaLastMb",1))),127,174,27,12);L("MBytes",160,177,82,10);L("every",63,189,59,10);auto hours=E(std::to_string(num(prefs,"QuotaHours",1)),127,187,27,12);L("hours",163,189,79,10);
  auto warn=C("Show warning before stopping downloads",yes(prefs,"WarnQuota",true),20,202,238);bindFlag(warn,"WarnQuota");
  auto active=[this,enabled,amount,hours,warn]{amount->EnableWindow(checked(enabled));hours->EnableWindow(checked(enabled));warn->EnableWindow(checked(enabled));};bind(enabled,active);active();bindings.push_back([this,enabled,amount,hours]{collectQuotaDraft(prefs,checked(enabled),text(amount),text(hours));});
  refreshBindings.push_back([this,enabled,amount,hours,active]{enabled->SendMessage(BM_SETCHECK,num(prefs,"QuotaMb")>0?BST_CHECKED:BST_UNCHECKED);amount->SetWindowText(cs(std::to_string(std::max<i64>(1,num(prefs,"QuotaMb")>0?num(prefs,"QuotaMb"):num(prefs,"QuotaLastMb",1)))));hours->SetWindowText(cs(std::to_string(num(prefs,"QuotaHours",1))));active();});
  auto tls=C("Use TLS 1.3 when supported by Windows",yes(prefs,"UseTls13",true),20,224,270);bindFlag(tls,"UseTls13");
  // UDM-only settings (parallel downloads, retries, global limit, clipboard) stay reachable from here now that the frame has only OK and Help.
  L("Additional UDM download and clipboard settings",16,243,214,10);B("More...",233,240,57,[this]{advancedOptions();});
 }
 void savePage();
 void downloadsPage();
 void proxyPage();
 void updateLoginActions(){bool selected=logins&&logins->GetNextItem(-1,LVNI_SELECTED)>=0;if(loginEdit)loginEdit->EnableWindow(selected);if(loginRemove)loginRemove->EnableWindow(selected);}
 void refreshLogins(){logins->DeleteAllItems();int i=0;for(const auto& login:prefs["SiteLogins"]){logins->InsertItem(i,cs(siteLoginAddress(login)));logins->SetItemText(i,1,cs(str(login,"UserName")));logins->SetItemText(i++,2,L"********");}updateLoginActions();}
 void login(bool editing=false){
  int row=editing?logins->GetNextItem(-1,LVNI_SELECTED):-1;if(editing&&row<0)return;Json existing=row<0?Json::object():prefs["SiteLogins"][row];Form d("Site login",300,94,this);d.dialogUnits=true;d.init=[&]{
   auto scheme=d.combo({"https://","http://","ftp://"},existing.empty()?"https://":Url(siteLoginAddress(existing)).scheme+"://",7,15,46);d.label("Server/path",57,5,219);auto address=existing.empty()?"":siteLoginAddress(existing).substr(Url(siteLoginAddress(existing)).scheme.size()+3);auto site=d.edit(address,56,15,237,12);d.label("Enter a path when different folders use different logins.",57,29,236,19);
   d.label("User",34,56,75,11);auto user=d.edit(str(existing,"UserName"),116,53,82,12);d.label("Password",34,72,75,11);auto password=d.edit(reveal(str(existing,"ProtectedPassword")),116,70,82,12,false,false,true);
   d.accept=[&,site,user,password,scheme]{auto value=makeSiteLogin(text(scheme)+trim(text(site)),text(user),text(password));auto list=prefs["SiteLogins"];list.erase(std::remove_if(list.begin(),list.end(),[&](const Json& item){return siteLoginAddress(item)==siteLoginAddress(value)||(!existing.empty()&&siteLoginAddress(item)==siteLoginAddress(existing));}),list.end());list.push_back(value);auto next=prefs;next["SiteLogins"]=list;validateSiteLogins(next);prefs=next;d.close();};
   d.defaultButton(d.button("OK",243,52,50,d.accept));d.button("Cancel",243,73,50,[&]{d.close(IDCANCEL);});
  };d.DoModal();refreshLogins();
 }
 void loginsPage(){heading("User names and passwords for servers/sites",IDI_INFORMATION,48,40,256);logins=table(8,28,290,172,{{"Site/path",186},{"User",45},{"Password",45}});B("New",15,205,57,[this]{login();});loginEdit=B("Edit",87,205,57,[this]{login(true);});loginRemove=B("Remove",158,205,57,[this]{auto row=logins->GetNextItem(-1,LVNI_SELECTED);if(row>=0){prefs["SiteLogins"].erase(row);refreshLogins();}});auto windows=C("Use the current Windows login for intranet servers that ask for it",yes(prefs,"WindowsLoginIntranet",true),15,228,275);bindFlag(windows,"WindowsLoginIntranet");refreshLogins();refreshBindings.push_back([this]{refreshLogins();});}
 void soundsPage();
 void dialPage();
 void advancedOptions();
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{
  auto event=(NMHDR*)l;if(tabs&&event->hwndFrom==tabs->m_hWnd&&event->code==TCN_SELCHANGE){page();*result=0;return TRUE;}
  if(servers&&event->hwndFrom==servers->m_hWnd&&event->code==LVN_ITEMCHANGED)updateServerActions();
  if(logins&&event->hwndFrom==logins->m_hWnd&&event->code==LVN_ITEMCHANGED)updateLoginActions();
  try{if(event->code==NM_DBLCLK){if(logins&&event->hwndFrom==logins->m_hWnd){login(true);*result=0;return TRUE;}if(servers&&event->hwndFrom==servers->m_hWnd){serverRule(true);*result=0;return TRUE;}}}catch(const std::exception& e){error(this,e);*result=0;return TRUE;}
  return Form::OnNotify(w,l,result);
 }
public:
 std::function<void(CWnd*)> networkIntegration;std::function<void()> help;
 Options(Manager& m,CWnd* parent,std::shared_ptr<DialCredentialStore> credentials={},std::function<std::vector<DialEntry>()> connections={}):Form("UDM Configuration",326,309,parent),manager(m),dialStore(credentials?std::move(credentials):dialCredentialStore()),dialEntriesProvider(connections?std::move(connections):dialEntries){
  dialogUnits=true;{Lock lock(m.mutex);prefs=m.state["Settings"];}prefs["RunAtLogin"]=startupEnabled();original=prefs;
  init=[this]{
   tabs=make<ThemeTabs>(WS_TABSTOP|TCS_MULTILINE,5,4,315,283);const wchar_t* names[]={L"General",L"File types",L"Save to",L"Downloads",L"Connection",L"Proxy / Socks",L"Sites Logins",L"Dial Up / VPN",L"Sounds"};
   for(int pageId=0;pageId<9;++pageId){tabs->InsertItem(pageId,names[pageId]);const auto before=controls.size();switch(pageId){case 0:generalPage();break;case 1:typesPage();break;case 2:savePage();break;case 3:downloadsPage();break;case 4:connectionPage();break;case 5:proxyPage();break;case 6:loginsPage();break;case 7:dialPage();break;case 8:soundsPage();break;}std::vector<CWnd*> items;for(size_t i=before;i<controls.size();++i)items.push_back(controls[i].get());pages.push_back(std::move(items));}
   accept=[this]{applyOptions();close();};defaultButton(button("OK",202,291,57,accept));button("Help",263,291,57,[this]{if(help)help();else MessageBox(L"Choose a tab, change the settings, then click OK to apply them. Closing the window discards unapplied changes.",L"Configuration help",MB_OK|MB_ICONINFORMATION);});page();
  };
 }
};
