#pragma once
#include "DialCredentials.hpp"
#include "AsyncDialLookup.hpp"
namespace udm {
class DialCredentialFields {
 struct Draft {DialEntry entry;DialCredentialInfo before;DialCredentialChange change;bool dirty=false;};
 Form& form;std::shared_ptr<DialCredentialStore> store;CWnd *user=nullptr,*password=nullptr,*save=nullptr,*apply=nullptr,*status=nullptr;
 std::map<std::string,Draft> drafts;std::string current;bool loading=false,passwordTouched=false;
 bool backgroundReads=false;AsyncDialLookup lookup;DialEntry reading;
 static std::string key(const DialEntry& entry){return lower(entry.phonebook)+"\n"+lower(entry.name);}
 void capture(){
  if(loading||!currentReady())return;auto& draft=drafts.at(current);auto& change=draft.change;
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
 // Owned worker input/output. No form, control, or reference into drafts is
 // transferred; acknowledge only after joining the worker on the UI thread.
 struct Batch {
  struct Item {std::string key;DialEntry entry;DialCredentialChange change;std::optional<DialCredentialInfo> saved;};
  std::shared_ptr<DialCredentialStore> store;std::vector<Item> items;
  void apply(){int count=0;try{for(auto& item:items){store->write(item.entry,item.change);item.saved=store->read(item.entry);++count;}}
   catch(const std::exception& e){if(count)throw std::runtime_error(std::to_string(count)+" connection credential update(s) were saved. The remaining change failed: "+e.what());throw;}}
 };
 std::shared_ptr<Batch> prepareBatch(){validate();auto batch=std::make_shared<Batch>();batch->store=store;for(const auto& item:drafts)if(item.second.dirty)batch->items.push_back({item.first,item.second.entry,item.second.change,{}});return batch;}
 std::shared_ptr<Batch> prepareCurrentBatch(){capture();auto batch=std::make_shared<Batch>();batch->store=store;if(!currentReady()&&lookup.pending())throw std::runtime_error("Connection details are still loading. Try again shortly.");if(currentReady()){const auto& draft=drafts.at(current);if(draft.dirty){validateDialCredentialChange(draft.change);batch->items.push_back({current,draft.entry,draft.change,{}});}}return batch;}
 void acknowledgeBatch(const Batch& batch){for(const auto& item:batch.items)if(item.saved){if(item.key==current)lookup.invalidate();auto& draft=drafts.at(item.key);draft.before=*item.saved;draft.change={draft.before.userName,"",false,draft.before.savedPassword};draft.dirty=false;}if(currentReady())display();}
 bool currentReady()const{return !current.empty()&&drafts.find(current)!=drafts.end();}
 bool readPending()const{return lookup.pending();}
 void stopRead()noexcept{lookup.stop();}
 void updateRead(){if(auto result=lookup.take()){if(current.empty())return;if(currentReady()&&drafts.at(current).dirty){status->SetWindowText(L"Changes have not been applied.");return;}if(result->value){auto info=std::get<DialCredentialInfo>(*result->value);auto entry=reading;drafts[current]={entry,info,{info.userName,"",false,info.savedPassword},false};display();}else status->SetWindowText(cs(result->error));}}
 DialCredentialFields(Form& owner,int ox,int oy,CWnd* note,std::shared_ptr<DialCredentialStore> backend,std::function<void()> applyRequest={},bool asyncReads=false):form(owner),store(std::move(backend)),status(note),backgroundReads(asyncReads){
  form.label("User name:",ox+20,oy+84,65,10);user=form.edit("",ox+91,oy+82,133,14);
  form.label("Password:",ox+18,oy+106,67,10);password=form.edit("",ox+91,oy+103,133,14,false,false,true);
  save=form.check("Save password",false,ox+91,oy+123,131,10);apply=form.button("Apply",ox+229,oy+134,57,[this,request=std::move(applyRequest)]{if(request)request();else applyCurrent();});
  user->SendMessage(EM_SETLIMITTEXT,UNLEN+DNLEN+1);password->SendMessage(EM_SETLIMITTEXT,PWLEN);
  user->SendMessage(EM_SETCUEBANNER,TRUE,(LPARAM)L"User or DOMAIN\\user");
  form.bindChange(user,[this]{capture();});form.bindChange(password,[this]{if(!loading){passwordTouched=true;capture();}});
  form.bind(save,[this]{capture();if(!current.empty()&&!form.checked(save)&&drafts.at(current).before.savedPassword&&!drafts.at(current).change.passwordEdited)status->SetWindowText(L"Apply removes the saved password. Enter it again to use it only for this UDM session.");});
  clear();
 }
 void clear(){capture();lookup.invalidate();current.clear();loading=true;user->SetWindowText(L"");password->SetWindowText(L"");save->SendMessage(BM_SETCHECK,BST_UNCHECKED);for(auto control:{user,password,save,apply})control->EnableWindow(FALSE);passwordTouched=false;loading=false;}
 void load(const DialEntry& entry){
  capture();if(backgroundReads){auto id=key(entry);auto found=drafts.find(id);clear();current=id;reading=entry;if(found!=drafts.end())display();if(found==drafts.end()||!found->second.dirty){status->SetWindowText(L"Reading Windows connection credentials...");try{lookup.request([backend=store,entry]{return AsyncDialLookup::Value(backend->read(entry));},form.taskNotifier());}catch(const std::exception& e){status->SetWindowText(cs(e.what()));}}return;}auto id=key(entry);auto found=drafts.find(id);
  if(found==drafts.end()||!found->second.dirty){auto info=store->read(entry);drafts[id]={entry,info,{info.userName,"",false,info.savedPassword},false};}
  current=id;display();
 }
 void validate(){capture();for(auto& item:drafts)if(item.second.dirty)validateDialCredentialChange(item.second.change);}
 void applyCurrent(){capture();if(!currentReady())return;auto& draft=drafts.at(current);if(draft.dirty){validateDialCredentialChange(draft.change);commit(draft);}display();}
 void applyAll(){
  validate();int saved=0;
  try{for(auto& item:drafts)if(item.second.dirty){commit(item.second);++saved;}}
  catch(const std::exception& e){if(saved)throw std::runtime_error(std::to_string(saved)+" connection credential update(s) were saved. The remaining change failed: "+e.what());throw;}
  if(currentReady())display();
 }
};
}
