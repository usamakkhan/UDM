// Included inside GrabberDialog.
std::string waitingBrowserTicket;bool exploreAfterLogin=false,resumeAfterLogin=false;
void browserLoginControls(){
 const bool manual=yes(project,"BrowserLogin");loginEnabled->EnableWindow(!manual);
 loginUser->EnableWindow(!manual&&checked(loginEnabled));loginPassword->EnableWindow(!manual&&checked(loginEnabled));
 if(manual)loginHint->SetWindowText(str(project,"ProtectedBrowserSession").empty()?L"Browser sign-in enabled. Open Advanced to sign in or change its settings.":L"A saved browser session is ready. Use Advanced to sign in again.");
}
bool syncBrowserSignIn(){
 if(waitingBrowserTicket.empty())return false;Json saved;try{saved=grabberProject(manager,str(project,"Id"));}catch(...){waitingBrowserTicket.clear();return false;}
 if(str(saved,"LastBrowserLoginTicket")==waitingBrowserTicket&&!saved.contains("PendingBrowserLogin")){
  for(const char* key:{"ProtectedBrowserSession","LastBrowserLoginTicket","BrowserLoginSaved"})if(saved.contains(key))project[key]=saved[key];project.erase("PendingBrowserLogin");project.erase("ExploreState");waitingBrowserTicket.clear();browserLoginControls();status->SetWindowText(L"Browser sign-in saved. You can explore and download this website.");return true;
 }
 auto pending=saved.value("PendingBrowserLogin",Json::object());if(str(pending,"Token")!=waitingBrowserTicket||num(pending,"Expires")<epoch()){waitingBrowserTicket.clear();exploreAfterLogin=false;status->SetWindowText(L"Browser sign-in stopped or expired. Open Advanced to sign in again.");}return false;
}
void browserSignIn(bool exploreAfter=false,bool resumeAfter=false){
 if(running)return;syncBrowserSignIn();collect();auto nextProject=project;nextProject.erase("ProtectedBrowserSession");nextProject.erase("LastBrowserLoginTicket");project=beginGrabberBrowserLogin(manager,nextProject);
 waitingBrowserTicket=str(project["PendingBrowserLogin"],"Token");exploreAfterLogin=exploreAfter;resumeAfterLogin=resumeAfter;fillProjects();browserLoginControls();page();
 status->SetWindowText(L"Sign in in your browser, then open the UDM extension and choose Use this signed-in session.");
 auto result=ShellExecuteW(m_hWnd,L"open",wide(str(project["PendingBrowserLogin"],"LoginPage")).c_str(),nullptr,nullptr,SW_SHOWNORMAL);
 if((INT_PTR)result<=32)throw std::runtime_error("Windows could not open the sign-in page. Open that page in your browser, then use the UDM extension.");
}
void advancedLogin(){
 if(running)return;Form dialog("Website authorization",646,294,this);auto draft=project;bool changed=false,start=false;
 dialog.init=[&]{
  auto manual=dialog.check("Enter login and password manually in the browser",yes(draft,"BrowserLogin"),14,15,615);
  dialog.label("Login page",14,55,105);auto loginPage=dialog.edit(str(draft,"LoginPage",trim(text(address))),126,51,505);
  dialog.label("Logout pages to avoid",14,91,600);auto logout=dialog.edit(str(draft,"LogoutPages","*/logout*,*/logoff*,*/signout*,*/sign-out*"),14,113,617,49,false,true);
  dialog.label("Sign in normally. Return to this project's website and choose Use this signed-in session in the UDM extension. Its cookies are saved for this project.",14,174,617,47);
  auto save=[&,manual,loginPage,logout](bool begin){draft["BrowserLogin"]=dialog.checked(manual);draft["LoginPage"]=trim(text(loginPage));draft["LogoutPages"]=trim(text(logout));draft["StartUrl"]=trim(text(address));if(yes(draft,"BrowserLogin")){draft.erase("ProtectedLogin");if(begin||(!str(draft,"ProtectedBrowserSession").empty()&&str(readBrowserSession(draft),"Origin")!=Url(str(draft,"StartUrl")).origin)){draft.erase("ProtectedBrowserSession");draft.erase("LastBrowserLoginTicket");draft.erase("PendingBrowserLogin");}}else{draft.erase("ProtectedBrowserSession");draft.erase("LastBrowserLoginTicket");draft.erase("PendingBrowserLogin");}validateGrabberBrowserLogin(draft);changed=true;start=begin&&yes(draft,"BrowserLogin");dialog.close();};
  dialog.button("Clear saved sign-in",14,246,155,[&]{draft.erase("ProtectedBrowserSession");draft.erase("LastBrowserLoginTicket");draft.erase("PendingBrowserLogin");});
  dialog.button("Sign in now",180,246,134,[save]{save(true);});dialog.button("OK",431,246,94,[save]{save(false);});dialog.button("Cancel",537,246,94,[&]{dialog.close(IDCANCEL);});
 };dialog.DoModal();if(!changed)return;
 for(const char* key:{"BrowserLogin","LoginPage","LogoutPages","ProtectedBrowserSession","LastBrowserLoginTicket","PendingBrowserLogin"}){if(draft.contains(key))project[key]=draft[key];else project.erase(key);}
 if(yes(project,"BrowserLogin")){project.erase("ProtectedLogin");loginEnabled->SendMessage(BM_SETCHECK,BST_UNCHECKED);loginUser->SetWindowText(L"");loginPassword->SetWindowText(L"");loginOrigin.clear();}waitingBrowserTicket.clear();browserLoginControls();if(start)browserSignIn();
}
