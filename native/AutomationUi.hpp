#pragma once
// "Download all links with UDM": review of a link array sent through automation.
#include "Automation.hpp"
namespace udm {
class AutomationReviewForm:public Form {
protected:
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{
  auto header=reinterpret_cast<NMHDR*>(l);
  if(header&&header->hwndFrom==selectionList&&header->code==LVN_ITEMCHANGED&&pulse){
   auto item=reinterpret_cast<NMLISTVIEW*>(l);
   if((item->uChanged&LVIF_STATE)&&((item->uOldState^item->uNewState)&LVIS_STATEIMAGEMASK))pulse();
  }
  return Form::OnNotify(w,l,result);
 }
public:
 using Form::Form;HWND selectionList=nullptr;
};
inline void automationBatchDialog(CWnd* owner,Manager& manager,const Json& offer){
 auto links=offer.at("links");auto id=str(offer,"Id");AutomationReviewForm dialog("Download all links with UDM",734,463,owner);
 dialog.init=[&]{
  dialog.label("Select the files you want to download:",13,13,706);
  auto list=dialog.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT,13,39,706,257);list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
  dialog.selectionList=list->GetSafeHwnd();
  list->InsertColumn(0,L"File name",LVCFMT_LEFT,dialog.rect(0,0,175,0).Width());list->InsertColumn(1,L"Address",LVCFMT_LEFT,dialog.rect(0,0,340,0).Width());list->InsertColumn(2,L"Description",LVCFMT_LEFT,dialog.rect(0,0,170,0).Width());
  for(int i=0;i<(int)links.size();++i){list->InsertItem(i,cs(safeName(unescape(Url(str(links[i],"url")).path))));list->SetItemText(i,1,cs(str(links[i],"url")));list->SetItemText(i,2,cs(str(links[i],"description")));list->SetCheck(i,!automationRowAdded(manager,id,i));}
  dialog.button("Check all",13,308,99,[&,list]{for(int i=0;i<(int)links.size();++i)list->SetCheck(i,!automationRowAdded(manager,id,i));});dialog.button("Uncheck all",122,308,104,[list]{for(int i=0;i<list->GetItemCount();++i)list->SetCheck(i,FALSE);});
  auto status=dialog.label("",247,312,472);dialog.label("Queue",13,350,59);auto queue=dialog.combo(queueNames(manager),"Main queue",78,346,283);auto later=dialog.check("Download later",true,391,347,328);
  dialog.label("Save to",13,386,59);auto folder=dialog.edit("",78,382,522);dialog.button("Browse...",610,381,109,[&,folder]{auto selected=chooseFolder(&dialog,wide(text(folder)));if(!selected.empty())folder->SetWindowText(selected.c_str());});
  auto confirm=dialog.button("OK",509,422,99,[]{});dialog.button("Cancel",620,422,99,[&]{dialog.close(IDCANCEL);});
  dialog.accept=[&,list,queue,later,folder]{auto destination=trim(text(folder));if(!destination.empty()&&!fs::path(wide(destination)).is_absolute())throw std::invalid_argument("Use an absolute destination folder.");int selected=0;
   for(int i=0;i<(int)links.size();++i)if(list->GetCheck(i)&&!automationRowAdded(manager,id,i)){acceptAutomationRow(manager,offer,i,destination,text(queue),dialog.checked(later));list->SetCheck(i,FALSE);++selected;}
   if(!selected)throw std::runtime_error("Select at least one unaccepted file.");dismissAutomationBatch(manager,id);dialog.close();
  };dialog.bind(confirm,dialog.accept);
  dialog.pulse=[&,list,confirm,status]{int selected=0,added=0;for(int i=0;i<(int)links.size();++i){bool done=automationRowAdded(manager,id,i);added+=done;selected+=list->GetCheck(i)&&!done;}confirm->EnableWindow(selected>0);status->SetWindowText(cs(std::to_string(selected)+" selected, "+std::to_string(added)+" already added"));};
  dialog.pulse();
 };
 if(dialog.DoModal()!=IDOK)dismissAutomationBatch(manager,id);
}
}
