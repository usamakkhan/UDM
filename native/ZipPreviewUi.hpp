#ifdef UDM_TOOLBAR_COMPONENT_TEST
inline std::function<void(CWnd&,const char*)> zipPreviewTestStage;
#endif
// Included inside udm after Form. The list is virtual, including large directories.
class ZipPreviewList:public CListCtrl {
 DECLARE_MESSAGE_MAP()
public:
 std::shared_ptr<const ZipListing> listing;
 afx_msg void itemText(NMHDR* header,LRESULT* result){
  *result=0;auto info=reinterpret_cast<NMLVDISPINFOW*>(header);auto& row=info->item;if(!(row.mask&LVIF_TEXT)||!row.pszText||row.cchTextMax<=0)return;std::string value;
  if(listing&&row.iItem>=0&&(size_t)row.iItem<listing->entries.size()){const auto& item=listing->entries[row.iItem];switch(row.iSubItem){case 0:value=item.name;break;case 1:value=bytes(item.size);break;case 2:value=bytes(item.compressed);break;case 3:value=item.encrypted?"Yes":"No";break;}}
  wcsncpy_s(row.pszText,(size_t)row.cchTextMax,wide(value).c_str(),_TRUNCATE);
 }
 afx_msg void findItem(NMHDR* header,LRESULT* result){
  *result=-1;const auto request=reinterpret_cast<NMLVFINDITEMW*>(header);const auto flags=request->lvfi.flags;
  if(!listing||!request->lvfi.psz||!flags||(flags&~(LVFI_STRING|LVFI_PARTIAL|LVFI_SUBSTRING|LVFI_WRAP)))return;
  const auto length=wcsnlen_s(request->lvfi.psz,65536);if(length>=65536)return;
  const bool prefix=(flags&(LVFI_PARTIAL|LVFI_SUBSTRING))!=0;const auto count=listing->entries.size();
  const auto start=std::min(count,static_cast<size_t>(std::max(0,request->iStart)));
  try{auto match=[&](size_t index){const auto name=wide(listing->entries[index].name);if((prefix&&name.size()<length)||(!prefix&&name.size()!=length))return false;if(!length)return prefix||name.empty();return CompareStringOrdinal(name.data(),static_cast<int>(length),request->lvfi.psz,static_cast<int>(length),TRUE)==CSTR_EQUAL;};
   for(size_t index=start;index<count;++index)if(match(index)){*result=static_cast<LRESULT>(index);return;}
   if(flags&LVFI_WRAP)for(size_t index=0;index<start;++index)if(match(index)){*result=static_cast<LRESULT>(index);return;}
  }catch(const std::exception&){}
 }
};
BEGIN_MESSAGE_MAP(ZipPreviewList,CListCtrl)
 ON_NOTIFY_REFLECT(LVN_GETDISPINFO,itemText)
 ON_NOTIFY_REFLECT(LVN_ODFINDITEM,findItem)
END_MESSAGE_MAP()
inline void localZipDialog(CWnd* parent,fs::path path,const std::string& name){
 Form dialog("ZIP contents — "+name,714,442,parent);std::unique_ptr<LocalZipPreviewTask> task;bool displayed=false;
 dialog.init=[&]{
  auto status=dialog.label("Reading ZIP directory...",13,13,688,30);
  auto list=dialog.make<ZipPreviewList>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_OWNERDATA|LVS_SHOWSELALWAYS,13,49,688,337);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
  list->InsertColumn(0,L"Name",LVCFMT_LEFT,dialog.pixelsX(390));list->InsertColumn(1,L"Size",LVCFMT_RIGHT,dialog.pixelsX(105));list->InsertColumn(2,L"Packed",LVCFMT_RIGHT,dialog.pixelsX(105));list->InsertColumn(3,L"Encrypted",LVCFMT_LEFT,dialog.pixelsX(82));
  auto close=dialog.button("Cancel",596,401,105,[&]{if(task)task->stop();dialog.close();});dialog.defaultButton(close);dialog.cancel=[&]{if(task)task->stop();dialog.close(IDCANCEL);};dialog.accept=dialog.cancel;
  task=std::make_unique<LocalZipPreviewTask>(std::move(path),dialog.taskNotifier());
  dialog.pulse=[&,list,status,close]{if(displayed)return;const auto current=task->snapshot();if(current.status=="Checking")return;displayed=true;close->SetWindowText(L"Close");
   if(current.listing){list->listing=current.listing;list->SetItemCountEx(static_cast<int>(current.listing->entries.size()),LVSICF_NOSCROLL);status->SetWindowText(cs(std::to_string(current.listing->entries.size())+" entries; "+bytes(current.listing->received)+" read. Nothing extracted."));}
   else status->SetWindowText(cs(current.message.empty()?"ZIP preview cancelled.":current.message));
  };
  dialog.pulse();
 };
 dialog.DoModal();if(task)task->stop();
}
inline FileInfoZipOutcome fileInfoZipDialog(CWnd* parent,Json data,Json prefs,std::function<void(const Json&)> save,
 std::function<std::optional<fs::path>()> prefetched={},FileInfoZipTask* preparation=nullptr){
 Form dialog("Zip preview",386,222,parent);dialog.dialogUnits=true;std::shared_ptr<ZipPreviewTask> task;FileInfoZipOutcome outcome;bool closing=false;std::shared_ptr<PrefetchedZipReadTask> waiter;
 std::unique_ptr<LocalZipPreviewTask> local;
 const auto stop=[&]{if(preparation)preparation->cancel();if(waiter)waiter->cancel();if(local)local->stop();if(task)task->stop();};
 dialog.init=[&]{
  auto name=dialog.control(L"STATIC",str(data,"FileName"),SS_CENTER|SS_ENDELLIPSIS,7,3,372,10);
  auto list=dialog.make<ZipPreviewList>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_OWNERDATA|LVS_SHOWSELALWAYS,7,16,372,160);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->SetBkColor(uiBackground());list->SetTextBkColor(uiBackground());list->SetTextColor(uiForeground());
  list->InsertColumn(0,L"Name",LVCFMT_LEFT,dialog.pixelsX(190));list->InsertColumn(1,L"Size",LVCFMT_RIGHT,dialog.pixelsX(62));list->InsertColumn(2,L"Packed",LVCFMT_RIGHT,dialog.pixelsX(62));list->InsertColumn(3,L"Encrypted",LVCFMT_LEFT,dialog.pixelsX(50));
  auto status=dialog.label(preparation?"Preparing ZIP preview...":prefetched?"Waiting for the background download to finish...":"Reading ZIP directory...",7,179,372,22);
  auto close=dialog.button("Cancel",167,204,50,[]{});dialog.defaultButton(close);
  const auto requestClose=[&,close,status]{stop();if((preparation&&preparation->valid())||(waiter&&!waiter->poll())){closing=true;close->EnableWindow(FALSE);status->SetWindowText(L"Closing preview; waiting for background preparation...");return;}dialog.close(IDCANCEL);};
  dialog.bind(close,requestClose);dialog.cancel=requestClose;dialog.accept=requestClose;
  if(preparation)preparation->start(dialog.taskNotifier()
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   ,fileInfoZipTestObserver
#endif
  );
  else if(!prefetched)task=std::make_shared<ZipPreviewTask>(data,prefs,std::move(save),30000,dialog.taskNotifier());
  dialog.pulse=[&,list,status,close,name,displayed=false]() mutable{
   if(preparation&&preparation->valid()){
    if(!preparation->ready())return;
    auto result=preparation->take();outcome=std::move(result.outcome);task=std::move(result.remote);waiter=std::move(result.reader);if(!result.name.empty())name->SetWindowText(cs(result.name));
    if(waiter){prefetched=[waiter]{return waiter->poll();};status->SetWindowText(L"Waiting for the background download to finish...");}
    if(closing){stop();}
    else if(result.cancelled||!result.message.empty()){displayed=true;close->SetWindowText(L"OK");status->SetWindowText(cs(result.message.empty()?"ZIP preview cancelled.":result.message));return;}
    else if(task)status->SetWindowText(L"Reading ZIP directory...");
   }
   if(closing){if(waiter&&!waiter->poll())return;dialog.close(IDCANCEL);return;}
   if(displayed)return;
   if(!task&&!local){
    const auto ready=prefetched();
    if(!ready)return;
    if(ready->empty()){
     displayed=true;close->SetWindowText(L"OK");
     status->SetWindowText(L"The background download did not leave a complete ZIP to preview.");
     return;
    }
    local=std::make_unique<LocalZipPreviewTask>(*ready,dialog.taskNotifier());
    status->SetWindowText(L"Reading ZIP directory...");
   }
   auto state=local?local->snapshot():task->snapshot();
   if(state.status=="Checking")return;
   displayed=true;close->SetWindowText(L"OK");
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(zipPreviewTestStage)zipPreviewTestStage(dialog,"ready");
#endif
   if(state.listing){list->listing=state.listing;list->SetItemCountEx((int)state.listing->entries.size(),LVSICF_NOSCROLL);status->SetWindowText(cs(std::to_string(state.listing->entries.size())+" entries; "+bytes(state.listing->received)+" read. Nothing extracted."+(state.listing->validated?"":" No strong server validator; listing is informational.")));}else status->SetWindowText(cs(state.message.empty()?"ZIP preview cancelled.":state.message));
  };
  dialog.pulse();
 };
 dialog.DoModal();stop();return outcome;
}

inline FileInfoZipOutcome fileInfoPreparedZipDialog(CWnd* parent,Manager& manager,FileInfoZipRequest request,const std::string& name){
 FileInfoZipTask preparation(manager,std::move(request));
 return fileInfoZipDialog(parent,Json{{"FileName",name}},{},{},{},&preparation);
}
