#pragma once
// Native review for a canceled browser response whose original refresh target
// changed before commit. Later/close preserves the protected request.
class CaptureReviewDialog:public Form {
 Manager& manager;std::string token;Json displayed;
 CWnd *original=nullptr,*current=nullptr,*fresh=nullptr,*status=nullptr,*useExisting=nullptr;
 void render(const Json& view){
  displayed=view;
  if(view.is_null()){status->SetWindowText(L"This captured link has already been handled.");useExisting->EnableWindow(FALSE);return;}
  original->SetWindowText(cs(str(view,"originalName")+"\r\n"+str(view,"previousUrl")));
  current->SetWindowText(cs(yes(view,"targetPresent")?str(view,"targetName")+" — "+str(view,"targetStatus")+"\r\n"+str(view,"targetUrl"):"The original download was removed from the list."));
  fresh->SetWindowText(cs(str(view,"newUrl")));useExisting->EnableWindow(yes(view,"canRefresh"));
  status->SetWindowText(cs(yes(view,"canRefresh")?"Review replacement opens the address dialog before any saved parts are reused.":"The original download cannot use this link now. Choose a new file, or leave this link for later."));
 }
 void choose(const std::string& action){
  auto latest=manager.browserCaptureReview(token);
  if(latest!=displayed){render(latest);status->SetWindowText(L"The original download changed again. Review these details, then choose again.");return;}
  if(latest.is_null()){close();return;}
  manager.resolveBrowserCaptureReview(displayed,action);close();
 }
public:
 CaptureReviewDialog(Manager& m,const std::string& id,CWnd* owner):Form("Review captured download",654,383,owner),manager(m),token(id){init=[this]{
  label("The download selected for this replacement link changed before it could be accepted.",14,12,626,24);
  label("The browser response was canceled. UDM saved the link for your decision; the original download and its saved parts are unchanged.",14,42,626,34);
  label("Originally selected",14,87,132);original=edit("",150,83,490,45,true,true);
  label("Current download",14,141,132);current=edit("",150,137,490,45,true,true);
  label("Captured address",14,195,132);fresh=edit("",150,191,490,45,true,true);
  status=label("",14,251,626,43);
  useExisting=button("Review replacement...",14,308,181,[this]{choose("refresh");});
  button("Download new file...",205,308,175,[this]{choose("new");});
  button("Discard captured link",390,308,162,[this]{choose("discard");});
  button("Later",562,308,78,[this]{close(IDCANCEL);});
  label("Later keeps the link. Reopen it with Recover interrupted downloads in the browser extension.",14,349,626,23);
  cancel=[this]{close(IDCANCEL);};render(manager.browserCaptureReview(token));
 };}
};
