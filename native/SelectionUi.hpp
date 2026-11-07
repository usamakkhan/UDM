// Included inside udm, after Form.
struct ImportSelectionForm:Form{
 using Form::Form;
 CListCtrl* importList=nullptr;std::function<void(int,bool)> sortColumn;bool changingChecks=false;std::function<void(int,bool)> selectionChanged;std::function<void()> highlightChanged;
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{
  auto event=reinterpret_cast<NMHDR*>(l);
  if(importList&&event->hwndFrom==importList->GetSafeHwnd()&&event->code==LVN_COLUMNCLICK){
   try{if(sortColumn)sortColumn(reinterpret_cast<NMLISTVIEW*>(l)->iSubItem,true);}catch(const std::exception& e){error(this,e);}
   *result=0;return TRUE;
  }
  if(importList&&event->hwndFrom==importList->GetSafeHwnd()&&event->code==LVN_ITEMCHANGED&&!changingChecks&&selectionChanged){
   auto change=reinterpret_cast<NMLISTVIEW*>(l);if(change->iItem>=0&&(change->uChanged&LVIF_STATE)&&((change->uOldState^change->uNewState)&LVIS_STATEIMAGEMASK))selectionChanged((int)importList->GetItemData(change->iItem),((change->uNewState&LVIS_STATEIMAGEMASK)>>12)==2);
  }
  if(importList&&event->hwndFrom==importList->GetSafeHwnd()&&event->code==LVN_ITEMCHANGED&&!changingChecks&&highlightChanged){auto change=reinterpret_cast<NMLISTVIEW*>(l);if((change->uChanged&LVIF_STATE)&&((change->uOldState^change->uNewState)&LVIS_SELECTED))highlightChanged();}
  return Form::OnNotify(w,l,result);
 }
};
inline std::string batchServerExtension(const Json& metadata){
 if(str(metadata,"Status")!="Ready")return "";
 const auto name=str(metadata,"SuggestedFileName");if(name.empty())return "";
 return utf8(fs::path(wide(name)).extension().wstring());
}
inline std::string batchExtensionPattern(const std::string& pattern,const std::string& extension){
 if(pattern.empty()||extension.empty())return pattern;
 const auto dot=pattern.rfind('.');auto stem=dot==std::string::npos?pattern:pattern.substr(0,dot);
 return stem.find('*')==std::string::npos?pattern:stem+extension;
}
inline std::string batchOutputName(const std::string& pattern,const std::string& token,const std::string& extension={}){
 if(pattern.empty())return "";
 if(pattern.find('*')==std::string::npos)throw std::runtime_error("Include * in the file name pattern for the original number or letter.");
 const auto adjusted=batchExtensionPattern(pattern,extension);std::string result;for(char character:adjusted){if(character=='*')result+=token;else result+=character;if(result.size()>240)throw std::runtime_error("The generated file name is too long.");}
 if(result.empty()||safeName(result)!=result)throw std::runtime_error("Enter a valid file name pattern without folders.");
 return result;
}
inline bool selectBatch(CWnd* parent,Manager& manager,const std::vector<std::string>& urls,Headers initialHeaders={},const std::vector<std::string>& sequenceTokens={},const std::vector<BrowserBatchRow>* browserRows=nullptr){
 if(!sequenceTokens.empty()&&sequenceTokens.size()!=urls.size())throw std::runtime_error("The batch sequence does not match its links.");
 if(browserRows&&browserRows->size()!=urls.size())throw std::runtime_error("The browser batch does not match its links.");
 std::string renamePattern,firstServerExtension;bool patternExtensionAdjusted=false;CWnd* patternControl=nullptr;
 struct ImportRow{std::string address,name,folder,description;Json metadata=Json::object();bool checked=true,selectionTouched=false,descriptionEdited=false;std::string protectedRequestOverrides,browserName,browserExtension;};std::vector<ImportRow> rows;for(const auto& address:urls)rows.push_back({address,"","",""});
 if(browserRows)for(auto& row:rows){row.checked=false;row.selectionTouched=true;}
 std::unique_ptr<BatchDownloadPreview> metadata;
 Json previewPrefs;
 std::function<void(std::optional<int>)> refreshMetadata;
 std::function<void()> rebuildList,snapshotChecks;
 bool hideHtml=false,hideRepeated=browserRows!=nullptr,hideImages=browserRows!=nullptr;
 auto outputName=[&](int index){return !rows[index].name.empty()?rows[index].name:sequenceTokens.empty()?std::string{}:batchOutputName(renamePattern,sequenceTokens[index],batchServerExtension(rows[index].metadata));};
 auto columnText=[&](int index){return browserRows?browserRows->at(index).linkText:rows[index].description;};
 auto refreshBrowserName=[&](int index){
  if(!browserRows)return;
  auto& row=rows[index];auto name=row.name;
  if(name.empty()){
   const auto suggested=str(row.metadata,"SuggestedFileName");
   name=!suggested.empty()&&safeName(suggested)==suggested?suggested:safeName(unescape(Url(row.address).path));
  }
  row.browserName=std::move(name);
  auto extension=lower(utf8(fs::path(wide(row.browserName)).extension().wstring()));
  row.browserExtension=extension.size()>=2&&extension.size()<=12?std::move(extension):std::string{};
 };
 if(browserRows)for(int i=0;i<(int)rows.size();++i)refreshBrowserName(i);
 auto displayName=[&](int index){
  if(browserRows)return rows[index].browserName;
  auto name=outputName(index);
  if(name.empty())name=str(rows[index].metadata,"SuggestedFileName");
  return name.empty()?safeName(unescape(Url(rows[index].address).path)):name;
 };
 auto sizeText=[&](int index){
  const auto& details=rows[index].metadata;const auto size=num(details,"Size",-1);
  if(size<0)return std::string(details.empty()?"Checking...":"Unknown");
  return browserRows?bytes(size):std::to_string(size)+" bytes";
 };
 std::optional<AsyncFileIcons> fileDetails;
 if(browserRows)fileDetails.emplace(FileIconMode::SmallWithType);
 CImageList fileImages;
 struct FileType {int icon=I_IMAGENONE;std::string name;};
 std::map<std::string,FileType> fileTypes;
 int imagePixels=0;
 auto typeText=[&](int index){
  if(browserRows){
   auto found=fileTypes.find(rows[index].browserExtension);
   if(found!=fileTypes.end()&&!found->second.name.empty())return found->second.name;
  }
  return str(rows[index].metadata,"ContentType");
 };
 std::function<std::string(int)> destinationText=[&](int index){return rows[index].folder;};
 ImportSelectionForm d(browserRows?"Download All Links with UDM":"Import links to UDM",browserRows?532:690,browserRows?335:sequenceTokens.empty()?500:560,parent);
 d.dialogUnits=d.resizable=browserRows!=nullptr;
 int sortedColumn=-1;bool ascending=true;std::set<int> added;Headers headers=std::move(initialHeaders);CWnd *confirm=nullptr,*duplicateFilter=nullptr,*useDescriptions=nullptr;
 auto browserContext=[&](int index)->const BrowserBatchRow*{
  if(!browserRows)return nullptr;
  const auto& source=browserRows->at(index);
  return source.address==rows[index].address?&source:nullptr;
 };
 auto baseRequestHeaders=[&](int index){
  auto result=headers;
  if(auto source=browserContext(index))for(const auto& [name,value]:source->headers)setHeader(result,name,value);
  return result;
 };
 auto requestHeaders=[&](int index){return batchRequestHeaders(baseRequestHeaders(index),rows[index].protectedRequestOverrides);};
 d.init=[&]{
 CWnd* htmlFilter=nullptr;
 if(browserRows){
  auto instructions=d.label("Check the links you want to add to the download list, then click OK.\r\nYou can wait for UDM to fill in the file types and sizes.",11,3,514,18);
  d.anchorControl(instructions,ResizeAnchor::StretchWidth);
 }else{
  d.label("Check the files you want to download:",13,13,310);
  htmlFilter=d.check("Hide HTML files",false,340,10,140);
  duplicateFilter=d.check("Hide repeated files",false,490,10,187);
 }
 auto list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|(browserRows?LVS_SHAREIMAGELISTS:0),browserRows?7:13,browserRows?23:39,browserRows?518:664,browserRows?222:251);
 d.importList=list;
 list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|(browserRows?LVS_EX_GRIDLINES:0));
 const wchar_t* headings[]{browserRows?L"File Name":L"File name",browserRows?L"Download from":L"Address",L"Size",browserRows?L"File type":L"Type",browserRows?L"Save to":L"Folder override",browserRows?L"Link Text":L"Description"};
 const int browserWidths[]{81,133,47,25,133,67},importWidths[]{210,330,110,180,230,240};
 for(int column=0;column<6;++column)
  list->InsertColumn(column,headings[column],column==2&&!browserRows?LVCFMT_RIGHT:LVCFMT_LEFT,d.pixelsX(browserRows?browserWidths[column]:importWidths[column]));
 if(browserRows){int order[]{0,3,2,1,4,5};list->SetColumnOrderArray(6,order);d.anchorControl(list,ResizeAnchor::StretchBoth);}
 for(int i=0;i<(int)urls.size();++i){
  list->InsertItem(i,cs(browserRows?rows[i].browserName:safeName(unescape(Url(urls[i]).path))));
  list->SetItemText(i,1,cs(urls[i]));list->SetItemData(i,(DWORD_PTR)i);list->SetCheck(i,rows[i].checked);
  if(browserRows)list->SetItemText(i,5,cs(columnText(i)));
 }
 d.selectionChanged=[&](int index,bool checked){if(index>=0&&index<(int)rows.size()){rows[index].checked=checked&&!added.count(index);rows[index].selectionTouched=true;}};
 refreshMetadata=[&,list](std::optional<int> changed){
  if(!changed){
   metadata.reset();firstServerExtension.clear();patternExtensionAdjusted=false;
   Lock lock(manager.mutex);previewPrefs=manager.state["Settings"];
  }
  const int begin=changed.value_or(0),end=changed?*changed+1:(int)rows.size();
  std::vector<BatchPreviewRequest> requests(end-begin);
  const auto savedLogins=previewPrefs.value("SiteLogins",Json::array());
  for(int i=begin;i<end;++i){
   if(added.count(i))continue;
   auto& row=rows[i];row.metadata=Json::object();
   const auto* source=browserContext(i);
   // Preview data is scoped to the observed request. Edited or differently
   // authenticated requests use the ordinary metadata probe.
   if(source&&source->observedSize>0&&headers.empty()&&row.protectedRequestOverrides.empty()&&
      (source->proxy.empty()||yes(previewPrefs,"UseBrowserProxy",true))){
    const auto origin=Url(row.address).origin;
    const bool couldUseSavedLogin=std::any_of(savedLogins.begin(),savedLogins.end(),[&](const Json& login){return str(login,"Origin")==origin;});
    if(!couldUseSavedLogin){
     row.metadata={{"Status","Ready"},{"Size",source->observedSize},{"ContentType",source->observedType},
       {"SuggestedFileName",previewResponseFilename(row.address,source->observedName,source->observedType,yes(previewPrefs,"CorrectFileExtensions",true))}};
     refreshBrowserName(i);
     continue;
    }
   }
   refreshBrowserName(i);
   auto& request=requests[i-begin];
   request.address=row.address;
   request.headers=baseRequestHeaders(i);
   request.protectedOverrides=row.protectedRequestOverrides;
   if(source){request.proxy=source->proxy;request.session=source->session;}
  }
  if(changed)metadata->replace(*changed,std::move(requests.front()));
  else metadata=std::make_unique<BatchDownloadPreview>(std::move(requests),previewPrefs);
  for(int position=0;position<list->GetItemCount();++position){
   const int index=(int)list->GetItemData(position);
   if(index<begin||index>=end||added.count(index))continue;
   list->SetItemText(position,2,cs(sizeText(index)));list->SetItemText(position,3,cs(typeText(index)));
   if(browserRows){
    list->SetItemText(position,0,cs(displayName(index)));
    list->SetItemText(position,4,cs(destinationText(index)));
   }
  }
  if(hideHtml)rebuildList();
  if(browserRows&&sortedColumn>=0)d.sortColumn(sortedColumn,false);
 };
 d.sortColumn=[&,list](int column,bool toggle){
  if(column<0||column>5)return;if(sortedColumn==column){if(toggle)ascending=!ascending;}else{sortedColumn=column;ascending=true;}
  struct SortContext{std::vector<std::wstring> keys;std::vector<i64> sizes;bool ascending;int column;};SortContext context{{},{},ascending,column};
  for(int i=0;i<(int)rows.size();++i){const auto& row=rows[i];auto name=column==0?displayName(i):std::string{};context.keys.push_back(wide(column==5?columnText(i):column==4?destinationText(i):column==3?typeText(i):column==1?row.address:name.empty()?safeName(unescape(Url(row.address).path)):name));context.sizes.push_back(num(row.metadata,"Size",-1));}
  list->SortItems([](LPARAM a,LPARAM b,LPARAM data)->int{const auto& context=*reinterpret_cast<const SortContext*>(data);const auto& left=context.keys.at((size_t)a);const auto& right=context.keys.at((size_t)b);int result=0;if(context.column==2){const auto leftSize=context.sizes.at((size_t)a),rightSize=context.sizes.at((size_t)b);if((leftSize<0)!=(rightSize<0))return leftSize<0?1:-1;result=leftSize<rightSize?-1:leftSize>rightSize?1:0;}else{if(context.column==3&&left.empty()!=right.empty())return left.empty()?1:-1;result=CompareStringOrdinal(left.c_str(),(int)left.size(),right.c_str(),(int)right.size(),TRUE)-CSTR_EQUAL;}if(result==0)return a<b?-1:a>b?1:0;return context.ascending?result:-result;},reinterpret_cast<DWORD_PTR>(&context));
  if(auto header=list->GetHeaderCtrl())for(int i=0;i<6;++i){HDITEM item{};item.mask=HDI_FORMAT;if(header->GetItem(i,&item)){item.fmt&=~(HDF_SORTUP|HDF_SORTDOWN);if(i==column)item.fmt|=ascending?HDF_SORTUP:HDF_SORTDOWN;header->SetItem(i,&item);}}
 };
 snapshotChecks=[&,list]{for(int position=0;position<list->GetItemCount();++position){int index=(int)list->GetItemData(position);rows[index].checked=!added.count(index)&&list->GetCheck(position);}};
 rebuildList=[&,list]{
  snapshotChecks();int selected=list->GetNextItem(-1,LVNI_SELECTED);const int selectedId=selected<0?-1:(int)list->GetItemData(selected);std::vector<int> visible;std::set<std::string> seen;
  bool hasRepeated=false;
  for(int i=0;i<(int)rows.size();++i){
   const auto& row=rows[i];
   const bool repeated=(browserRows||hideRepeated)&&!seen.insert(row.address).second;
   hasRepeated|=repeated;
   if(hideRepeated&&repeated)continue;
   if(hideImages){const auto* source=browserContext(i);if(source&&source->imageOnPage)continue;}
   if(hideHtml){
    auto name=browserRows?row.browserName:str(row.metadata,"SuggestedFileName");
    if(name.empty())name=safeName(unescape(Url(row.address).path));
    const auto ext=lower(utf8(fs::path(wide(name)).extension().wstring()));
    const auto rawType=str(row.metadata,"ContentType");
    const auto type=lower(trim(rawType.substr(0,rawType.find(';'))));
    if(ext==".htm"||ext==".html"||type=="text/html"||type=="application/xhtml+xml")continue;
   }
   visible.push_back(i);
  }
  if(browserRows)duplicateFilter->EnableWindow(hasRepeated);
  // Metadata changes need a rebuild only if the filter's visible set changed.
  std::set<int> current;for(int position=0;position<list->GetItemCount();++position)current.insert((int)list->GetItemData(position));if(current==std::set<int>(visible.begin(),visible.end()))return;
  list->SetRedraw(FALSE);d.changingChecks=true;list->DeleteAllItems();for(int index:visible){std::string name;try{name=displayName(index);}catch(...){name=rows[index].name.empty()?safeName(unescape(Url(rows[index].address).path)):rows[index].name;}int position=list->InsertItem(list->GetItemCount(),cs(name));list->SetItemData(position,index);list->SetItemText(position,1,cs(rows[index].address));list->SetItemText(position,2,cs(sizeText(index)));list->SetItemText(position,3,cs(typeText(index)));list->SetItemText(position,4,cs(destinationText(index)));list->SetItemText(position,5,cs(columnText(index)));list->SetCheck(position,rows[index].checked&&!added.count(index));if(index==selectedId)list->SetItemState(position,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);}
  d.changingChecks=false;try{if(sortedColumn>=0)d.sortColumn(sortedColumn,false);}catch(...){}list->SetRedraw(TRUE);list->Invalidate(FALSE);
 };
 auto checkAll=d.button(browserRows?"Check All":"Check all",browserRows?311:13,browserRows?250:300,browserRows?60:94,[list,&added,&rows]{for(int i=0;i<list->GetItemCount();++i){int index=(int)list->GetItemData(i);if(!added.count(index)){rows[index].selectionTouched=true;list->SetCheck(i,TRUE);}}},browserRows?14:25);
 auto uncheckAll=d.button(browserRows?"Uncheck All":"Uncheck all",browserRows?387:117,browserRows?250:300,browserRows?60:100,[list,&rows]{for(int i=0;i<list->GetItemCount();++i){rows[(int)list->GetItemData(i)].selectionTouched=true;list->SetCheck(i,FALSE);}},browserRows?14:25);
 if(!browserRows)d.button("Authorization...",232,300,133,[&]{Form auth("Batch authorization",432,147,&d);auth.init=[&]{auto existing=basicLogin(headers);auth.label("Login",13,17,82);auto user=auth.edit(existing.first,104,13,313);auth.label("Password",13,53,82);auto password=auth.edit(existing.second,104,49,313,23,false,false,true);auth.label("Default credentials; per-file overrides take precedence.",13,85,404);auth.accept=[&,user,password]{if(text(user).find(':')!=std::string::npos)throw std::runtime_error("Login cannot contain a colon.");auto plain=text(user)+":"+text(password);setHeader(headers,"Authorization",text(user).empty()&&text(password).empty()?"":"Basic "+b64(Bytes(plain.begin(),plain.end())));validateHeaders(headers);auth.close();};auth.button("OK",229,111,89,auth.accept);auth.button("Cancel",328,111,89,[&]{auth.close(IDCANCEL);});};if(auth.DoModal()==IDOK)refreshMetadata(std::nullopt);});
 auto editButton=d.button("Edit...",browserRows?465:382,browserRows?250:300,browserRows?60:94,[&,list]{
  if(list->GetSelectedCount()!=1)return;
  const int position=list->GetNextItem(-1,LVNI_SELECTED);
  if(position<0)return;
  const int index=(int)list->GetItemData(position);
  if(added.count(index))return;
  auto row=rows[index];
  Form edit("File Properties",browserRows?318:600,browserRows?246:516,&d);
  edit.dialogUnits=browserRows!=nullptr;
  const auto releaseIcon=[](HICON icon){DestroyIcon(icon);};
  std::unique_ptr<std::remove_pointer_t<HICON>,decltype(releaseIcon)> fileIcon(nullptr,releaseIcon);
  if(browserRows)edit.init=[&]{
   const auto displayPath=destinationText(index);
   const auto* source=browserContext(index);
   const auto sourcePage=source?source->downloadPage:std::string{};
   const auto displayDescription=source&&!row.descriptionEdited&&row.description.empty()&&d.checked(useDescriptions)?source->linkText:row.description;
   const auto initialRequest=requestHeaders(index);
   const auto initialLogin=basicLogin(initialRequest);
   const auto initialReferer=headerValue(initialRequest,"Referer");
   const auto fileType=fileTypes.find(row.browserExtension);
   if(fileType!=fileTypes.end()&&fileType->second.icon>=0&&fileImages.GetSafeHandle())
    fileIcon.reset(fileImages.ExtractIcon(fileType->second.icon));
   auto icon=edit.control(L"STATIC","",SS_ICON,7,7,20,20);
   const auto image=fileIcon?fileIcon.get():LoadIconW(nullptr,IDI_APPLICATION);
   if(image)icon->SendMessage(STM_SETICON,reinterpret_cast<WPARAM>(image));
   auto rule=[&edit](int y){edit.control(L"STATIC","",SS_ETCHEDHORZ,7,y,304,1);};
   edit.control(L"STATIC",displayName(index),SS_ENDELLIPSIS,58,13,253,9);rule(31);
   edit.label("Type:",7,36,44);auto typeValue=edit.control(L"STATIC","",SS_ENDELLIPSIS,58,36,253,9);
   edit.label("Status:",7,48,43);auto statusValue=edit.control(L"STATIC","",SS_ENDELLIPSIS,58,48,253,9);
   edit.label("Size:",7,60,43);auto sizeValue=edit.control(L"STATIC","",SS_ENDELLIPSIS,58,60,253,9);
   edit.label("Save To:",7,77,45);auto destination=edit.edit(displayPath,58,74,197,14);
   edit.button("&Browse...",261,74,50,[&edit,destination]{
    const auto current=wide(text(destination));
    CFileDialog chooser(FALSE,nullptr,current.c_str(),OFN_PATHMUSTEXIST,L"All files|*.*||",&edit);
    if(chooser.DoModal()==IDOK)destination->SetWindowText(chooser.GetPathName());
   });
   edit.label("Address:",7,96,44);auto address=edit.edit(row.address,58,93,253,14);
   edit.label("Description:",7,115,47);auto description=edit.edit(displayDescription,58,112,253,14);description->SendMessage(EM_SETLIMITTEXT,4096);
   rule(131);edit.label("The web page from which this file was obtained:",7,136,304);
   if(sourcePage.empty())edit.label("Unknown",7,149,304);
   else edit.staticLink(sourcePage,7,149,304,[&edit,sourcePage]{openFile(&edit,fs::path(wide(sourcePage)));});
   rule(161);edit.label("Referer:",7,171,47);auto referer=edit.edit(initialReferer,58,167,253,14);
   edit.control(L"STATIC","Login",SS_RIGHT,23,188,50,9);auto user=edit.edit(initialLogin.first,80,185,126,13);
   edit.control(L"STATIC","Password",SS_RIGHT,21,205,52,9);auto password=edit.edit(initialLogin.second,80,203,126,13,false,false,true);
   rule(220);
   edit.pulse=[&,address,referer,user,password,typeValue,statusValue,sizeValue,initialReferer,initialLogin]{
    const bool pending=trim(text(address))!=row.address||trim(text(referer))!=initialReferer||text(user)!=initialLogin.first||text(password)!=initialLogin.second;
    const auto& details=rows[index].metadata;
    const auto state=str(details,"Status"),type=typeText(index);
    const auto size=num(details,"Size",-1);
    typeValue->SetWindowText(cs(pending||type.empty()?"Unknown":type));
    sizeValue->SetWindowText(cs(!pending&&state=="Ready"&&size>=0?bytes(size)+" ("+std::to_string(size)+" Bytes)":"Unknown"));
    statusValue->SetWindowText(cs(pending?"Save request changes to refresh details.":state=="Ready"?"Not started":details.empty()||state=="Checking"?"Checking details...":"Details unavailable"));
   };edit.pulse();
   edit.accept=[&,address,destination,description,referer,user,password,displayPath,displayDescription,initialReferer,initialLogin]{
    auto next=rows[index];
    const Url requested(trim(text(address))),previous(row.address);
    next.address=requested.full;
    const auto newPath=trim(text(destination));
    if(newPath!=displayPath){
     if(newPath.empty()){next.folder.clear();next.name.clear();}
     else{
      const auto path=fs::path(wide(newPath)),initialPath=fs::path(wide(displayPath));
      const auto name=utf8(path.filename().wstring());
      if(!path.is_absolute()||name.empty()||safeName(name)!=name)
       throw std::runtime_error("Enter an absolute destination with a valid file name.");
      if(path.parent_path()!=initialPath.parent_path())next.folder=utf8(path.parent_path().wstring());
      if(path.filename()!=initialPath.filename())next.name=name;
     }
    }
    const auto newDescription=text(description);
    if(newDescription!=displayDescription){next.description=newDescription;next.descriptionEdited=true;}
    const auto original=readHeaders(Json{{"ProtectedHeaders",row.protectedRequestOverrides}});
    const bool sameOrigin=requested.origin==previous.origin;
    auto overrides=sameOrigin?original:Headers{};
    const auto newReferer=trim(text(referer)),newUser=text(user),newPassword=text(password);
    if(newReferer!=initialReferer){
     if(!newReferer.empty()){const Url page(newReferer);if(page.scheme!="http"&&page.scheme!="https")throw std::runtime_error("Enter an HTTP or HTTPS Referer.");}
     setHeader(overrides,"Referer",newReferer);
     if(newReferer.empty())overrides["Referer"]="";
    }
    if(newUser!=initialLogin.first||newPassword!=initialLogin.second){
     const auto passwordForAddress=!sameOrigin&&newPassword==initialLogin.second?std::string{}:newPassword;
     setBasicLogin(overrides,newUser,passwordForAddress,!newUser.empty()||!passwordForAddress.empty());
    }
    validateHeaders(overrides);
    if(overrides!=original)next.protectedRequestOverrides=overrides.empty()?"":protect(legacyDictionary(Json(overrides)).dump());
    rows[index]=std::move(next);
    edit.close();
   };
   edit.button("Open",172,227,50,[]{})->EnableWindow(FALSE);
   edit.defaultButton(edit.button("OK",261,227,50,edit.accept));
  };else edit.init=[&]{
  edit.label("Type",13,13,92);auto typeValue=edit.label("Unknown",110,13,476);edit.label("Status",13,37,92);auto statusValue=edit.label("Checking details...",110,37,476);edit.label("Size",13,61,92);auto sizeValue=edit.label("Unknown",110,61,476);
  edit.label("Address",13,95,92);auto address=edit.edit(row.address,110,91,476);
  edit.label("File name",13,133,92);auto name=edit.edit(row.name,110,129,476);edit.label("Leave blank to use the batch pattern or server file name.",110,158,476);
  edit.label("Save to",13,191,92);auto destination=edit.edit(row.folder,110,187,365);edit.button("Browse...",485,186,101,[&,destination]{auto value=chooseFolder(&edit,wide(text(destination)));if(!value.empty())destination->SetWindowText(value.c_str());});
  edit.label("Leave blank to use the import folder or category default.",110,218,476);
  edit.label("Description",13,255,92);auto description=edit.edit(row.description,110,251,476,65,false,true);description->SendMessage(EM_SETLIMITTEXT,4096);
  auto separate=edit.check("Use separate login and Referer for this file",!row.protectedRequestOverrides.empty(),110,326,476);const auto initialRequest=requestHeaders(index);const auto initialLogin=basicLogin(initialRequest);
  edit.label("Referer",13,364,92);auto referer=edit.edit(headerValue(initialRequest,"Referer"),110,360,476);
  edit.label("Login",13,402,92);auto user=edit.edit(initialLogin.first,110,398,476);
  edit.label("Password",13,440,92);auto password=edit.edit(initialLogin.second,110,436,476,23,false,false,true);
  auto toggleRequest=[&edit,separate,referer,user,password]{const auto enabled=edit.checked(separate);for(auto field:{referer,user,password})field->EnableWindow(enabled);};edit.bind(separate,toggleRequest);toggleRequest();
  edit.pulse=[&,address,separate,referer,user,password,typeValue,statusValue,sizeValue,initialLogin,initialRequest]{
   const bool pending=trim(text(address))!=rows[index].address||edit.checked(separate)!=!row.protectedRequestOverrides.empty()||(edit.checked(separate)&&(text(user)!=initialLogin.first||text(password)!=initialLogin.second||trim(text(referer))!=headerValue(initialRequest,"Referer")));
   const auto& details=rows[index].metadata;const auto state=str(details,"Status"),mime=str(details,"ContentType");const auto size=num(details,"Size",-1);
   typeValue->SetWindowText(cs(!pending&&!mime.empty()?mime:"Unknown"));sizeValue->SetWindowText(cs(!pending&&state=="Ready"&&size>=0?std::to_string(size)+" bytes":"Unknown"));statusValue->SetWindowText(cs(pending?"Save request changes to refresh details.":state=="Ready"?"Not started":details.empty()||state=="Checking"?"Checking details...":"Details unavailable"));
  };edit.pulse();
  edit.accept=[&,address,name,destination,description,separate,referer,user,password]{auto next=rows[index];next.address=trim(text(address));Url valid(next.address);next.name=trim(text(name));next.folder=trim(text(destination));next.description=text(description);next.protectedRequestOverrides.clear();if(edit.checked(separate)){Headers overrides;setBasicLogin(overrides,text(user),text(password),!text(user).empty()||!text(password).empty());overrides["Referer"]=trim(text(referer));validateHeaders(overrides);next.protectedRequestOverrides=!rows[index].protectedRequestOverrides.empty()&&readHeaders(Json{{"ProtectedHeaders",rows[index].protectedRequestOverrides}})==overrides?rows[index].protectedRequestOverrides:protect(legacyDictionary(Json(overrides)).dump());}if(next.name.empty()&&!sequenceTokens.empty())batchOutputName(renamePattern,sequenceTokens[index],batchServerExtension(next.metadata));if(!next.name.empty()&&safeName(next.name)!=next.name)throw std::runtime_error("Enter a valid file name without folders.");if(!next.folder.empty()&&!fs::path(wide(next.folder)).is_absolute())throw std::runtime_error("Choose an absolute destination folder, or leave it blank.");rows[index]=std::move(next);edit.close();};edit.defaultButton(edit.button("OK",389,474,94,edit.accept));edit.button("Cancel",493,474,94,[&]{edit.close(IDCANCEL);});
 };if(edit.DoModal()==IDOK){if(row.address!=rows[index].address||row.protectedRequestOverrides!=rows[index].protectedRequestOverrides)refreshMetadata(browserRows?std::optional<int>{index}:std::nullopt);else refreshBrowserName(index);for(int at=0;at<list->GetItemCount();++at)if((int)list->GetItemData(at)==index){list->SetItemText(at,0,cs(displayName(index)));list->SetItemText(at,1,cs(rows[index].address));list->SetItemText(at,4,cs(destinationText(index)));list->SetItemText(at,5,cs(columnText(index)));break;}if(browserRows||hideHtml||hideRepeated)rebuildList();}},browserRows?14:25);
 d.highlightChanged=[&,list,editButton]{const int position=list->GetNextItem(-1,LVNI_SELECTED);editButton->EnableWindow(list->GetSelectedCount()==1&&position>=0&&!added.count((int)list->GetItemData(position)));};d.highlightChanged();
 if(browserRows){
  useDescriptions=d.check("Use link texts as download descriptions",true,312,268,209,10);
  htmlFilter=d.check("Hide HTML files",false,312,279,169,10);
  auto images=d.check("Hide images located on this web page",true,312,289,213,10);
  duplicateFilter=d.check("Hide repeated files",hideRepeated,312,300,213,10);
  for(auto control:{checkAll,uncheckAll,editButton,useDescriptions,htmlFilter,images,duplicateFilter})
   d.anchorControl(control,ResizeAnchor::BottomRight);
  d.bind(images,[&,images]{hideImages=d.checked(images);rebuildList();});
 }
 d.bind(htmlFilter,[&,htmlFilter]{hideHtml=d.checked(htmlFilter);rebuildList();});
 d.bind(duplicateFilter,[&,duplicateFilter]{hideRepeated=d.checked(duplicateFilter);rebuildList();});
 CWnd *report=nullptr,*paused=nullptr;
 QueueCombo* queue=nullptr;
 if(!browserRows){
  report=d.label("",489,306,188);
  d.label("Queue",13,344,54);
  queue=d.queueCombo(manager,queueNames(manager),"Main queue",74,340,250);
  paused=d.check("Download later",true,345,340,260);
 }

 enum class SaveMode {CategoryFolders,OneCategory,OneFolder};
 ThemeCombo* mode=nullptr;
 CWnd *ownCategory=nullptr,*categoryRadio=nullptr,*folderRadio=nullptr;
 Json prefs;
 if(browserRows){
  {Lock lock(manager.mutex);prefs=manager.state["Settings"];}
  auto saveTo=d.control(L"BUTTON","Save To",BS_GROUPBOX,7,248,273,81);
  ownCategory=d.check("Every file to the directory according to the category of the file",true,12,258,262,10);
  categoryRadio=d.check("All files to one &category",false,12,269,262,10);
  folderRadio=d.check("All files to one &directory",false,12,297,262,10);
  ownCategory->ModifyStyle(BS_TYPEMASK|BS_MULTILINE,BS_AUTORADIOBUTTON|WS_GROUP);
  for(auto radio:{categoryRadio,folderRadio})radio->ModifyStyle(BS_TYPEMASK|BS_MULTILINE|WS_TABSTOP,BS_AUTORADIOBUTTON);
  for(auto control:{saveTo,ownCategory,categoryRadio,folderRadio})d.anchorControl(control,ResizeAnchor::BottomLeft);
 }else{
  d.label("Save mode",13,380,87);
  mode=d.combo({"Each file's category folder","All files to one category","All files to one folder"},"Each file's category folder",110,376,264);
  d.label("Category",389,380,61);d.label("Save to",13,420,87);
 }
 const auto categoryNames=manager.categories();
 auto category=d.combo(categoryNames,categoryNames.empty()?"":categoryNames.front(),browserRows?23:455,browserRows?281:376,browserRows?126:222);
 if(browserRows)category->ModifyStyle(0,WS_GROUP);
 auto folder=d.edit(browserRows?str(prefs,"DownloadFolder"):"",browserRows?23:110,browserRows?308:416,browserRows?193:458);
 auto browse=d.button("Browse...",browserRows?220:578,browserRows?308:415,browserRows?55:99,[&,folder]{
  auto value=chooseFolder(&d,wide(text(folder)));if(!value.empty())folder->SetWindowText(value.c_str());
 },browserRows?14:25);
 if(browserRows){
  d.anchorControl(category,ResizeAnchor::BottomLeft);
  d.anchorControl(folder,ResizeAnchor::BottomLeft);
  d.anchorControl(browse,ResizeAnchor::BottomLeft);
 }
 auto readSaveMode=[&d,mode,categoryRadio,folderRadio]{
  if(mode){
   const auto value=text(mode);
   if(value=="All files to one category")return SaveMode::OneCategory;
   if(value=="All files to one folder")return SaveMode::OneFolder;
  }else{
   if(d.checked(categoryRadio))return SaveMode::OneCategory;
   if(d.checked(folderRadio))return SaveMode::OneFolder;
  }
  return SaveMode::CategoryFolders;
 };
 if(browserRows){
  destinationText=[&,readSaveMode,category,folder,prefs=std::move(prefs)](int index){
   const auto& row=rows[index];
   auto destination=row.folder;
   if(destination.empty()){
    const auto saveMode=readSaveMode();
    if(saveMode==SaveMode::OneFolder){
     destination=trim(text(folder));
     if(!fs::path(wide(destination)).is_absolute())return std::string{};
    }else{
     const Url address(row.address);
     const auto categoryName=saveMode==SaveMode::OneCategory?text(category):downloadCategory(displayName(index),address.host,prefs);
     destination=categoryFolder(prefs,categoryName);
    }
   }
   return utf8((fs::path(wide(destination))/wide(displayName(index))).wstring());
  };
 }
 auto updateDestinations=[&,list]{
  if(!browserRows)return;
  for(int position=0;position<list->GetItemCount();++position)
   list->SetItemText(position,4,cs(destinationText((int)list->GetItemData(position))));
  if(sortedColumn==4)d.sortColumn(4,false);
 };
 auto modeChanged=[readSaveMode,category,folder,browse,updateDestinations]{
  const auto value=readSaveMode();
  category->EnableWindow(value==SaveMode::OneCategory);
  folder->EnableWindow(value==SaveMode::OneFolder);browse->EnableWindow(value==SaveMode::OneFolder);
  updateDestinations();
 };
 if(mode)d.bindChange(mode,modeChanged);
 else{
  for(auto radio:{ownCategory,categoryRadio,folderRadio})d.bind(radio,modeChanged);
  d.bindChange(category,updateDestinations);d.bindChange(folder,updateDestinations);rebuildList();
 }
 modeChanged();
 if(!sequenceTokens.empty()){
  d.label("File name pattern",13,460,132);auto pattern=d.edit("",150,456,527);patternControl=pattern;pattern->SendMessage(EM_SETLIMITTEXT,240);
  auto patternStatus=d.label("Use * for the sequence. Server extensions apply; per-file names override the pattern.",13,489,664);
  d.bindChange(pattern,[&,list,pattern,patternStatus]{renamePattern=trim(text(pattern));if(!patternExtensionAdjusted&&!firstServerExtension.empty()&&!renamePattern.empty()){patternExtensionAdjusted=true;auto adjusted=batchExtensionPattern(renamePattern,firstServerExtension);if(adjusted!=renamePattern){pattern->SetWindowText(cs(adjusted));return;}}try{std::vector<std::string> names;for(int position=0;position<list->GetItemCount();++position){int index=(int)list->GetItemData(position);auto name=added.count(index)?rows[index].name:displayName(index);names.push_back(name.empty()?safeName(unescape(Url(rows[index].address).path)):name);}for(int position=0;position<list->GetItemCount();++position)list->SetItemText(position,0,cs(names[position]));patternStatus->SetWindowText(L"Use * for the sequence. Server extensions apply; per-file names override the pattern.");}catch(const std::exception& e){patternStatus->SetWindowText(cs(e.what()));}});
 }
 auto updateFileTypes=[&,list]{
  if(!fileDetails)return false;
  const int pixels=std::max(1,MulDiv(16,GetDpiForWindow(d.GetSafeHwnd()),96));
  if(pixels!=imagePixels){
   CImageList next;
   if(next.Create(pixels,pixels,ILC_COLOR32|ILC_MASK,1,8)){
    list->SetImageList(&next,LVSIL_SMALL);
    fileImages.DeleteImageList();fileImages.Attach(next.Detach());
    imagePixels=pixels;fileTypes.clear();fileDetails->invalidate();
    for(int row=0;row<list->GetItemCount();++row){
     LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=row;item.iImage=I_IMAGENONE;list->SetItem(&item);
    }
   }
  }
  bool typeChanged=false;
  for(auto& result:fileDetails->take()){
   auto& type=fileTypes[result.extension];
   if(!result.typeName.empty()){type.name=std::move(result.typeName);typeChanged=true;}
   if(result.icon&&fileImages.GetSafeHandle()){
    const auto image=fileImages.Add(result.icon);if(image>=0)type.icon=image;
   }
  }
  if(sortedColumn==3)for(const auto& row:rows)
   if(!fileTypes.count(row.browserExtension))fileDetails->request(row.browserExtension);
  const int top=std::max(0,list->GetTopIndex());
  const int end=std::min(list->GetItemCount(),top+std::max(1,list->GetCountPerPage())+1);
  for(int position=top;position<end;++position){
   const int index=(int)list->GetItemData(position);
   const auto& extension=rows[index].browserExtension;
   const auto type=fileTypes.find(extension);
   if(type==fileTypes.end())fileDetails->request(extension);
   const int image=type==fileTypes.end()?I_IMAGENONE:type->second.icon;
   LVITEMW item{};item.mask=LVIF_IMAGE;item.iItem=position;
   if(list->GetItem(&item)&&item.iImage!=image){item.iImage=image;list->SetItem(&item);}
   const auto label=cs(typeText(index));
   if(list->GetItemText(position,3)!=label)list->SetItemText(position,3,label);
  }
  return typeChanged;
 };
 d.pulse=[&,list,report,editButton,updateFileTypes]{
  bool metadataChanged=false;
  if(metadata)for(auto& completed:metadata->take()){
   const auto index=completed.first;
   if(index>=rows.size()||added.count((int)index))continue;
   auto& row=rows[index];row.metadata=std::move(completed.second);refreshBrowserName((int)index);metadataChanged=true;
   if(!row.selectionTouched&&str(row.metadata,"Status")=="Ready"){
    const auto extension=lower(batchServerExtension(row.metadata)),type=lower(str(row.metadata,"ContentType"));
    row.checked=extension!=".htm"&&extension!=".html"&&type!="text/html"&&type!="application/xhtml+xml";
   }
   if(firstServerExtension.empty()){
    auto extension=batchServerExtension(row.metadata);
    if(!extension.empty()&&lower(extension)!=".htm"&&lower(extension)!=".html")firstServerExtension=std::move(extension);
   }
   if(patternControl&&!patternExtensionAdjusted&&!firstServerExtension.empty()&&!renamePattern.empty()){
    patternExtensionAdjusted=true;auto adjusted=batchExtensionPattern(renamePattern,firstServerExtension);
    if(adjusted!=renamePattern)patternControl->SetWindowText(cs(adjusted));
   }
   for(int position=0;position<list->GetItemCount();++position)if(list->GetItemData(position)==index){
    d.changingChecks=true;list->SetCheck(position,row.checked);d.changingChecks=false;
    list->SetItemText(position,2,cs(sizeText((int)index)));list->SetItemText(position,3,cs(typeText((int)index)));
    try{
     list->SetItemText(position,0,cs(displayName((int)index)));
     if(browserRows)list->SetItemText(position,4,cs(destinationText((int)index)));
    }catch(...){}
    break;
   }
  }
  if(hideHtml&&metadataChanged)rebuildList();
  const bool typeChanged=updateFileTypes();
  if(browserRows&&((typeChanged&&sortedColumn==3)||
     (metadataChanged&&(sortedColumn==0||sortedColumn==2||sortedColumn==3||sortedColumn==4))))
   d.sortColumn(sortedColumn,false);
  snapshotChecks();int visibleCount=0;
  for(int i=0;i<list->GetItemCount();++i)if(list->GetCheck(i)&&!added.count((int)list->GetItemData(i)))++visibleCount;
  int count=browserRows?visibleCount:0;
  if(!browserRows)for(int i=0;i<(int)rows.size();++i)if(rows[i].checked&&!added.count(i))++count;
  if(confirm)confirm->EnableWindow(count>0);
  const int index=list->GetNextItem(-1,LVNI_SELECTED);
  editButton->EnableWindow(list->GetSelectedCount()==1&&index>=0&&!added.count((int)list->GetItemData(index)));
  if(report)report->SetWindowText(cs(std::to_string(count)+" selected"+(count>visibleCount?" ("+std::to_string(count-visibleCount)+" hidden)":"")));
 };
 d.accept=[&,list,queue,folder,paused,readSaveMode,category,useDescriptions]{
  snapshotChecks();
  std::vector<int> selected;
  std::set<int> visible;
  for(int position=0;position<list->GetItemCount();++position){
   const int index=(int)list->GetItemData(position);if(!browserRows)visible.insert(index);
   if(rows[index].checked&&!added.count(index))selected.push_back(index);
  }
  if(!browserRows)for(int index=0;index<(int)rows.size();++index)
   if(!visible.count(index)&&rows[index].checked&&!added.count(index))selected.push_back(index);
  if(selected.empty())throw std::runtime_error("Select at least one file.");
  const auto saveMode=readSaveMode();
  const auto forcedCategory=saveMode==SaveMode::OneCategory?text(category):"";
  const auto dest=saveMode==SaveMode::OneFolder?trim(text(folder)):"";
  if(saveMode==SaveMode::OneFolder&&!fs::path(wide(dest)).is_absolute())
   throw std::runtime_error("Choose an absolute destination folder.");
  std::vector<BatchDownloadEntry> entries;
  entries.reserve(selected.size());
  for(int index:selected){
   const auto& row=rows[index];
   BatchDownloadEntry entry{row.address,row.folder.empty()?dest:row.folder,browserRows?displayName(index):outputName(index),row.description,requestHeaders(index)};
   if(const auto* source=browserContext(index)){
    entry.browserProxy=source->proxy;entry.browserSession=source->session;entry.downloadPage=source->downloadPage;
    if(entry.description.empty()&&!row.descriptionEdited&&useDescriptions&&d.checked(useDescriptions))entry.description=source->linkText;
   }
   entries.push_back(std::move(entry));
  }
  bool ask=false;
  const bool queueOnly=browserRows||d.checked(paused);
  std::string queueName;
  {
   Lock lock(manager.mutex);
   if(browserRows)queueName=primaryDownloadQueueName(manager.state["Queues"]);
   else ask=yes(manager.state["Settings"],"QueuePromptBatch",true);
  }
  if(!browserRows)queueName=queue->queueName();
  if(queueName.empty())throw std::runtime_error("Choose or create a download queue in Scheduler.");
  std::optional<DownloadQueueChoice> choice;
  if(ask){choice=chooseDownloadQueue(&d,manager,queueName,!queueOnly);if(!choice)return;}
  const auto admitted=manager.offerDownloadBatch(entries,choice?choice->name:queueName,choice||queueOnly,forcedCategory,choice?&*choice:nullptr);
  for(int index:selected)rows[index].checked=false;
  if(choice&&choice->start)try{manager.startQueueForDownloads(choice->name,admitted);}
  catch(const std::exception& e){error(&d,std::runtime_error("The batch was saved, but queue processing could not start. "+std::string(e.what())));}
  d.close();
 };const int footer=browserRows?315:sequenceTokens.empty()?462:522;
 confirm=d.button("OK",browserRows?412:478,footer,browserRows?50:94,d.accept,browserRows?14:25);
 auto cancelButton=d.button("Cancel",browserRows?475:583,footer,browserRows?50:94,[&]{d.close(IDCANCEL);},browserRows?14:25);
 if(browserRows){
  d.anchorControl(confirm,ResizeAnchor::BottomRight);
  d.anchorControl(cancelButton,ResizeAnchor::BottomRight);
  d.defaultButton(confirm);d.pulse();showBrowserOwnedDialog(d,parent);
 }
 refreshMetadata(std::nullopt);
 };const auto result=d.DoModal();if(result==-1&&browserRows)throw std::runtime_error("Cannot open the browser batch dialog.");return result==IDOK;
}
// Expands IDM-style "img*.jpg" patterns: every asterisk becomes the same zero-padded number or letter. Without an asterisk the bracket syntax of expand() still applies.
inline std::vector<std::string> batchPattern(const std::string& address,bool letters,const std::string& from,const std::string& to,int wildcardSize,std::vector<std::string>* sequenceTokens=nullptr){
 if(sequenceTokens)sequenceTokens->clear();
 const auto star=address.find('*');if(star==std::string::npos)return expand(address);
 std::vector<std::string> urls;auto append=[&](const std::string& token){std::string generated;for(char character:address){if(character=='*')generated+=token;else generated+=character;}urls.push_back(std::move(generated));if(sequenceTokens)sequenceTokens->push_back(token);};
 if(letters){
  if(from.size()!=1||to.size()!=1||!std::isalpha((unsigned char)from[0])||!std::isalpha((unsigned char)to[0])||(std::isupper((unsigned char)from[0])!=0)!=(std::isupper((unsigned char)to[0])!=0))throw std::runtime_error("Use single letters of the same case for From and To.");
  const int step=to[0]>=from[0]?1:-1;
  for(int value=from[0];;value+=step){auto token=std::string(1,(char)value);append(token);if(value==to[0])break;}
 }else{
  if(!std::regex_match(from,std::regex("[0-9]{1,6}"))||!std::regex_match(to,std::regex("[0-9]{1,6}")))throw std::runtime_error("Enter whole numbers from 0 to 999999 for From and To.");
  const long long first=std::stoll(from),last=std::stoll(to),distance=last>=first?last-first:first-last,step=last>=first?1:-1;if(distance>=1000)throw std::runtime_error("A batch can contain at most 1,000 URLs.");
  for(auto value=first;;value+=step){std::ostringstream formatted;formatted<<std::setfill('0')<<std::setw(std::clamp(wildcardSize,1,10))<<value;auto token=formatted.str();append(token);if(value==last)break;}
 }
 return urls;
}
inline void selectExpandedBatch(CWnd* parent,Manager& manager,const std::string& addresses){
 auto urls=expand(addresses);
 if(!urls.empty())selectBatch(parent,manager,urls);
}
inline void batchDialog(CWnd* parent,Manager& manager,const std::string& initial=""){
 // IDM "Batch download" layout: one address with an asterisk, replaced by a number or letter range; live first/second/last previews.
 Form d("Batch download",316,238,parent);d.dialogUnits=true;
 d.init=[&]{
  d.label("It's possible to add a group of sequential file names like img001.jpg, img002.jpg, etc., img100.jpg to UDM download queue. Use the asterisk wildcard for the file name pattern. \r\nFor example:   http://www.example.com/pictures/img*.jpg",7,4,301,37);
  d.label("Address",7,47,30);std::vector<std::string> history;{Lock lock(manager.mutex);for(auto it=manager.jobs.rbegin();it!=manager.jobs.rend()&&history.size()<25;++it){auto value=str((*it)->data,"Url");if(std::find(history.begin(),history.end(),value)==history.end())history.push_back(value);}}
  auto address=d.combo(history,initial,39,44,269,true);address->SetWindowText(cs(initial));
  d.control(L"BUTTON","Replace asterisk to",BS_GROUPBOX,7,64,301,55);
  auto numbers=d.check("&Numbers",true,15,79,66);auto letters=d.check("&Letters",false,92,79,65);for(auto item:{numbers,letters})item->ModifyStyle(BS_TYPEMASK,BS_AUTORADIOBUTTON);
  d.label("From:",11,100,33);auto from=d.edit("0",53,95,23,16);auto fromSpin=d.make<ThemeSpin>(UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_NOTHOUSANDS,75,95,12,16);fromSpin->SetBuddy(from);fromSpin->SetRange32(0,999999);fromSpin->SetPos32(0);
  d.label("To:",106,100,22);auto to=d.edit("100",136,95,23,16);auto toSpin=d.make<ThemeSpin>(UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_NOTHOUSANDS,158,95,12,16);toSpin->SetBuddy(to);toSpin->SetRange32(0,999999);toSpin->SetPos32(100);
  d.label("Wildcard size",202,100,61);auto size=d.edit("2",267,95,19,16);auto sizeSpin=d.make<ThemeSpin>(UDS_SETBUDDYINT|UDS_ARROWKEYS|UDS_NOTHOUSANDS,285,95,12,16);sizeSpin->SetBuddy(size);sizeSpin->SetRange32(1,10);sizeSpin->SetPos32(2);
  d.control(L"BUTTON","",BS_GROUPBOX,7,126,301,29);auto auth=d.check("Use &authorization",false,13,126,70,8);
  auto loginLabel=d.label("Login",13,139,60);auto user=d.edit("",77,136,78);
  auto passwordLabel=d.label("Password",159,139,64);auto password=d.edit("",227,136,75,14,false,false,true);
  auto toggleAuth=[&d,auth,loginLabel,user,passwordLabel,password]{
   const BOOL enabled=d.checked(auth);
   for(auto control:{loginLabel,user,passwordLabel,password})control->EnableWindow(enabled);
  };d.bind(auth,toggleAuth);toggleAuth();
  d.label("First file:",7,166,58);auto firstFile=d.control(L"STATIC","",SS_LEFTNOWORDWRAP|SS_NOPREFIX|SS_PATHELLIPSIS,68,165,240,11);
  d.label("Second file:",7,179,58);auto secondFile=d.control(L"STATIC","",SS_LEFTNOWORDWRAP|SS_NOPREFIX|SS_PATHELLIPSIS,68,178,240,11);
  d.label("...",15,189,8,8);d.label("Last file:",7,201,58);auto lastFile=d.control(L"STATIC","",SS_LEFTNOWORDWRAP|SS_NOPREFIX|SS_PATHELLIPSIS,68,199,240,11);
  auto wildcardSize=[size]{auto value=trim(text(size));return std::regex_match(value,std::regex("[0-9]{1,2}"))?std::clamp(std::stoi(value),1,10):1;};
  auto generate=[&d,address,numbers,from,to,wildcardSize]{return batchPattern(trim(text(address)),!d.checked(numbers),trim(text(from)),trim(text(to)),wildcardSize());};
  auto preview=[address,generate,firstFile,secondFile,lastFile]{
   std::vector<std::string> urls;const auto pattern=trim(text(address));
   if(pattern.find_first_of("*[")!=std::string::npos){
    try{urls=generate();}catch(const std::exception&){urls.clear();}
    if(urls.size()==1&&urls.front()==pattern)urls.clear();
   }
   firstFile->SetWindowText(cs(!urls.empty()?urls[0]:""));secondFile->SetWindowText(cs(urls.size()>1?urls[1]:""));lastFile->SetWindowText(cs(urls.size()>2?urls.back():""));
  };
  auto numeric=std::make_shared<std::pair<std::string,std::string>>("0","100"),alphabetic=std::make_shared<std::pair<std::string,std::string>>("a","z");
  auto lettersMode=std::make_shared<bool>(false);
  auto mode=[&d,numbers,letters,from,to,fromSpin,toSpin,size,sizeSpin,numeric,alphabetic,preview,lettersMode](bool useLetters){*lettersMode=useLetters;numbers->SendMessage(BM_SETCHECK,useLetters?BST_UNCHECKED:BST_CHECKED);letters->SendMessage(BM_SETCHECK,useLetters?BST_CHECKED:BST_UNCHECKED);auto& values=useLetters?*alphabetic:*numeric;from->SetWindowText(cs(values.first));to->SetWindowText(cs(values.second));for(auto item:{static_cast<CWnd*>(fromSpin),static_cast<CWnd*>(toSpin),size,static_cast<CWnd*>(sizeSpin)})item->EnableWindow(!useLetters);preview();};
  d.bind(numbers,[lettersMode,mode,alphabetic,from,to]{if(*lettersMode)*alphabetic={trim(text(from)),trim(text(to))};mode(false);});d.bind(letters,[lettersMode,mode,numeric,from,to]{if(!*lettersMode)*numeric={trim(text(from)),trim(text(to))};mode(true);});
  for(auto item:{static_cast<CWnd*>(address),from,to,size})d.bindChange(item,preview);
  d.accept=[&,address,numbers,from,to,wildcardSize,auth,user,password]{if(trim(text(address)).empty())throw std::runtime_error("Enter an address. Use the asterisk wildcard for the part that changes.");std::vector<std::string> sequenceTokens;auto urls=batchPattern(trim(text(address)),!d.checked(numbers),trim(text(from)),trim(text(to)),wildcardSize(),&sequenceTokens);if(urls.empty())throw std::runtime_error("The address pattern produces no files.");for(const auto& url:urls)Url valid(url);Headers headers;if(d.checked(auth))setBasicLogin(headers,text(user),text(password));if(selectBatch(&d,manager,urls,headers,sequenceTokens))d.close();};
  d.defaultButton(d.button("O&K",182,217,50,d.accept));d.button("&Cancel",246,217,50,[&]{d.close(IDCANCEL);});preview();address->SetFocus();address->SetEditSel(0,-1);
 };d.DoModal();
}
inline void serverRulesDialog(CWnd* owner,Json& prefs){
 Form d("Server connection limits",531,326,owner);auto rules=prefs.value("ServerConnections",Json::array());CListCtrl* list=nullptr;
 auto fill=[&](int selection){list->DeleteAllItems();for(int i=0;i<(int)rules.size();++i){list->InsertItem(i,cs(connectionRuleLabel(rules[i])));list->SetItemText(i,1,cs(std::to_string(num(rules[i],"Connections"))));}if(selection>=0)list->SetItemState(selection,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);};
 auto change=[&](int index){Json rule=index>=0?rules[index]:Json{{"Host",""},{"Connections",8}};Form e(index<0?"New server limit":"Edit server limit",401,151,&d);e.init=[&]{auto scope=e.combo({"All","http","https","ftp"},str(rule,"Scheme").empty()?"All":str(rule,"Scheme"),13,14,100);auto host=e.edit(str(rule,"Host"),127,14,260);e.label("Max. connections",13,57,109);auto count=e.combo({"1","2","4","8","16","24","32"},std::to_string(num(rule,"Connections",8)),127,53,113,true);e.label("Use * as a server wildcard.",13,88,374);e.accept=[&,host,count,scope]{auto h=lower(trim(text(host)));auto n=parseConnectionCount(text(count));auto scheme=text(scope)=="All"?"":text(scope);Json value={{"Host",h},{"Scheme",scheme},{"Connections",n}};validateConnectionRule(value);for(int i=0;i<(int)rules.size();++i)if(i!=index&&lower(str(rules[i],"Host"))==h&&str(rules[i],"Scheme")==scheme)throw std::runtime_error("This server already has a rule.");if(index<0){if(rules.size()>=100)throw std::runtime_error("Keep at most 100 server rules.");rules.push_back(value);}else rules[index]=value;e.close();};e.button("OK",199,113,88,e.accept);e.button("Cancel",299,113,88,[&]{e.close(IDCANCEL);});};if(e.DoModal()==IDOK)fill(index<0?(int)rules.size()-1:index);};
 d.init=[&]{d.label("New downloads use the most specific matching server rule.",13,13,505,29);list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL,13,48,393,231);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->InsertColumn(0,L"Server",LVCFMT_LEFT,d.rect(0,0,267,0).Width());list->InsertColumn(1,L"Connections",LVCFMT_LEFT,d.rect(0,0,98,0).Width());d.button("New...",419,49,98,[change]{change(-1);});auto editButton=d.button("Edit...",419,87,98,[&,change]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0)change(row);});auto deleteButton=d.button("Delete",419,125,98,[&]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0){rules.erase(row);fill(std::min(row,(int)rules.size()-1));}});d.accept=[&]{prefs["ServerConnections"]=rules;d.close();};d.button("OK",318,291,94,d.accept);d.button("Cancel",423,291,94,[&]{d.close(IDCANCEL);});d.pulse=[&,editButton,deleteButton]{bool selected=list->GetNextItem(-1,LVNI_SELECTED)>=0;editButton->EnableWindow(selected);deleteButton->EnableWindow(selected);};fill(rules.empty()?-1:0);d.pulse();};d.DoModal();
}
