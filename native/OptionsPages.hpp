// Included inside udm after Options.
inline void Options::savePage(){
 heading("Categories, file types, folders",OptionsPageIcon::SaveTo,55,36,260);
 G("Save To...",8,25,288,128);L("Category",15,42,138,10);
 auto categories=Q(optionCategories(prefs),"Other",15,55,160);auto selected=std::make_shared<std::string>("Other");auto changing=std::make_shared<bool>(false);auto loaded=std::make_shared<Json>();
 auto extensionsCaption=L("",16,74,271,10);auto extensions=E("",16,85,274);auto folderCaption=L("",15,106,275,10);auto folder=E("",16,118,213);
 auto remember=C("Change category folder to the last selected folder",false,17,137,273);
 auto load=[this,categories,selected,changing,loaded,extensionsCaption,extensions,folderCaption,folder,remember]{
  *changing=true;*selected=text(categories);auto types=categoryExtensions(prefs,*selected);
  extensionsCaption->SetWindowText(cs("Automatically put these file types in \""+*selected+"\":"));extensions->SetWindowText(cs(types));folderCaption->SetWindowText(cs("Default download directory for \""+*selected+"\""));folder->SetWindowText(cs(categoryFolder(prefs,*selected)));
  remember->SendMessage(BM_SETCHECK,yes(prefs.value("CategoryRememberLast",Json::object()),selected->c_str())?BST_CHECKED:BST_UNCHECKED);*loaded={{"Types",text(extensions)},{"Folder",text(folder)},{"Remember",checked(remember)}};*changing=false;
 };
 saveCategory=[this,selected,changing,loaded,extensions,folder,remember]{
  if(*changing)return;Json values={{"Types",text(extensions)},{"Folder",text(folder)},{"Remember",checked(remember)}};if(values==*loaded)return;const auto path=trim(text(folder));if(!fs::path(wide(path)).is_absolute())throw std::runtime_error("Choose an absolute category folder.");
  const auto types=lower(trim(text(extensions)));for(auto ext:words(types))if(ext!="*"&&!std::regex_match(ext,std::regex("[a-z0-9_-]{1,30}")))throw std::runtime_error("Use file extensions separated by spaces.");
  auto rules=prefs["CategoryRules"];if(text(extensions)!=str(*loaded,"Types")){if(siteRestrictedCategory(prefs,*selected)){for(auto& rule:rules)if(str(rule,"Category")==*selected)rule["Extensions"]=types;if(prefs.contains("CategoryTypeOverrides"))prefs["CategoryTypeOverrides"].erase(*selected);}else{rules.erase(std::remove_if(rules.begin(),rules.end(),[&](const Json& row){return str(row,"Category")==*selected&&str(row,"Hosts").empty();}),rules.end());prefs["CategoryTypeOverrides"][*selected]=types;}}
  prefs["CategoryRules"]=rules;if(path!=str(*loaded,"Folder")){auto paths=dictionary(prefs["CategoryPaths"]);paths[*selected]=path;prefs["CategoryPaths"]=legacyDictionary(paths);}if(checked(remember)!=yes(*loaded,"Remember"))prefs["CategoryRememberLast"][*selected]=checked(remember);*loaded=values;
 };
 bindChange(categories,[this,categories,selected,load]{try{saveCategory();load();}catch(...){categories->SetCurSel(categories->FindStringExact(-1,cs(*selected)));throw;}});
 B("New",233,35,57,[this,categories,load]{saveCategory();auto name=prompt(this,"New category");if(name.empty())return;if(safeName(name)!=name)throw std::runtime_error("Choose a valid category name.");for(const auto& value:optionCategories(prefs))if(lower(value)==lower(name))throw std::runtime_error("That category already exists.");prefs["CustomCategories"].push_back(name);categories->AddString(cs(name));categories->SetCurSel(categories->FindStringExact(-1,cs(name)));load();});
 B("Edit...",233,55,57,[this,load]{saveCategory();categoryRule(this,prefs);load();});
 B("Browse",233,118,57,[this,folder]{auto path=chooseFolder(this,wide(text(folder)));if(!path.empty())folder->SetWindowText(path.c_str());});
 auto date=C("Set file creation date as provided by the server",yes(prefs,"UseServerDate"),17,159,273);bindFlag(date,"UseServerDate");
 G("Temporary directory",8,173,288,77);auto temporary=E(str(prefs,"TemporaryFolder"),15,186,213);bindText(temporary,"TemporaryFolder");B("Browse",232,186,57,[this,temporary]{auto value=chooseFolder(this,wide(text(temporary)));if(!value.empty())temporary->SetWindowText(value.c_str());});
 L("Temporary directory stores file parts during download. Leave it blank to use UDM's default location.",15,203,275,42);reloadCategory=load;load();
}
inline void Options::downloadsPage(){
 heading("Default download settings",OptionsPageIcon::Downloads,55,36,260);
 L("Customize \"Download progress\" dialog",8,28,217,10);B("Edit...",233,25,57,[this]{collect();progressPreferences(this,prefs);reload();});
 auto start=C("Show start download dialog",!yes(prefs,"SkipBrowserFileInfo"),10,44,274);bindFlag(start,"SkipBrowserFileInfo",true);
 auto later=C("Do not start downloading; only add files to the queue",yes(prefs,"BrowserDownloadLater"),30,57,268);bindFlag(later,"BrowserDownloadLater");auto updateLater=[this,start,later]{later->EnableWindow(!checked(start));};bind(start,updateLater);refreshBindings.push_back(updateLater);updateLater();
 auto complete=C("Show download complete dialog",!yes(prefs,"SuppressCompletionDialog"),10,69,277);bindFlag(complete,"SuppressCompletionDialog",true);
 L("Note: These settings don't relate to queue processing",16,82,270,10);rule(8,93,289);
 auto prefetch=C("Start downloading immediately while displaying \"Download File Info\" dialog",yes(prefs,"PrefetchFileInfo",true),10,96,286,19);bindFlag(prefetch,"PrefetchFileInfo");
 auto queueLater=C("Show queue selection panel on pressing Download Later",yes(prefs,"QueuePromptLater",true),10,119,286);bindFlag(queueLater,"QueuePromptLater");
 auto queueBatch=C("Show queue selection panel on closing batch downloads",yes(prefs,"QueuePromptBatch",true),10,134,287);bindFlag(queueBatch,"QueuePromptBatch");
 auto dates=C("Ignore file modification time when resuming downloads",yes(prefs,"IgnoreLastModified"),10,149,286);bindFlag(dates,"IgnoreLastModified");
 rule(8,164,289);L("If a duplicate download link is added:",8,170,280,10);
 const std::vector<std::string> choices={"Show a dialog and ask what to do","Add a numbered copy","Overwrite the existing file","Show or resume the existing download"};auto policy=str(prefs,"DuplicatePolicy","Ask");auto duplicate=Q(choices,choices[policy=="Numbered"?1:policy=="Replace"?2:policy=="Existing"?3:0],8,180,288);auto last=std::make_shared<int>(duplicate->GetCurSel());
 refreshBindings.push_back([this,duplicate,last]{auto policy=str(prefs,"DuplicatePolicy","Ask");*last=policy=="Numbered"?1:policy=="Replace"?2:policy=="Existing"?3:0;duplicate->SetCurSel(*last);});
 bindings.push_back([this,duplicate,last]{auto value=duplicate->GetCurSel();if(value!=*last){prefs["DuplicatePolicy"]=value==1?"Numbered":value==2?"Replace":value==3?"Existing":"Ask";*last=value;}});
 rule(8,197,289);L("User-Agent for manually added downloads:",8,203,289);auto agent=E(str(prefs,"UserAgent"),8,216,289);bindText(agent,"UserAgent");
 rule(8,233,289);L("Virus checking settings",8,242,214,8);B("Edit...",233,239,57,[this]{collect();scannerSettings(this,prefs);reload();});
}
inline void Options::proxyPage(){
 heading("Proxy / socks configuration",OptionsPageIcon::Proxy,41,0,0,168);
 auto browser=C("Use the proxy captured with a browser download",yes(prefs,"UseBrowserProxy",true),15,27,277,19);bindFlag(browser,"UseBrowserProxy");
 rule(9,50,289);auto direct=C("No proxy/socks",false,8,55,282);direct->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);
 rule(9,69,289);auto system=C("Use system settings",false,8,73,283);system->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);
 rule(8,86,289);auto pac=C("Use automatic configuration script",false,8,90,266);pac->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);L("Address",15,105,40);auto script=E(str(prefs,"ProxyAutoConfigUrl"),60,102,231);bindText(script,"ProxyAutoConfigUrl");rule(8,120,289);
 auto manual=C("Manual proxy/socks configuration",false,9,124,261);manual->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);
 L("Proxy server address",15,136,96,10);L("Port",123,136,39,10);L("UserName",166,136,59,10);L("Password",230,136,61,10);
 auto current=str(prefs,"Proxy");auto colon=current.rfind(':');std::string host=current,port="8080";if(colon!=std::string::npos){host=current.substr(0,colon);port=current.substr(colon+1);}
 auto address=E(host,15,147,104,15);auto proxyPort=E(port,123,147,39,15);auto user=E(str(prefs,"ProxyUser"),166,147,59,15);auto password=E(reveal(str(prefs,"ProxySecret")),229,147,63,15,true);
 L("Use this proxy for the following protocols:",15,164,268,10);
 auto http=C("http",true,15,176,32);auto https=C("https",true,66,176,35);auto ftp=C("ftp",true,124,176,27);
 auto protocolOn=[&](const char* name){auto rows=prefs.value("ProtocolProxies",Json::object());return !rows.contains(name)||str(rows[name],"ProxyMode")!="Connect directly";};
 http->SendMessage(BM_SETCHECK,protocolOn("http")?BST_CHECKED:BST_UNCHECKED);https->SendMessage(BM_SETCHECK,protocolOn("https")?BST_CHECKED:BST_UNCHECKED);ftp->SendMessage(BM_SETCHECK,protocolOn("ftp")?BST_CHECKED:BST_UNCHECKED);
 auto mode=std::make_shared<std::string>(str(prefs,"ProxyMode",current.empty()?"Use Windows proxy / PAC settings":"Use a proxy server"));
 auto update=[this,direct,system,pac,script,manual,address,proxyPort,user,password,http,https,ftp,mode]{bool scripted=*mode=="Use automatic configuration script";bool custom=!scripted&&*mode!="Connect directly"&&*mode!="Use Windows proxy / PAC settings";pac->SendMessage(BM_SETCHECK,scripted?BST_CHECKED:BST_UNCHECKED);script->EnableWindow(scripted);direct->SendMessage(BM_SETCHECK,*mode=="Connect directly"?BST_CHECKED:BST_UNCHECKED);system->SendMessage(BM_SETCHECK,*mode=="Use Windows proxy / PAC settings"?BST_CHECKED:BST_UNCHECKED);manual->SendMessage(BM_SETCHECK,custom?BST_CHECKED:BST_UNCHECKED);for(auto c:{address,proxyPort,user,password,http,https,ftp})c->EnableWindow(custom);};
 bind(direct,[mode,update]{*mode="Connect directly";update();});bind(system,[mode,update]{*mode="Use Windows proxy / PAC settings";update();});bind(pac,[mode,update]{*mode="Use automatic configuration script";update();});bind(manual,[mode,update]{*mode="Use a proxy server";update();});update();
 auto previous=std::make_shared<Json>();auto gather=[this,address,proxyPort,user,password,http,https,ftp,mode]{
  auto full=trim(text(address));if(!full.empty()&&*mode!="Connect directly"&&*mode!="Use Windows proxy / PAC settings"&&*mode!="Use automatic configuration script"){auto n=std::stoll(text(proxyPort));if(n<1||n>65535)throw std::runtime_error("Enter a proxy port from 1 to 65535.");full+=":"+std::to_string(n);}else if(!full.empty())full+=":"+text(proxyPort);
  return Json{{"ProxyMode",*mode},{"Proxy",full},{"ProxyUser",text(user)},{"Password",text(password)},{"http",checked(http)},{"https",checked(https)},{"ftp",checked(ftp)}};
 };
 *previous=gather();auto save=[this,gather,previous]{auto values=gather();if(values==*previous)return;for(const char* key:{"ProxyMode","Proxy","ProxyUser"})prefs[key]=values[key];prefs["ProxySecret"]=protect(str(values,"Password"));auto rows=prefs.value("ProtocolProxies",Json::object());for(const char* scheme:{"http","https","ftp"}){if(!yes(values,scheme))rows[scheme]={{"ProxyMode","Connect directly"}};else if(rows.contains(scheme)&&str(rows[scheme],"ProxyMode")=="Connect directly")rows.erase(scheme);}prefs["ProtocolProxies"]=rows;*previous=values;};bindings.push_back(save);
 auto refreshProxy=[this,mode,address,proxyPort,user,password,http,https,ftp,previous,gather,update]{
  *mode=str(prefs,"ProxyMode",str(prefs,"Proxy").empty()?"Use Windows proxy / PAC settings":"Use a proxy server");auto proxy=str(prefs,"Proxy");auto separator=proxy.rfind(':');address->SetWindowText(cs(separator==std::string::npos?proxy:proxy.substr(0,separator)));proxyPort->SetWindowText(cs(separator==std::string::npos?"8080":proxy.substr(separator+1)));user->SetWindowText(cs(str(prefs,"ProxyUser")));password->SetWindowText(cs(reveal(str(prefs,"ProxySecret"))));
  auto rows=prefs.value("ProtocolProxies",Json::object());for(auto item:std::vector<std::pair<const char*,CWnd*>>{{"http",http},{"https",https},{"ftp",ftp}})item.second->SendMessage(BM_SETCHECK,!rows.contains(item.first)||str(rows[item.first],"ProxyMode")!="Connect directly"?BST_CHECKED:BST_UNCHECKED);update();*previous=gather();
 };refreshBindings.push_back(refreshProxy);
 B("Advanced / Socks...",170,179,122,[this]{collect();protocolProxyDialog(this,prefs);reload();});
 B("Get System",224,8,72,[this]{
  collect();WINHTTP_CURRENT_USER_IE_PROXY_CONFIG config{};if(!WinHttpGetIEProxyConfigForCurrentUser(&config))throw std::runtime_error("Windows proxy settings could not be read.");
  const auto proxy=config.lpszProxy?utf8(config.lpszProxy):"",bypass=config.lpszProxyBypass?utf8(config.lpszProxyBypass):"";bool automatic=config.fAutoDetect||config.lpszAutoConfigUrl||proxy.find_first_of("=;")!=std::string::npos;
  if(config.lpszProxy)GlobalFree(config.lpszProxy);if(config.lpszProxyBypass)GlobalFree(config.lpszProxyBypass);if(config.lpszAutoConfigUrl)GlobalFree(config.lpszAutoConfigUrl);
  prefs["ProxyMode"]=automatic?"Use Windows proxy / PAC settings":proxy.empty()?"Connect directly":"Use a proxy server";prefs["Proxy"]=automatic?"":proxy;prefs["ProxyBypass"]=bypass;prefs["ProxyUser"]="";prefs["ProxySecret"]="";prefs["ProtocolProxies"]=Json::object();reload();
 });
 G("Do not use proxy for addresses beginning with:",15,197,281,41);auto bypass=E(str(prefs,"ProxyBypass"),24,208,266,17);bindText(bypass,"ProxyBypass");L("Separate names by semicolons",22,227,260,10);rule(8,241,289);
 auto passive=C("Use FTP in PASV mode",yes(prefs,"FtpPassive",true),13,244,110);bindFlag(passive,"FtpPassive");
 L("FTP proxy type",140,246,52,10);const std::vector<std::string> ftpTypes={"CONNECT","USER user@host","SITE host","OPEN host"};auto ftpType=Q(ftpTypes,ftpProxyType(prefs),196,243,96);
 refreshBindings.push_back([this,ftpType,ftpTypes]{auto current=ftpProxyType(prefs);for(size_t i=0;i<ftpTypes.size();++i)if(ftpTypes[i]==current)ftpType->SetCurSel((int)i);});
 bindings.push_back([this,ftpType,ftpTypes]{auto sel=ftpType->GetCurSel();if(sel>=0&&sel<(int)ftpTypes.size()&&ftpTypes[sel]!=ftpProxyType(prefs))prefs["FtpProxyType"]=ftpTypes[sel];});
}
inline void Options::soundsPage(){
 heading("Sound settings",OptionsPageIcon::Sounds,55,39,263);G("Select sounds for UDM events",11,30,290,188);sounds=table(17,40,278,154,{{"Event",120},{"Sound file",140}},true);
 const std::vector<std::string> keys={"Sound","FailureSoundEnabled","PauseSoundEnabled"},paths={"CompletionSoundFile","FailureSoundFile","PauseSoundFile"},titles={"Download complete","Download failed","Download paused"};
 for(int i=0;i<3;++i){sounds->InsertItem(i,cs(titles[i]));sounds->SetItemText(i,1,cs(str(prefs,paths[i].c_str()).empty()?"Windows default":str(prefs,paths[i].c_str())));sounds->SetCheck(i,yes(prefs,keys[i].c_str(),i==0));}
 auto selected=[this]{int row=sounds->GetNextItem(-1,LVNI_SELECTED);if(row<0)throw std::runtime_error("Select a sound event.");return row;};
 B("Browse...",72,199,57,[this,paths,selected]{int row=selected();CFileDialog picker(TRUE,L"wav",nullptr,OFN_FILEMUSTEXIST,L"Wave sound|*.wav||",this);if(picker.DoModal()==IDOK){auto path=utf8((LPCWSTR)picker.GetPathName());prefs[paths[row]]=path;sounds->SetItemText(row,1,cs(path));sounds->SetCheck(row,TRUE);}});
 B("Play",174,199,57,[this,paths,selected]{auto value=str(prefs,paths[selected()].c_str());if(value.empty())MessageBeep(MB_OK);else if(!PlaySoundW(wide(value).c_str(),nullptr,SND_FILENAME|SND_ASYNC|SND_NODEFAULT))throw std::runtime_error("Windows could not play this WAV file.");});
 bindings.push_back([this,keys,paths]{for(int i=0;i<3;++i){auto value=str(prefs,paths[i].c_str());if(!value.empty()&&!fs::is_regular_file(fs::path(wide(value))))throw std::runtime_error("Choose an existing sound file.");prefs[keys[i]]=sounds->GetCheck(i)!=FALSE;}});
 sounds->SetItemState(0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
}
inline void Options::dialPage(){
 heading("Dial up / VPN settings",OptionsPageIcon::DialUp,53,40,256);
 auto enabled=C("Use Windows Dial Up / VPN Networking",yes(prefs,"DialEnabled"),9,30,281);bindFlag(enabled,"DialEnabled");
 G("Connection options",8,44,289,110);L("Connection:",20,62,65,10);auto choice=Q({},"",91,59,178);auto entries=std::make_shared<std::vector<DialEntry>>();auto status=L("",15,214,275,26);
 dialCredentials=std::make_shared<DialCredentialFields>(*this,ox,oy,status,dialStore);
 auto selected=[entries,choice]{auto row=choice->GetCurSel();if(row<0||row>=(int)entries->size())throw std::runtime_error("Select a Windows connection.");return entries->at(row);};
 auto load=[this,selected,status]{try{dialCredentials->load(selected());}catch(const std::exception& e){dialCredentials->clear();status->SetWindowText(cs(e.what()));}};
 auto refresh=[this,entries,choice,status,load]{try{
  *entries=dialEntriesProvider();choice->ResetContent();int selected=-1;
  for(int i=0;i<(int)entries->size();++i){auto entry=entries->at(i);int count=0;for(const auto& other:*entries)if(other.name==entry.name)++count;auto label=entry.name;if(count>1)label+=" ("+entry.phonebook+")";choice->AddString(cs(label));if(entry.name==str(prefs,"DialEntry")&&(str(prefs,"DialPhonebook").empty()||entry.phonebook==str(prefs,"DialPhonebook")))selected=i;}
  choice->SetCurSel(selected<0&&entries->size()==1?0:selected);
  if(entries->empty()){dialCredentials->clear();status->SetWindowText(L"No Windows connections found. Use More to create one.");}else load();
 }catch(const std::exception& e){dialCredentials->clear();status->SetWindowText(cs(e.what()));}};
 bindChange(choice,load);
 B("Connect...",91,243,80,[this,selected]{dialCredentials->applyCurrent();auto entry=selected();auto name=wide(entry.name),book=wide(entry.phonebook);RASDIALDLG info{};info.dwSize=sizeof(info);info.hwndOwner=m_hWnd;RasDialDlgW(book.empty()?nullptr:book.data(),name.data(),nullptr,&info);if(info.dwError)throw std::runtime_error("Windows connection failed ("+std::to_string(info.dwError)+").");});
 B("More...",233,243,57,[this,refresh]{collect();Form d("Windows connection options",602,410,this);std::vector<std::function<void()>> apply;d.init=[&]{dialUpOptions(d,prefs,apply);d.accept=[&]{for(auto& action:apply)action();d.close();};d.button("OK",407,376,83,d.accept);d.button("Cancel",502,376,83,[&]{d.close(IDCANCEL);});};d.DoModal();refresh();reload();});
 G("Redial options",8,158,289,54);L("Redial attempts (zero if endlessly):",15,172,150,10);auto attempts=E(std::to_string(num(prefs,"DialAttempts",3)),172,169,32);L("times",209,172,77,10);L("Time between redial attempts:",16,192,149,10);auto delay=E(std::to_string(num(prefs,"DialRetrySeconds",10)),172,189,32);L("seconds",209,192,79,10);bindText(attempts,"DialAttempts",true);bindText(delay,"DialRetrySeconds",true);
 bindings.push_back([this,selected]{try{auto entry=selected();prefs["DialEntry"]=entry.name;prefs["DialPhonebook"]=entry.phonebook;}catch(...){if(yes(prefs,"DialEnabled"))throw;}});refresh();
}
inline void Options::advancedOptions(){
 collect();Form d("Additional UDM settings",520,393,this);Json next=prefs;
 d.init=[&]{
  d.label("Default download folder",14,18,202);auto folder=d.edit(str(next,"DownloadFolder"),230,14,180);d.button("Browse...",420,13,86,[&d,folder]{auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());});
  auto categories=d.check("Use separate folders for categories without a saved path",yes(next,"CategoryFolders",true),14,50,492);
  auto tray=d.check("Close the main window to the notification area",yes(next,"CloseToTray"),14,83,492);
  d.label("Simultaneous downloads (1-16)",14,122,255);auto parallel=d.edit(std::to_string(num(next,"Parallel",3)),366,118,140);
  d.label("Retries per connection (0-10)",14,158,255);auto retries=d.edit(std::to_string(num(next,"Retries",3)),366,154,140);
  d.label("Global speed limit (KB/s; 0 = off)",14,194,255);auto speed=d.edit(std::to_string(num(next,"LimitKbps")),366,190,140);
  d.label("Global speed limit applies to",14,230,210);auto mode=d.combo({"Total across downloads","Each download"},str(next,"GlobalLimitMode","Total across downloads"),230,226,276);
  d.label("Clipboard capture",14,267,202);auto clipboard=d.combo({"Show a suggestion","Open Download File Info"},str(next,"ClipboardMode","Show a suggestion"),230,263,276);
  auto types=d.check("Monitor clipboard only for extensions listed in File types",yes(next,"ClipboardOnlyFileTypes",true),14,296,492);
  d.accept=[&,folder,categories,tray,parallel,retries,speed,mode,clipboard,types]{next["DownloadFolder"]=trim(text(folder));next["CategoryFolders"]=d.checked(categories);next["CloseToTray"]=d.checked(tray);next["Parallel"]=std::stoll(text(parallel));next["Retries"]=std::stoll(text(retries));next["LimitKbps"]=std::stoll(text(speed));next["GlobalLimitMode"]=text(mode);next["ClipboardMode"]=text(clipboard);next["ClipboardOnlyFileTypes"]=d.checked(types);prefs=next;d.close();};
  d.button("Browser integration...",14,321,240,[&d,&next]{browserIntegration(&d,next);});d.button("Network integration...",266,321,240,[this,&d]{if(networkIntegration)networkIntegration(&d);else showNetworkIntegration(&d);});
  d.defaultButton(d.button("OK",326,351,85,d.accept));d.button("Cancel",421,351,85,[&]{d.close(IDCANCEL);});
 };d.DoModal();reload();
}
