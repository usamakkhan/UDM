#pragma once
#include "IdmExport.hpp"
namespace udm {
class IdmImportDialog:public Form {
 Manager& manager;std::vector<IdmExportRecord> rows;CListCtrl* list=nullptr;CWnd *count=nullptr,*submit=nullptr,*session=nullptr;
 void updateCount(){int n=0;for(int i=0;i<list->GetItemCount();++i)n+=list->GetCheck(i)?1:0;count->SetWindowText(cs(std::to_string(n)+" downloads selected"));submit->EnableWindow(n>0);}
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto n=reinterpret_cast<NMHDR*>(l);if(list&&n->hwndFrom==list->GetSafeHwnd()&&n->code==LVN_ITEMCHANGED&&submit)updateCount();return Form::OnNotify(w,l,result);}
public:size_t imported=0;
 IdmImportDialog(Manager& m,std::vector<IdmExportRecord> records,CWnd* parent=nullptr):Form("Import links to UDM",710,461,parent),manager(m),rows(std::move(records)){
 init=[this]{label("Check the downloads to import. Saved request fields stay with their own address.",13,13,684,27);
 list=make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT,13,46,684,235);list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
 list->InsertColumn(0,L"Address",LVCFMT_LEFT,rect(0,0,330,0).Width());list->InsertColumn(1,L"Referer",LVCFMT_LEFT,rect(0,0,200,0).Width());list->InsertColumn(2,L"Saved request",LVCFMT_LEFT,rect(0,0,135,0).Width());
 for(int i=0;i<(int)rows.size();++i){auto& row=rows[i];list->InsertItem(i,cs(row.url));list->SetItemText(i,1,cs(row.referer));list->SetItemText(i,2,cs(std::string(row.post?"POST":"GET")+(row.cookie.empty()?"":" + cookies")));list->SetCheck(i,TRUE);}
 button("Check all",13,292,96,[this]{for(int i=0;i<list->GetItemCount();++i)list->SetCheck(i,TRUE);updateCount();});button("Uncheck all",121,292,103,[this]{for(int i=0;i<list->GetItemCount();++i)list->SetCheck(i,FALSE);updateCount();});count=label("",240,297,445);
 label("Queue",13,336,57);auto names=queueNames(manager);if(std::find(names.begin(),names.end(),"Imported")==names.end())names.push_back("Imported");auto queue=combo(names,"Imported",78,330,224);
 session=check("Use saved cookies and form data",false,320,330,374);label("Save to",13,375,57);auto folder=edit("",78,370,510);button("Browse...",599,369,98,[this,folder]{auto chosen=chooseFolder(this,wide(text(folder)));if(!chosen.empty())folder->SetWindowText(chosen.c_str());});
 label("Added paused. Existing files and download records are preserved.",13,413,468,35);
 accept=[this,queue,folder]{std::vector<IdmExportRecord> selected;for(int i=0;i<(int)rows.size();++i)if(list->GetCheck(i))selected.push_back(rows[i]);imported=importIdmFile(manager,selected,trim(text(folder)),text(queue),checked(session));close();};submit=button("Import",488,418,99,accept);button("Cancel",599,418,98,[this]{close(IDCANCEL);});defaultButton(submit);updateCount();
 };
 }
};
}
