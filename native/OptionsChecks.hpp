#pragma once
#include "OptionsModel.hpp"
#include "BrowserSettings.hpp"
#include "BrowserIdentity.hpp"
static void optionsChecks(const fs::path& root){
 auto original=defaultSettings(),draft=original,live=original;
 draft["Connections"]=16;live["LimitKbps"]=128;live["SuppressProgressDialog"]=true;
 auto merged=mergeOptionsDraft(original,draft,live);
 check(num(merged,"Connections")==16&&num(merged,"LimitKbps")==128&&yes(merged,"SuppressProgressDialog"),"Options Apply keeps unrelated settings changed while the draft was open");
 original["CategoryRememberLast"]={{"Video",false},{"Archives",false}};draft=original;live=original;draft["CategoryRememberLast"]["Video"]=true;live["CategoryRememberLast"]["Archives"]=true;
 merged=mergeOptionsDraft(original,draft,live);check(yes(merged["CategoryRememberLast"],"Video")&&yes(merged["CategoryRememberLast"],"Archives"),"Options merges independent category memory edits");
 original["CategoryPaths"]=legacyDictionary(Json{{"Video","C:\\Video"},{"Archives","C:\\Archives"}});draft=original;live=original;draft["CategoryPaths"]=legacyDictionary(Json{{"Video","C:\\NewVideo"},{"Archives","C:\\Archives"}});live["CategoryPaths"]=legacyDictionary(Json{{"Video","C:\\Video"},{"Archives","C:\\NewArchives"}});
 merged=mergeOptionsDraft(original,draft,live);check(dictionary(merged["CategoryPaths"])["Video"]=="C:\\NewVideo"&&dictionary(merged["CategoryPaths"])["Archives"]=="C:\\NewArchives","Options merges legacy category-path arrays by category rather than replacing the array");
 original.erase("BrandNewTable");draft=original;live=original;draft["BrandNewTable"]={{"user",1}};live["BrandNewTable"]={{"background",2}};merged=mergeOptionsDraft(original,draft,live);check(merged["BrandNewTable"].size()==2,"Options preserves independently added rows in a newly introduced table");
 original={{"One",1},{"Two",2}};draft={{"Two",3}};live={{"One",4},{"Two",2},{"Three",3}};merged=mergeOptionsDraft(original,draft,live);check(!merged.contains("One")&&merged["Two"]==3&&merged["Three"]==3,"Explicit removal and edits survive a three-way settings merge");
 rejects([]{mergeOptionsDraft(Json::array(),Json::object(),Json::object());},"Invalid settings drafts are rejected before mutation");

 auto prefs=defaultSettings();prefs["BrowserCaptureTargets"]={{"msedge.exe",{{"Name","Microsoft Edge"},{"Enabled",false}}},{"chrome.exe",{{"Name","Google Chrome"},{"Enabled",true}}}};validateBrowserSettings(prefs);
 auto edge=browserPreferences(prefs,"MSEDGE.EXE"),chrome=browserPreferences(prefs,"chrome.exe");
 check(!yes(edge,"captureAllowed")&&!yes(edge,"panelEnabled")&&yes(chrome,"captureAllowed")&&yes(chrome,"panelEnabled"),"Disabling Edge leaves Chrome capture and its video panel enabled");
 check(yes(browserPreferences(prefs,"firefox.exe"),"captureAllowed"),"Browsers without an override retain the global capture setting");
 prefs["BrowserCaptureEnabled"]=false;check(!yes(browserPreferences(prefs,"chrome.exe"),"captureAllowed"),"A per-browser enable cannot override global capture disable");
 for(auto key:{"../chrome.exe","Chrome.exe","chrome"}){auto bad=prefs;bad["BrowserCaptureTargets"][key]={{"Name","Bad"},{"Enabled",true}};rejects([&]{validateBrowserSettings(bad);},"Browser entries require normalized executable basenames");}
 auto bad=prefs;bad["BrowserCaptureTargets"]["chrome.exe"]["Enabled"]="false";rejects([&]{validateBrowserSettings(bad);},"Browser enable flags reject string values");
 check(!nativeBrowserExecutable().empty(),"Native host identity obtains a real parent executable from the process snapshot");

 Manager manager(root/L"options-workflows");prefs=manager.state["Settings"];prefs["DownloadFolder"]=utf8((root/L"options-downloads").wstring());prefs["CategoryFolders"]=false;manager.setSettings(prefs);
 auto incoming=[&](const char* name){return manager.receive({{"action","add"},{"url",std::string("https://example.test/")+name},{"filename",name}});};
 auto info=incoming("info.bin");check(str(info->data,"Status")=="Awaiting confirmation","Default browser handoff still waits for Download File Info");
 prefs["SkipBrowserFileInfo"]=true;manager.setSettings(prefs);auto direct=incoming("direct.bin");check(str(direct->data,"Status")=="Queued","Skip File Info still queues a normal browser download immediately");
 prefs["BrowserDownloadLater"]=true;manager.setSettings(prefs);auto later=incoming("later.bin");check(str(later->data,"Status")=="Paused"&&yes(later->data,"QueueMember"),"Queue-only browser capture creates a paused queue member even when File Info is skipped");
 prefs["SkipBrowserFileInfo"]=false;manager.setSettings(prefs);check(str(incoming("later-with-dialog.bin")->data,"Status")=="Awaiting confirmation","Showing File Info takes precedence over the remembered queue-only preference");
 prefs["SkipBrowserFileInfo"]=true;manager.setSettings(prefs);
 Json plan={{"type","hls"},{"height",720},{"audioExpected",true},{"tracks",Json::array({{{"kind","video"},{"segments",Json::array({{{"url","https://media.example.test/part.ts"}}})}}})}};
 auto media=manager.receive({{"action","adaptive"},{"url","https://example.test/player"},{"filename","queued-video"},{"plan",plan}});check(str(media->data,"Status")=="Paused","Queue-only mode applies to HLS/DASH browser capture as well as files");
 auto queue=defaultQueue("Later");queue["Enabled"]=false;manager.setQueue(queue);manager.configure(later,{{"Queue","Later"}});manager.pause(later);
 check(str(later->data,"Status")=="Paused"&&str(later->data,"Queue")=="Later","Selecting a queue retains paused status until processing is explicitly started");
 manager.queueRun("Later",true);check(str(later->data,"Status")=="Queued"&&yes(later->data,"QueueOrigin"),"Start queue processing prepares the selected queue's paused members");manager.pause(later);

 prefs=manager.state["Settings"];prefs["CustomCategories"]={"Research","Unmatched"};prefs["CategoryTypeOverrides"]={{"Archives","zip"},{"Research","iso pdf"},{"Unmatched","*"}};prefs["CategoryRememberLast"]={{"Research",true},{"Video",false}};
 manager.setSettings(prefs);auto iso=manager.add("https://example.test/a.ISO");check(str(iso->data,"Category")=="Research","Custom category file types take precedence over built-in extensions");
 check(str(manager.add("https://example.test/a.rar")->data,"Category")=="Unmatched","Removing a built-in extension sends it to the selected wildcard category");
 check(str(manager.add("https://example.test/a.mp4")->data,"Category")=="Video","Wildcard category does not swallow unchanged built-in file types");
 auto folder=root/L"remembered-research";rememberCategoryDestination(prefs,"Research",folder);rememberCategoryDestination(prefs,"Video",root/L"not-remembered");
 check(categoryFolder(prefs,"Research")==utf8(folder.wstring())&&!dictionary(prefs["CategoryPaths"]).contains("Video"),"Remember-last-folder updates only opted-in categories");
 auto before=prefs;rejects([&]{rememberCategoryDestination(prefs,"Research",L"relative");},"Remembered category destination rejects relative paths");check(prefs==before,"Rejected remembered destination leaves settings unchanged");
 manager.setSettings(prefs);check(str(manager.add("https://example.test/b.pdf")->data,"Folder")==utf8(folder.wstring()),"New files actually use the remembered category folder");
 auto blocker=manager.root/L"state.json.tmp";fs::create_directory(blocker);before=manager.state["Settings"];rejects([&]{manager.editCategory("Research","Renamed","iso pdf","",utf8(folder.wstring()));},"Category rename surfaces storage failure");check(manager.state["Settings"]==before,"Failed rename restores both new category maps");fs::remove(blocker);
 manager.editCategory("Research","Renamed","iso pdf","",utf8(folder.wstring()));check(yes(manager.state["Settings"]["CategoryRememberLast"],"Renamed")&&!manager.state["Settings"]["CategoryTypeOverrides"].contains("Research"),"Category rename moves folder-memory and extension preferences together");
 manager.editCategory("Renamed","Renamed","txt","",utf8(folder.wstring()));check(categoryExtensions(manager.state["Settings"],"Renamed")=="txt","Editing a renamed category updates its extension override instead of retaining stale types");
 manager.editCategory("Renamed","Renamed","txt","restricted.example",utf8(folder.wstring()));
 check(str(manager.add("https://elsewhere.example/private.txt")->data,"Category")!="Renamed","Adding a host restriction clears the old global extension override");
 check(str(manager.add("https://files.restricted.example/private.txt")->data,"Category")=="Renamed","Host-restricted category still routes matching subdomains");
 manager.deleteCategory("Renamed");check(!manager.state["Settings"]["CategoryRememberLast"].contains("Renamed")&&!manager.state["Settings"]["CategoryTypeOverrides"].contains("Renamed"),"Deleting a category removes its folder-memory and type overrides");
 prefs=manager.state["Settings"];prefs["QueuePromptLater"]=false;prefs["QueuePromptBatch"]=false;prefs["BrowserCaptureTargets"]={{"msedge.exe",{{"Name","Edge"},{"Enabled",false}}}};manager.setSettings(prefs);{Manager reopened(manager.root);check(!yes(reopened.state["Settings"],"QueuePromptLater",true)&&!yes(reopened.state["Settings"],"QueuePromptBatch",true)&&!yes(browserPreferences(reopened.state["Settings"],"msedge.exe"),"captureAllowed"),"Queue prompt and browser settings persist after reopening the database");}
 for(auto key:{"BrowserDownloadLater","QueuePromptLater","QueuePromptBatch"}){auto bad=prefs;bad[key]="false";rejects([&]{manager.setSettings(bad);},"New download dialog preferences reject non-boolean values");}
 bad=prefs;bad["CategoryRememberLast"]["Missing"]=true;rejects([&]{manager.setSettings(bad);},"Category memory cannot refer to a deleted category");
 bad=prefs;bad["CategoryTypeOverrides"]["Video"]="mp4 ../bin";rejects([&]{manager.setSettings(bad);},"Category file types reject path syntax");
}
