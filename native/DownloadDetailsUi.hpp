// Included inside udm after Form and queue helpers.
inline void copyEdit(CWnd* field){field->SendMessage(EM_SETSEL,0,-1);field->SendMessage(WM_COPY);}
inline void downloadLinkDetails(CWnd* owner,Manager& manager,JobPtr job){
 Json data;{Lock lock(manager.mutex);data=job->snapshot();}auto links=downloadLinks(data);Form dialog("Download links",690,194+(int)links.size()*63,owner);
 dialog.init=[&]{int y=14;for(const auto& link:links){dialog.label(link.label,14,y,660);auto value=dialog.edit(link.address,14,y+23,566,23,true);dialog.button("Copy",590,y+22,85,[value]{copyEdit(value);});y+=63;}
  auto note=mediaAddressNote(data);dialog.label(note.empty()?"These are the addresses UDM uses for this download.":note,14,y,660,39);y+=48;
  dialog.label("Web page",14,y,83);auto page=dialog.edit(recoveryPage(data),103,y-4,477,23,true);auto open=dialog.button("Open page",590,y-5,85,[&dialog,page]{Url u(text(page));if(u.scheme!="http"&&u.scheme!="https")throw std::runtime_error("No HTTP or HTTPS page is available.");openFile(&dialog,fs::path(wide(u.full)));});open->EnableWindow(!text(page).empty());
  dialog.button("Close",590,y+40,85,[&]{dialog.close();});
 };dialog.DoModal();
}
inline void requestDownloadLogin(CWnd* owner,Manager& manager,JobPtr job){
 Json data;{Lock lock(manager.mutex);data=job->data;}if(!canRequestLogin(data))return;
 Form dialog("Login required",520,254,owner);dialog.init=[&]{
  dialog.label("This download server requires a login:",14,14,491);dialog.edit(str(data,"AuthenticationOrigin"),14,39,491,23,true);
  Json loginPrefs;{Lock lock(manager.mutex);loginPrefs=manager.state["Settings"];}auto initial=basicLogin(siteRequestHeaders(str(data,"Url"),readHeaders(data),loginPrefs));dialog.label("User name",14,81,90);auto user=dialog.edit(initial.first,112,77,393);
  dialog.label("Password",14,116,90);auto password=dialog.edit(initial.second,112,112,393,23,false,false,true);
  auto show=dialog.check("Show password",false,112,145,175);dialog.bind(show,[&dialog,show,password]{password->SendMessage(EM_SETPASSWORDCHAR,dialog.checked(show)?0:0x25cf);password->Invalidate();});
  auto remember=dialog.check("Remember for this site",false,14,178,491);remember->EnableWindow(siteLoginScheme(Url(str(data,"AuthenticationOrigin")).scheme));
  dialog.accept=[&,user,password,remember]{manager.setDownloadLogin(job,text(user),text(password),dialog.checked(remember));manager.resume(job);dialog.close();};
  dialog.button("Sign in and retry",267,215,140,dialog.accept);dialog.button("Cancel",417,215,88,[&]{dialog.close(IDCANCEL);});user->SetFocus();
 };dialog.DoModal();
}
