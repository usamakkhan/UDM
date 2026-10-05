// Included inside MainWindow.
 void previewZip(JobPtr job){auto entries=zipContents(job->target());Form d("ZIP contents — "+str(job->data,"FileName"),714,442,this);d.init=[&]{d.label(std::to_string(entries.size())+" entries. Preview reads the archive directory without extracting files.",13,13,688,30);auto list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS,13,49,688,337);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->InsertColumn(0,L"Name",LVCFMT_LEFT,d.rect(0,0,390,0).Width());list->InsertColumn(1,L"Size",LVCFMT_RIGHT,d.rect(0,0,105,0).Width());list->InsertColumn(2,L"Packed",LVCFMT_RIGHT,d.rect(0,0,105,0).Width());list->InsertColumn(3,L"Encrypted",LVCFMT_LEFT,d.rect(0,0,82,0).Width());for(int i=0;i<(int)entries.size();++i){auto& e=entries[i];list->InsertItem(i,cs(e.name));list->SetItemText(i,1,cs(bytes(e.size)));list->SetItemText(i,2,cs(bytes(e.compressed)));list->SetItemText(i,3,e.encrypted?L"Yes":L"No");}d.button("Close",596,401,105,[&]{d.close();});};d.DoModal();}
 void recycleDownloads(const std::vector<JobPtr>& selection){
  Form d("Recycle downloaded files",589,322,this);d.init=[&]{d.label("The selected saved files will move to the Windows Recycle Bin.",14,13,560,32);auto list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_REPORT,14,51,560,179);list->InsertColumn(0,L"Saved file",LVCFMT_LEFT,d.rect(0,0,537,0).Width());for(int i=0;i<(int)selection.size();++i)list->InsertItem(i,cs(utf8(selection[i]->target().wstring())));auto remove=d.check("Also remove these records from the download list",false,14,242,560);d.button("Recycle files",330,282,125,[&,remove]{for(auto job:selection){manager.recycleCompleted(job,d.GetSafeHwnd());if(d.checked(remove))manager.remove(job);}d.close();});d.button("Cancel",470,282,104,[&]{d.close(IDCANCEL);});};d.DoModal();
 }
 Json findQuery;
 void showFind(){Form d("Find file",230,148,this);d.dialogUnits=true;d.init=[&]{
  d.label("Find:",7,7,216,8);auto input=d.edit(str(findQuery,"Text"),7,19,216,14);
  auto filename=d.check("File name or part of the name",yes(findQuery,"FileName",true),7,36,165,10);
  auto description=d.check("Description or part of the description",yes(findQuery,"Description"),7,50,216,10);
  auto address=d.check("Site name/Download link/Parent web page/Referer",yes(findQuery,"Address"),7,64,216,19);
  d.control(L"STATIC","",SS_ETCHEDHORZ,7,88,216,1);
  auto sensitive=d.check("Match case",yes(findQuery,"MatchCase"),15,97,208,10);
  auto whole=d.check("Match whole string only",yes(findQuery,"WholeString"),15,110,208,10);
  d.accept=[&,input,filename,description,address,sensitive,whole]{if(text(input).empty())throw std::runtime_error("Enter text to find.");if(!d.checked(filename)&&!d.checked(description)&&!d.checked(address))throw std::runtime_error("Choose at least one search field.");findQuery={{"Text",text(input)},{"FileName",d.checked(filename)},{"Description",d.checked(description)},{"Address",d.checked(address)},{"MatchCase",d.checked(sensitive)},{"WholeString",d.checked(whole)}};d.close();};
  d.defaultButton(d.button("Find",48,127,50,d.accept,14));d.button("Cancel",131,127,50,[&]{d.close(IDCANCEL);},14);input->SetFocus();
 };if(d.DoModal()==IDOK)findNext();}

 void findNext(){if(str(findQuery,"Text").empty()){showFind();return;}refresh();int count=(int)visible.size(),current=table.GetNextItem(-1,LVNI_SELECTED);for(int step=1;step<=count;++step){int index=(std::max(-1,current)+step)%count;bool match;{Lock lock(manager.mutex);match=matchesDownload(visible[index]->data,findQuery);}if(match){table.SetItemState(-1,0,LVIS_SELECTED);table.SetItemState(index,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);table.EnsureVisible(index,FALSE);table.SetFocus();return;}}MessageBox(L"No matching file was found in the current category or filtered list.",L"Find file",MB_OK|MB_ICONINFORMATION);}
 std::string currentCategory(){for(auto prefix:{"category:","unfinished:","finished:"})if(filter.rfind(prefix,0)==0)return filter.substr(strlen(prefix));return {};}
 void categoryEditor(std::string original=""){
  auto p=preferences();
  Form d("UDM categories",322,177,this);d.dialogUnits=true;
  d.init=[&]{
   d.label("Category name",9,3,235,10);auto name=d.edit(original,9,13,238,14,original=="Other");
   auto paths=dictionary(p["CategoryPaths"]);std::string extensions=original.empty()?"":categoryExtensions(p,original),hosts;
   for(const auto& rule:p["CategoryRules"])if(str(rule,"Category")==original){extensions=str(rule,"Extensions");hosts=str(rule,"Hosts");break;}
   d.label("Automatically put in this category the following file types:",9,33,238,10);auto ex=d.edit(extensions,9,43,238,14);
   d.label("Note: type file extensions separated by space (e.g. avi mpg mpeg)",9,58,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,72,302,1);
   auto restrictSites=d.check("Automatically put in this category the files from the following sites only:",!hosts.empty(),9,78,302,10);
   auto sites=d.edit(hosts,9,90,302,14);sites->EnableWindow(!hosts.empty());
   d.bind(restrictSites,[&,restrictSites,sites]{sites->EnableWindow(d.checked(restrictSites));});
   d.label("Separate sites by spaces. Use *.example.com for a domain and its subdomains.",9,106,302,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,9,121,302,1);
   d.label("Save future downloads of this category to the following folder:",9,127,302,10);auto folder=d.edit(str(paths,original.c_str()),9,138,302,14);
   auto remember=d.check("Remember last save path",yes(p.value("CategoryRememberLast",Json::object()),original.c_str()),9,155,228,10);
   d.button("Browse...",261,155,50,[&,folder]{auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());},14);
   d.accept=[&,name,ex,sites,folder,restrictSites,remember]{auto value=trim(text(name));auto hosts=d.checked(restrictSites)?trim(text(sites)):std::string();if(d.checked(restrictSites)&&hosts.empty())throw std::runtime_error("Enter at least one site or turn off the site restriction.");saveCategoryProperties(manager,original,value,text(ex),hosts,trim(text(folder)),d.checked(remember));if(!original.empty()&&currentCategory()==original)filter=filter.substr(0,filter.find(':')+1)+value;d.close();};
   auto ok=d.button("OK",261,13,50,d.accept,14);d.defaultButton(ok);d.button("Cancel",261,34,50,[&]{d.close(IDCANCEL);},14);
  };d.DoModal();
 }
 void queuePopup(bool start,CPoint point){CMenu popup;popup.CreatePopupMenu();auto names=queueNames(manager);for(size_t i=0;i<names.size();++i)popup.AppendMenuW(MF_STRING,20000+(UINT)i,cs(names[i]));auto selected=popup.TrackPopupMenu(TPM_RETURNCMD|TPM_NONOTIFY|TPM_RIGHTBUTTON,point.x,point.y,this);if(selected>=20000&&selected<20000+names.size())manager.queueRun(names[selected-20000],start);}
 bool treeMenuPoint(CPoint& point){
  if(point.x==-1&&point.y==-1){
   auto item=tree.GetSelectedItem();if(!item)return false;
   tree.EnsureVisible(item);CRect rect;if(!tree.GetItemRect(item,&rect,TRUE))return false;
   point=rect.CenterPoint();tree.ClientToScreen(&point);
  }else{
   CPoint local=point;tree.ScreenToClient(&local);auto item=tree.HitTest(local);if(!item)return false;
   tree.SelectItem(item);
  }
  return true;
 }
 void treeMenu(CPoint point){
  if(!treeMenuPoint(point))return;CMenu popup;popup.CreatePopupMenu();auto cat=currentCategory();if(!cat.empty()){popup.AppendMenuW(MF_STRING,CMD_NEWCATEGORY,L"Add category...");popup.AppendMenuW(MF_STRING,CMD_EDITCATEGORY,L"Properties...");popup.AppendMenuW(MF_STRING|(cat=="Other"?MF_GRAYED:0),CMD_DELETECATEGORY,L"Delete category...");}else if(filter.rfind("queue:",0)==0){popup.AppendMenuW(MF_STRING,CMD_STARTQUEUE,L"Start queue");popup.AppendMenuW(MF_STRING,CMD_STOPQUEUE,L"Stop queue");popup.AppendMenuW(MF_STRING,CMD_SCHEDULER,L"Edit queue / Scheduler...");popup.AppendMenuW(MF_STRING,CMD_NEWQUEUE,L"Create queue...");popup.AppendMenuW(MF_STRING,CMD_DELETEQUEUE,L"Delete queue...");}else{popup.AppendMenuW(MF_STRING,CMD_NEWCATEGORY,L"Add category...");popup.AppendMenuW(MF_STRING,CMD_NEWQUEUE,L"Create queue...");}popup.TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this);
 }
 void importDownloads(int kind=0){const wchar_t* importFilter=kind==1?L"IDM export files|*.ef2;*.ief||":kind==2?L"URL lists|*.txt;*.lst||":L"Download lists|*.udmcatalog;*.json;*.ef2;*.ief;*.txt;*.lst|All files|*.*||";CFileDialog chooser(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST,importFilter,this);if(chooser.DoModal()!=IDOK)return;auto path=fs::path((LPCWSTR)chooser.GetPathName());auto source=readText(path,16*1024*1024);auto extension=lower(utf8(path.extension().wstring()));if(kind==1||extension==".ef2"||extension==".ief"){IdmImportDialog dialog(manager,parseIdmExport(source,extension==".ief"),this);dialog.DoModal();return;}if(lower(utf8(path.extension().wstring()))!=".udmcatalog"&&lower(utf8(path.extension().wstring()))!=".json"){selectBatch(this,manager,importTextUrls(source));return;}auto catalog=Json::parse(source);if(str(catalog,"Format")!="UDM catalog"||!catalog["Downloads"].is_array())throw std::runtime_error("Choose a UDM catalog file.");Form d("Import UDM catalog",570,295,this);d.init=[&]{d.label(std::to_string(catalog["Downloads"].size())+" saved download records. New downloads will be added paused.",14,14,542,37);d.label("Destination (blank = each original folder)",14,63,542);auto folder=d.edit("",14,87,424);d.button("Browse...",450,86,105,[&,folder]{auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());});auto credentials=d.check("Import encrypted request credentials (same Windows account)",false,14,126,542);auto attach=d.check("Recognize existing completed files after SHA-256 verification",false,14,161,542);d.label("Existing downloads stay intact. Imported queues start disabled; saved power actions and schedules are not armed by import.",14,200,542,43);d.button("Import",342,256,102,[&,folder,credentials,attach]{auto count=importCatalog(manager,catalog,trim(text(folder)),d.checked(credentials),d.checked(attach));d.MessageBox(cs("Imported "+std::to_string(count)+" records."),L"UDM",MB_OK|MB_ICONINFORMATION);d.close();});d.button("Cancel",454,256,102,[&]{d.close(IDCANCEL);});};d.DoModal();}
 void exportDownloads(const std::vector<JobPtr>& selection,int kind=1){ExportDownloadsDialog dialog(manager,selection,selectedQueue(),this,kind);dialog.DoModal();}
