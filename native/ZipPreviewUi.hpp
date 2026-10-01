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
};
BEGIN_MESSAGE_MAP(ZipPreviewList,CListCtrl)
 ON_NOTIFY_REFLECT(LVN_GETDISPINFO,itemText)
END_MESSAGE_MAP()
inline void remoteZipDialog(CWnd* parent,Json data,Json prefs,std::function<void(const Json&)> save){
 Form dialog("Zip preview",386,222,parent);dialog.dialogUnits=true;std::unique_ptr<ZipPreviewTask> task;
 dialog.init=[&]{
  dialog.control(L"STATIC",str(data,"FileName"),SS_CENTER|SS_ENDELLIPSIS,7,3,372,10);
  auto list=dialog.make<ZipPreviewList>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_OWNERDATA|LVS_SHOWSELALWAYS,7,16,372,160);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->SetBkColor(uiBackground());list->SetTextBkColor(uiBackground());list->SetTextColor(uiForeground());
  list->InsertColumn(0,L"Name",LVCFMT_LEFT,dialog.pixelsX(190));list->InsertColumn(1,L"Size",LVCFMT_RIGHT,dialog.pixelsX(62));list->InsertColumn(2,L"Packed",LVCFMT_RIGHT,dialog.pixelsX(62));list->InsertColumn(3,L"Encrypted",LVCFMT_LEFT,dialog.pixelsX(50));
  auto status=dialog.label("Reading ZIP directory...",7,179,372,22);auto close=dialog.button("Cancel",167,204,50,[&]{if(task)task->stop();dialog.close();});dialog.defaultButton(close);dialog.cancel=[&]{if(task)task->stop();dialog.close(IDCANCEL);};dialog.accept=dialog.cancel;
  auto displayed=std::make_shared<bool>(false);task=std::make_unique<ZipPreviewTask>(data,prefs,std::move(save));
  dialog.pulse=[&,list,status,close,displayed]{if(*displayed)return;auto state=task->snapshot();if(state.status=="Checking")return;*displayed=true;close->SetWindowText(L"OK");if(state.listing){list->listing=state.listing;list->SetItemCountEx((int)state.listing->entries.size(),LVSICF_NOSCROLL);status->SetWindowText(cs(std::to_string(state.listing->entries.size())+" entries; "+bytes(state.listing->received)+" read. Nothing extracted."+(state.listing->validated?"":" No strong server validator; listing is informational.")));}else status->SetWindowText(cs(state.message.empty()?"ZIP preview cancelled.":state.message));};
 };
 dialog.DoModal();if(task)task->stop();
}
