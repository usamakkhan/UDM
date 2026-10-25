#pragma once
#include "UpdateManifest.hpp"
// Included by Tests.cpp after ContentSniffChecks.hpp, whose SniffFixture serves the manifest.
// A throwaway ECDSA P-256 key, generated in process, that signs manifests the way tools/publish-manifest.ps1 does.
struct TestUpdateKey {
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_KEY_HANDLE key=nullptr;unsigned char publicXY[64]{};
 TestUpdateKey(){
  if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)<0||BCryptGenerateKeyPair(algorithm,&key,256,0)<0||BCryptFinalizeKeyPair(key,0)<0)throw std::runtime_error("Cannot create a test update key.");
  ULONG size=0;BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,nullptr,0,&size,0);std::vector<unsigned char> blob(size);
  if(BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,blob.data(),size,&size,0)<0||size<sizeof(BCRYPT_ECCKEY_BLOB)+64)throw std::runtime_error("Cannot export the test update key.");
  std::memcpy(publicXY,blob.data()+sizeof(BCRYPT_ECCKEY_BLOB),64);
 }
 ~TestUpdateKey(){if(key)BCryptDestroyKey(key);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
 std::string envelope(const udm::Json& manifest,const char* algorithmName="ecdsa-p256-sha256"){
  auto text=manifest.dump();udm::Bytes data(text.begin(),text.end());auto digest=udm::sha256Raw(data);udm::Bytes signature(64);ULONG written=0;
  if(BCryptSignHash(key,nullptr,digest.data(),32,signature.data(),64,&written,0)<0||written!=64)throw std::runtime_error("Cannot sign the test manifest.");
  return udm::Json{{"algorithm",algorithmName},{"keyId","test"},{"manifest",udm::b64(data)},{"signature",udm::b64(signature)}}.dump();
 }
};
static void updateChecks(const udm::fs::path& root){
 using namespace udm;
 TestUpdateKey key;
 Json manifest={{"version","9.9.9"},{"browserVersion","0.99.0"},{"published","2026-10-24"},{"notes","https://github.com/usamakkhan/UDM/releases/tag/UDM-9.9.9"},
  {"setup",{{"url","https://github.com/usamakkhan/UDM/releases/download/UDM-9.9.9/UDM-9.9.9-Browser-0.99.0-Setup-x64.exe"},{"sha256",std::string(64,'a')},{"size",12345678}}}};
 auto parsed=parseUpdateManifest(key.envelope(manifest),key.publicXY,"1.0.4");
 check(parsed.newer&&parsed.version=="9.9.9"&&parsed.setupSize==12345678&&parsed.setupSha256==std::string(64,'a')&&updateSetupFileName(parsed)=="UDM-9.9.9-Browser-0.99.0-Setup-x64.exe","A signed manifest verifies, parses and names a newer setup");
 check(!parseUpdateManifest(key.envelope(manifest),key.publicXY,"9.9.9").newer&&!parseUpdateManifest(key.envelope(manifest),key.publicXY,"10.0.0").newer,"An equal or newer installed version is not offered the update");
 {auto envelope=Json::parse(key.envelope(manifest));auto changed=manifest;changed["version"]="9.9.10";auto text=changed.dump();envelope["manifest"]=b64(Bytes(text.begin(),text.end()));
  rejects([&]{parseUpdateManifest(envelope.dump(),key.publicXY);},"A manifest changed after signing is rejected");}
 {TestUpdateKey other;rejects([&]{parseUpdateManifest(key.envelope(manifest),other.publicXY);},"A manifest signed by another key is rejected");}
 {unsigned char none[64]{};rejects([&]{parseUpdateManifest(key.envelope(manifest),none);},"A build without an update key verifies nothing");check(!updateKeyConfigured(none)&&updateKeyConfigured(key.publicXY),"The built-in key is recognized as configured only when set");}
 rejects([&]{parseUpdateManifest(key.envelope(manifest,"rsa-sha256"),key.publicXY);},"Only the ECDSA P-256 envelope is accepted");
 rejects([&]{parseUpdateManifest("not json",key.publicXY);},"Malformed manifest text is rejected");
 struct Bad{const char* name;std::function<void(Json&)> change;};
 const std::vector<Bad> bad={
  {"A setup served over plain HTTP is rejected",[](Json& m){m["setup"]["url"]="http://github.com/usamakkhan/UDM/releases/download/x/UDM-Setup.exe";}},
  {"A setup on another host is rejected",[](Json& m){m["setup"]["url"]="https://example.com/usamakkhan/UDM/releases/download/x/UDM-Setup.exe";}},
  {"A setup outside the repository's releases is rejected",[](Json& m){m["setup"]["url"]="https://github.com/someone/else/releases/download/x/UDM-Setup.exe";}},
  {"A setup address with a query is rejected",[](Json& m){m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download/x/UDM-Setup.exe?x=1";}},
  {"A setup without a full SHA-256 is rejected",[](Json& m){m["setup"]["sha256"]="abc";}},
  {"A setup with an implausible size is rejected",[](Json& m){m["setup"]["size"]=0;}},
  {"Release notes outside GitHub are rejected",[](Json& m){m["notes"]="https://example.com/notes";}},
  {"A manifest without a version is rejected",[](Json& m){m.erase("version");}},
  {"A manifest with a malformed version is rejected",[](Json& m){m["version"]="nine";}},
  {"A setup address that climbs out of the releases path is rejected",[](Json& m){m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download/../../x/y/releases/download/t/UDM-Setup.exe";}},
  {"A setup address with an empty segment is rejected",[](Json& m){m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download//UDM-Setup.exe";}},
  {"A setup address deeper than tag/file is rejected",[](Json& m){m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download/t/sub/UDM-Setup.exe";}},
  {"A setup address without a file is rejected",[](Json& m){m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download/t/";}},
 };
 for(const auto& b:bad){auto m=manifest;b.change(m);rejects([&]{parseUpdateManifest(key.envelope(m),key.publicXY);},b.name);}
 {auto m=manifest;m["setup"]["url"]="https://github.com/usamakkhan/UDM/releases/download/UDM-9.9.9/weird%20name";check(updateSetupFileName(parseUpdateManifest(key.envelope(m),key.publicXY))=="UDM-9.9.9-Setup-x64.exe","A setup address without a plain .exe name gets a generated file name");}
 Json settings=Json::object();const auto now=epoch();
 check(updateCheckDue(settings,now),"A fresh profile is due for a startup check");
 settings["UpdateCheckedAt"]=date(now);check(!updateCheckDue(settings,now+3600*1000),"A check within the last 20 hours is not repeated");
 settings["UpdateCheckedAt"]=date(now-21LL*3600*1000);check(updateCheckDue(settings,now),"The startup check is due again after 20 hours");
 settings["CheckForUpdates"]=false;check(!updateCheckDue(settings,now),"The opt-out disables the startup check");
 settings["UpdateCheckedAt"]="garbage";settings["CheckForUpdates"]=true;check(updateCheckDue(settings,now),"An unreadable last-check date counts as due");
 check(updateSkipped(Json{{"SkippedUpdateVersion","9.9.9"}},"9.9.9")&&!updateSkipped(Json{{"SkippedUpdateVersion","9.9.9"}},"9.9.10")&&!updateSkipped(Json::object(),"9.9.9"),"Skipping a version silences that version only");
 {SniffFixture server({{"/latest.json",{"application/json",key.envelope(manifest)}},{"/page.json",{"application/json","{\"manifest\":\"\",\"signature\":\"\",\"algorithm\":\"ecdsa-p256-sha256\"}"}}});
  Json prefs=defaultSettings();prefs["ProxyMode"]="Connect directly";Cancel cancel;
  auto fetched=fetchUpdateManifest(prefs,cancel,server.url("/latest.json"),key.publicXY);check(fetched.version=="9.9.9"&&fetched.newer,"The manifest is fetched over HTTP and verified");
  rejects([&]{fetchUpdateManifest(prefs,cancel,server.url("/missing.json"),key.publicXY);},"A missing manifest reports that none is published");
  rejects([&]{fetchUpdateManifest(prefs,cancel,server.url("/page.json"),key.publicXY);},"An empty envelope from the host is rejected");}
 // The launch gate: this test executable is unsigned, so it stands in for a tampered or unsigned setup.
 wchar_t module[MAX_PATH]{};GetModuleFileNameW(nullptr,module,MAX_PATH);const fs::path self=module;
 check(!setupSignatureProblem(self).empty(),"An unsigned file is refused by the signature gate with a reason");
 const auto copy=root/L"update-setup-fixture.exe";fs::copy_file(self,copy,fs::copy_options::overwrite_existing);const auto stage=root/L"update-stage";
 rejects([&]{launchDownloadedSetup(copy,std::string(64,'0'),nullptr,stage);},"A setup whose SHA-256 differs from the manifest is not run");
 rejects([&]{launchDownloadedSetup(copy,fileHash(copy),nullptr,stage);},"An unsigned setup that matches the manifest is still not run");
 rejects([&]{launchDownloadedSetup(root/L"absent-setup.exe",std::string(64,'0'),nullptr,stage);},"A missing setup file is reported");
 check(fs::exists(copy),"A refused setup is kept for inspection rather than deleted");
 check(!fs::exists(stage)||fs::is_empty(stage),"A refused setup leaves no staged copy behind");
 {std::error_code ec;fs::create_directories(stage/L"old");std::ofstream(stage/L"old"/L"stale.exe")<<"x";
  rejects([&]{verifyDownloadedSetup(copy,std::string(64,'0'),stage);},"Verification stages a private copy before hashing it");
  check(!fs::exists(stage/L"old"),"Earlier staged copies are cleared when a new setup is verified");}
}
