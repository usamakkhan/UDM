#pragma once
#include "Catalog.hpp"
#include "ExportScope.hpp"
namespace udm {
class ExportDownloadsDialog:public Form {
 Manager& manager;std::vector<JobPtr> all,rows;std::set<std::string> selectedIds;std::map<std::string,bool> choices;
 ExportScope scope;std::string initialQueue;std::vector<std::string> queueNames;
 CListCtrl* list=nullptr;CComboBox *queues=nullptr,*format=nullptr;CWnd *allRadio=nullptr,*selectedRadio=nullptr,*queueRadio=nullptr,*credentials=nullptr,*count=nullptr,*exportButton=nullptr;
 void retainChoices(){if(list)for(int i=0;i<(int)rows.size();++i)choices[rows[i]->id()]=list->GetCheck(i)!=FALSE;}
 void rebuild(){
  retainChoices();Lock lock(manager.mutex);auto index=queues->GetCurSel();auto queue=index>0&&index<=(int)queueNames.size()?queueNames[index-1]:std::string();rows=exportScopeRows(all,selectedIds,scope,queue);
  list->SetRedraw(FALSE);list->DeleteAllItems();
  for(int i=0;i<(int)rows.size();++i){auto job=rows[i];list->InsertItem(i,cs(str(job->data,"FileName")));list->SetItemText(i,1,cs(str(job->data,"Status")));list->SetItemText(i,2,cs(str(job->data,"Queue")));auto found=choices.find(job->id());list->SetCheck(i,found==choices.end()||found->second);}
  list->SetRedraw(TRUE);list->Invalidate();allRadio->SendMessage(BM_SETCHECK,scope==ExportScope::All);selectedRadio->SendMessage(BM_SETCHECK,scope==ExportScope::Selected);queueRadio->SendMessage(BM_SETCHECK,scope==ExportScope::Queue);queues->EnableWindow(scope==ExportScope::Queue);updateCount();
 }
 void updateCount(){int checkedRows=0;for(int i=0;i<list->GetItemCount();++i)checkedRows+=list->GetCheck(i)?1:0;count->SetWindowText(cs(std::to_string(checkedRows)+" of "+std::to_string(rows.size())+" downloads selected"));exportButton->EnableWindow(checkedRows>0);}
 void updateFormat(){bool catalog=format->GetCurSel()==1;credentials->EnableWindow(catalog);}
 void exportFiles(){
  std::vector<JobPtr> chosen;for(int i=0;i<(int)rows.size();++i)if(list->GetCheck(i))chosen.push_back(rows[i]);if(chosen.empty())throw std::runtime_error("Select at least one download.");
  bool catalog=format->GetCurSel()==1;CFileDialog chooser(FALSE,catalog?L"udmcatalog":L"txt",catalog?L"udm-downloads.udmcatalog":L"udm-downloads.txt",OFN_OVERWRITEPROMPT,catalog?L"UDM catalog|*.udmcatalog||":L"URL list|*.txt||",this);if(chooser.DoModal()!=IDOK)return;
  writeDownloadExport(manager,chosen,fs::path((LPCWSTR)chooser.GetPathName()),catalog,catalog&&checked(credentials));close();
 }
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto n=reinterpret_cast<NMHDR*>(l);if(list&&n->hwndFrom==list->GetSafeHwnd()&&n->code==LVN_ITEMCHANGED&&count&&exportButton){updateCount();}return Form::OnNotify(w,l,result);}
public:
 ExportDownloadsDialog(Manager& m,const std::vector<JobPtr>& selection,std::string queue,CWnd* parent=nullptr):Form("Export downloads",654,492,parent),manager(m),scope(selection.empty()?ExportScope::All:ExportScope::Selected),initialQueue(std::move(queue)){
  {Lock lock(m.mutex);all=m.jobs;for(auto job:selection)if(std::find(all.begin(),all.end(),job)!=all.end())selectedIds.insert(job->id());for(auto q:m.state["Queues"])queueNames.push_back(str(q,"Name"));}
  init=[this]{
   allRadio=control(L"BUTTON","All downloads",WS_GROUP|WS_TABSTOP|BS_AUTORADIOBUTTON,13,13,148,22);selectedRadio=control(L"BUTTON","Selected downloads",WS_TABSTOP|BS_AUTORADIOBUTTON,177,13,175,22);queueRadio=control(L"BUTTON","Files in download queue",WS_TABSTOP|BS_AUTORADIOBUTTON,368,13,271,22);
   selectedRadio->EnableWindow(!selectedIds.empty());label("Queue",13,49,55);auto names=std::vector<std::string>{"All queues"};names.insert(names.end(),queueNames.begin(),queueNames.end());queues=combo(names,"All queues",77,44,366);auto found=std::find(queueNames.begin(),queueNames.end(),initialQueue);if(found!=queueNames.end())queues->SetCurSel((int)(found-queueNames.begin())+1);
   count=label("",457,49,182,28);list=make<CListCtrl>(WS_GROUP|WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT,13,83,626,242);list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->InsertColumn(0,L"File name",LVCFMT_LEFT,rect(0,0,350,0).Width());list->InsertColumn(1,L"Status",LVCFMT_LEFT,rect(0,0,95,0).Width());list->InsertColumn(2,L"Queue",LVCFMT_LEFT,rect(0,0,150,0).Width());
   button("Check all",13,336,96,[this]{for(int i=0;i<list->GetItemCount();++i)list->SetCheck(i,TRUE);updateCount();});button("Uncheck all",121,336,101,[this]{for(int i=0;i<list->GetItemCount();++i)list->SetCheck(i,FALSE);updateCount();});label("Format",247,341,55);format=combo({"URL list (.txt)","UDM catalog (.udmcatalog)"},"UDM catalog (.udmcatalog)",309,336,330);
   credentials=check("Include Windows-account-encrypted request credentials",false,13,379,626);label("Catalogs include descriptions, source pages, file properties and queue names.",13,413,626,29);
   accept=[this]{exportFiles();};exportButton=button("Export...",435,453,96,accept);button("Cancel",543,453,96,[this]{close(IDCANCEL);});defaultButton(exportButton);
   bind(allRadio,[this]{scope=ExportScope::All;rebuild();});bind(selectedRadio,[this]{scope=ExportScope::Selected;rebuild();});bind(queueRadio,[this]{scope=ExportScope::Queue;rebuild();});bindChange(queues,[this]{rebuild();});bindChange(format,[this]{updateFormat();});rebuild();updateFormat();
  };
 }
};
}
