#pragma once
#include "DialCredentials.hpp"
#include <raserror.h>
static void dialCredentialChecks(){
 auto plain=dialCredentialName("user@example.test");check(plain.first.empty()&&plain.second==L"user@example.test","Dial credentials preserve a UPN user name");
 auto domain=dialCredentialName("DOMAIN\\user");check(domain.first==L"DOMAIN"&&domain.second==L"user","Dial credentials separate Windows domain and user");
 check(dialCredentialName("").second.empty(),"Dial credential editor permits clearing a user name");
 rejects([]{dialCredentialName("DOMAIN\\");},"Dial credentials reject incomplete domain identities");
 rejects([]{dialCredentialName("\\user");},"Dial credentials reject an empty explicit domain");
 rejects([]{dialCredentialName("one\\two\\three");},"Dial credentials reject ambiguous domain identities");
 rejects([]{dialCredentialName(std::string("a\0b",3));},"Dial credentials reject embedded null user names");
 rejects([]{dialCredentialName("a\nb");},"Dial credentials reject multiline user names");
 rejects([]{dialCredentialName(std::string(UNLEN+1,'u'));},"Dial user name obeys the Windows length bound");
 rejects([]{dialCredentialName(std::string(DNLEN+1,'d')+"\\u");},"Dial domain obeys the Windows length bound");
 DialCredentialChange c{"user",protect(std::string(PWLEN,'p')),true,true};validateDialCredentialChange(c);check(true,"Dial password accepts the documented Windows length");
 c.protectedPassword=protect(std::string(PWLEN+1,'p'));rejects([&]{validateDialCredentialChange(c);},"Dial password rejects an oversized value");
 c.protectedPassword=protect(std::string("a\0b",3));rejects([&]{validateDialCredentialChange(c);},"Dial password rejects embedded null data");
 c.protectedPassword=protect("a\rb");rejects([&]{validateDialCredentialChange(c);},"Dial password rejects multiline data");
 c.protectedPassword="not-protected";rejects([&]{validateDialCredentialChange(c);},"Dial password rejects malformed protected data");
 c.passwordEdited=false;validateDialCredentialChange(c);check(true,"Unchanged password field never decodes its display placeholder");
 c.passwordEdited=true;c.protectedPassword="";validateDialCredentialChange(c);check(true,"Explicit empty password is a supported clear operation");
}
static void dialCredentialOsChecks(const fs::path& root){
 struct PrivateEntry{
  std::wstring book,name;bool created=false;
  ~PrivateEntry(){if(created)RasDeleteEntryW(book.c_str(),name.c_str());}
 } fixture;
 fixture.book=(root/L"isolated-credentials.pbk").wstring();fixture.name=L"UDM credential fixture "+wide(guid());
 atomicText(fs::path(fixture.book),"",false);
 DWORD bytes=0,count=0;auto error=RasEnumDevicesW(nullptr,&bytes,&count);
 if(error!=ERROR_BUFFER_TOO_SMALL&&error!=ERROR_SUCCESS)throw std::runtime_error("RAS device enumeration failed: "+std::to_string(error));
 std::vector<RASDEVINFOW> devices((bytes+sizeof(RASDEVINFOW)-1)/sizeof(RASDEVINFOW));if(devices.empty())throw std::runtime_error("No Windows RAS device is available for the isolated fixture.");
 devices[0].dwSize=sizeof(RASDEVINFOW);error=RasEnumDevicesW(devices.data(),&bytes,&count);if(error)throw std::runtime_error("RAS device read failed: "+std::to_string(error));
 std::wstring device;for(DWORD i=0;i<count;++i)if(_wcsicmp(devices[i].szDeviceType,RASDT_Vpn)==0){device=devices[i].szDeviceName;break;}
 if(device.empty())throw std::runtime_error("No VPN device is available for the private phone-book fixture.");
 RASENTRYW entry{};entry.dwSize=sizeof(entry);entry.dwType=RASET_Vpn;entry.dwFramingProtocol=RASFP_Ppp;entry.dwfNetProtocols=RASNP_Ip;entry.dwVpnStrategy=VS_Default;
 wcscpy_s(entry.szDeviceType,RASDT_Vpn);wcscpy_s(entry.szDeviceName,device.c_str());wcscpy_s(entry.szLocalPhoneNumber,L"127.0.0.1");
 error=RasSetEntryPropertiesW(fixture.book.c_str(),fixture.name.c_str(),&entry,sizeof(entry),nullptr,0);if(error)throw std::runtime_error("Private RAS entry creation failed: "+std::to_string(error));fixture.created=true;
 DialEntry target{utf8(fixture.name),utf8(fixture.book)};auto store=dialCredentialStore();auto initial=store->read(target);
 check(!initial.savedPassword&&!initial.sessionPassword,"Private Windows phone book starts without a password");
 DialCredentialChange change{"EXAMPLE\\udm-fixture",protect("UDM-fixture-secret"),true,true};store->write(target,change);auto saved=store->read(target);
 check(saved.userName==change.userName&&saved.savedPassword&&!saved.sessionPassword,"Actual Windows RAS stores the selected identity and saved password");
 struct Params {RASDIALPARAMSW value{};~Params(){SecureZeroMemory(&value,sizeof(value));}} args;args.value.dwSize=sizeof(args.value);
 dialConnectionCredentials(target,args.value);check(std::wstring(args.value.szDomain)==L"EXAMPLE"&&std::wstring(args.value.szUserName)==L"udm-fixture"&&args.value.szPassword[0]!=0,"Download dialing receives the Windows identity and opaque saved-password handle");
 change={"EXAMPLE\\renamed","",false,true};store->write(target,change);saved=store->read(target);
 check(saved.userName==change.userName&&saved.savedPassword,"Editing only the Windows user name preserves its saved password");
 change={change.userName,protect("UDM-session-only"),true,false};store->write(target,change);auto transient=store->read(target);
 check(!transient.savedPassword&&transient.sessionPassword,"Save password off removes the Windows password and keeps only a session secret");
 dialConnectionCredentials(target,args.value);check(std::wstring(args.value.szPassword)==L"UDM-session-only","Automatic dialing receives the entered session-only password");
 change={change.userName,"",false,true};store->write(target,change);saved=store->read(target);
 check(saved.savedPassword&&!saved.sessionPassword,"Enabling Save password persists the existing session secret in Windows");
 change={change.userName,"",true,false};store->write(target,change);saved=store->read(target);dialConnectionCredentials(target,args.value);
 check(!saved.savedPassword&&!saved.sessionPassword&&args.value.szPassword[0]==0,"Explicit clear removes saved and in-memory passwords");
 change={change.userName,protect("UDM-stale-password"),true,false};store->write(target,change);
 RASCREDENTIALSW passwordUpdate{};passwordUpdate.dwSize=sizeof(passwordUpdate);passwordUpdate.dwMask=RASCM_Password;wcscpy_s(passwordUpdate.szPassword,L"UDM-external-password");
 error=RasSetCredentialsW(fixture.book.c_str(),fixture.name.c_str(),&passwordUpdate,FALSE);SecureZeroMemory(&passwordUpdate,sizeof(passwordUpdate));if(error)throw std::runtime_error("Fixture password update failed: "+std::to_string(error));
 saved=store->read(target);dialConnectionCredentials(target,args.value);
 check(saved.savedPassword&&!saved.sessionPassword&&std::wstring(args.value.szPassword)!=L"UDM-stale-password","A newer Windows saved password supersedes an old UDM session secret");
 change={change.userName,protect("UDM-stale-fixture"),true,false};store->write(target,change);
 RASCREDENTIALSW external{};external.dwSize=sizeof(external);external.dwMask=RASCM_UserName|RASCM_Domain;wcscpy_s(external.szUserName,L"external-fixture");
 error=RasSetCredentialsW(fixture.book.c_str(),fixture.name.c_str(),&external,FALSE);if(error)throw std::runtime_error("Fixture identity update failed: "+std::to_string(error));
 saved=store->read(target);dialConnectionCredentials(target,args.value);
 check(!saved.sessionPassword&&args.value.szPassword[0]==0,"An external Windows identity change cannot reuse a stale session password");
 rejects([&]{store->write({target.name+" missing",target.phonebook},change);},"A missing phone-book entry cannot receive a credential update");
 check(store->read(target).userName=="external-fixture","Rejected credential update leaves the existing fixture identity intact");
 error=RasDeleteEntryW(fixture.book.c_str(),fixture.name.c_str());if(error)throw std::runtime_error("Private RAS cleanup failed: "+std::to_string(error));fixture.created=false;
 rejects([&]{store->read(target);},"Private Windows credential entry is removed after acceptance");
}
