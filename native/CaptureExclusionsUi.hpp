// Included inside udm, after Form. All settings edits remain a draft until OK.
class CaptureExclusionOffer:public Form {
public:
 CaptureExclusionOffer(CWnd* owner,Manager& manager,Json offer):Form("Cancelling an automatically started download",283,178,owner){
  dialogUnits=true;init=[this,&manager,offer]{
   label("You cancelled two automatic downloads from this site. If UDM is taking over downloads you do not want, add an exception below.",6,5,271,33);
   auto site=check("Don't automatically download from this site:",false,15,46,261);
   edit(str(offer,"host"),27,59,248,14,true);
   auto address=check("Don't automatically download from this exact address:",true,15,78,261);
   edit(str(offer,"address"),27,91,248,14,true);
   label("The site is the file's server (including its subdomains). Edit exceptions later in Options > File Types.",7,111,269,20);
   auto suppress=check("Don't show this dialog again",false,15,160,253);
   accept=[this,&manager,offer,site,address,suppress]{manager.applyCaptureExclusions(offer,checked(site),checked(address),checked(suppress));close();};
   defaultButton(button("OK",82,139,50,accept));button("Cancel",151,139,50,[this]{close(IDCANCEL);});
  };
 }
};
class AddressExceptionsDialog:public Form {
 Json& prefs;std::vector<std::string> rows;CListCtrl* list=nullptr;CWnd* remove=nullptr;
 void reload(){list->DeleteAllItems();for(int i=0;i<(int)rows.size();++i){bool exact=rows[i].rfind("=",0)==0;list->InsertItem(i,cs(exact?rows[i].substr(1):rows[i]));list->SetItemText(i,1,exact?L"Exact":L"Pattern");}remove->EnableWindow(list->GetNextItem(-1,LVNI_SELECTED)>=0);}
 void add(){
  Form editor("Add an address to exceptions list",300,88,this);editor.dialogUnits=true;
  editor.init=[&]{editor.label("Web address or pattern:",7,6,286);auto value=editor.edit("",7,19,286);
   auto exact=editor.check("Match this exact address (including its query)",false,7,39,286);
   editor.label("For patterns, * matches any part of the path.",7,53,286);
   editor.accept=[&,value,exact]{auto entry=trim(text(value));if(editor.checked(exact))entry="="+exactCaptureAddress(entry);else{auto parsed=addressExceptions(entry);if(parsed.size()!=1||entry.rfind("=",0)==0)throw std::runtime_error("Enter one address pattern, or select exact address.");entry=parsed.front();}
    auto next=rows;if(std::find(next.begin(),next.end(),entry)==next.end())next.push_back(entry);addressExceptions(captureExceptionText(next));rows=std::move(next);reload();editor.close();};
   editor.defaultButton(editor.button("OK",174,68,50,editor.accept));editor.button("Cancel",236,68,50,[&]{editor.close(IDCANCEL);});
  };editor.DoModal();
 }
 BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{auto n=reinterpret_cast<NMHDR*>(l);if(list&&n->hwndFrom==list->GetSafeHwnd()&&n->code==LVN_ITEMCHANGED&&remove)remove->EnableWindow(list->GetNextItem(-1,LVNI_SELECTED)>=0);return Form::OnNotify(w,l,result);}
public:
 AddressExceptionsDialog(CWnd* owner,Json& settings):Form("The list of address exceptions",281,198,owner),prefs(settings),rows(addressExceptions(str(settings,"CaptureExcludedUrls"))){
  dialogUnits=true;init=[this]{
   auto offers=check("Offer an exception after two cancelled automatic downloads",yes(prefs,"OfferCaptureExclusions",true),7,6,267,20);
   label("Don't automatically download from these addresses:",7,29,267);
   list=make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_REPORT|LVS_SHOWSELALWAYS,7,42,267,109);list->SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);list->InsertColumn(0,L"Address",LVCFMT_LEFT,pixelsX(215));list->InsertColumn(1,L"Match",LVCFMT_LEFT,pixelsX(46));
   label("Patterns support *; exact addresses match literally.",7,157,267);
   button("Add",16,177,50,[this]{add();});remove=button("Delete",79,177,50,[this]{for(int i=list->GetItemCount()-1;i>=0;--i)if(list->GetItemState(i,LVIS_SELECTED)&LVIS_SELECTED)rows.erase(rows.begin()+i);reload();});
   accept=[this,offers]{auto next=prefs;next["CaptureExcludedUrls"]=captureExceptionText(rows);next["OfferCaptureExclusions"]=checked(offers);validateBrowserSettings(next);prefs=std::move(next);close();};defaultButton(button("OK",153,177,50,accept));button("Cancel",215,177,50,[this]{close(IDCANCEL);});reload();
  };
 }
};
