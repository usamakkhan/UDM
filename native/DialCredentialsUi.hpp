#pragma once
#include "DialCredentials.hpp"
namespace udm {
class DialCredentialFields {
 struct Draft {DialEntry entry;DialCredentialInfo before;DialCredentialChange change;bool dirty=false;};
 Form& form;std::shared_ptr<DialCredentialStore> store;CWnd *user=nullptr,*password=nullptr,*save=nullptr,*apply=nullptr,*status=nullptr;
 std::map<std::string,Draft> drafts;std::string current;bool loading=false,passwordTouched=false;
 static std::string key(const DialEntry& entry){return lower(entry.phonebook)+"\n"+lower(entry.name);}
 void capture(){
  if(loading||current.empty())return;auto& draft=drafts.at(current);auto& change=draft.change;
  change.userName=text(user);change.savePassword=form.checked(save);
  if(passwordTouched){auto plain=text(password);change.protectedPassword=protect(plain);if(!plain.empty())SecureZeroMemory(plain.data(),plain.size());change.passwordEdited=true;}
  draft.dirty=change.passwordEdited||change.userName!=draft.before.userName||change.savePassword!=draft.before.savedPassword;
  apply->EnableWindow(draft.dirty);
 }
 void display(){
  loading=true;auto& draft=drafts.at(current);const auto& change=draft.change;
  user->SetWindowText(cs(change.userName));
  auto shown=change.passwordEdited?reveal(change.protectedPassword):(draft.before.savedPassword||draft.before.sessionPassword?"********":"");
  password->SetWindowText(cs(shown));if(!shown.empty())SecureZeroMemory(shown.data(),shown.size());
  save->SendMessage(BM_SETCHECK,change.savePassword?BST_CHECKED:BST_UNCHECKED);passwordTouched=false;loading=false;
  for(auto control:{user,password,save})control->EnableWindow(TRUE);apply->EnableWindow(draft.dirty);
  status->SetWindowText(cs(draft.before.savedPassword?"Password is saved by Windows.":draft.before.sessionPassword?"Password is available only until UDM exits.":"Enter credentials for this connection. Passwords are stored by Windows only when selected."));
 }
 void commit(Draft& draft){store->write(draft.entry,draft.change);draft.before=store->read(draft.entry);draft.change={draft.before.userName,"",false,draft.before.savedPassword};draft.dirty=false;}
public:
 DialCredentialFields(Form& owner,int ox,int oy,CWnd* note,std::shared_ptr<DialCredentialStore> backend):form(owner),store(std::move(backend)),status(note){
  form.label("User name:",ox+18,oy+84,57,10);user=form.edit("",ox+80,oy+82,117,14);
  form.label("Password:",ox+16,oy+106,59,10);password=form.edit("",ox+80,oy+103,117,14,false,false,true);
  save=form.check("Save password",false,ox+80,oy+123,115,10);apply=form.button("Apply",ox+201,oy+134,50,[this]{applyCurrent();});
  user->SendMessage(EM_SETLIMITTEXT,UNLEN+DNLEN+1);password->SendMessage(EM_SETLIMITTEXT,PWLEN);
  user->SendMessage(EM_SETCUEBANNER,TRUE,(LPARAM)L"User or DOMAIN\\user");
  form.bindChange(user,[this]{capture();});form.bindChange(password,[this]{if(!loading){passwordTouched=true;capture();}});
  form.bind(save,[this]{capture();if(!current.empty()&&!form.checked(save)&&drafts.at(current).before.savedPassword&&!drafts.at(current).change.passwordEdited)status->SetWindowText(L"Apply removes the saved password. Enter it again to use it only for this UDM session.");});
  clear();
 }
 void clear(){capture();current.clear();loading=true;user->SetWindowText(L"");password->SetWindowText(L"");save->SendMessage(BM_SETCHECK,BST_UNCHECKED);for(auto control:{user,password,save,apply})control->EnableWindow(FALSE);passwordTouched=false;loading=false;}
 void load(const DialEntry& entry){
  capture();auto id=key(entry);auto found=drafts.find(id);
  if(found==drafts.end()||!found->second.dirty){auto info=store->read(entry);drafts[id]={entry,info,{info.userName,"",false,info.savedPassword},false};}
  current=id;display();
 }
 void validate(){capture();for(auto& item:drafts)if(item.second.dirty)validateDialCredentialChange(item.second.change);}
 void applyCurrent(){capture();if(current.empty())return;auto& draft=drafts.at(current);if(draft.dirty){validateDialCredentialChange(draft.change);commit(draft);}display();}
 void applyAll(){
  validate();int saved=0;
  try{for(auto& item:drafts)if(item.second.dirty){commit(item.second);++saved;}}
  catch(const std::exception& e){if(saved)throw std::runtime_error(std::to_string(saved)+" connection credential update(s) were saved. The remaining change failed: "+e.what());throw;}
  if(!current.empty())display();
 }
};
}
