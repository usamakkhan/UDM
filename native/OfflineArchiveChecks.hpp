#pragma once
#include "OfflineArchive.hpp"
static void offlineArchiveChecks(const fs::path& root){
 OfflineArchiveLayout layout("https://example.test/docs/start.html");
 check(layout.file("https://example.test/docs/start.html",".html",true)=="docs/start.html","Offline archive preserves the nested starting page");
 check(layout.file("https://example.test/docs/",".html")=="docs/index.html","Directory pages get a local index document");
 auto query1=layout.file("https://example.test/assets/image?id=1",".svg"),query2=layout.file("https://example.test/assets/image?id=2",".svg");
 check(query1=="assets/image.svg"&&query2=="assets/image (1).svg","Different query resources have unique local files");
 auto case1=layout.file("https://example.test/Case/a.png",".png"),case2=layout.file("https://example.test/case/a.png",".png");
 check(case1=="Case/a.png"&&case2=="case (1)/a.png","Case-sensitive server folders remain distinct on Windows");
 check(layout.file("https://example.test/case/b.png",".png")=="case (1)/b.png","Colliding folders consistently reuse their allocated directory");
 auto file=layout.file("https://example.test/occupied.css",".css"),nested=layout.file("https://example.test/occupied.css/child.html",".html");
 check(file=="occupied.css"&&nested=="occupied.css (1)/child.html","A website file cannot occupy another resource's directory");
 check(layout.file("https://example.test/UDM-offline-report.json",".bin")=="UDM-offline-report (1).json","A site resource cannot overwrite the offline report");
 check(layout.file("https://example.test/index.html",".html")=="index (1).html","A later site index cannot replace the archive entry point");
 auto localExternal=layout.file("https://example.test/_external/site.svg",".svg"),remote=layout.file("https://cdn.test/assets/site.svg",".svg");
 check(localExternal=="_external (1)/site.svg"&&remote=="_external/https_cdn.test_443/assets/site.svg","External assets have a namespace separate from the original site");
 check(layout.file("http://cdn.test/assets/site.svg",".svg")!=remote&&layout.file("https://cdn.test:444/assets/site.svg",".svg")!=remote,"External origins with different protocols or ports stay separate");
 check(layout.file("https://example.test/%E6%96%87%20space/a%23b%25.svg",".svg")==u8"文 space/a#b%.svg","Unicode and encoded URL path characters retain readable file names");
 check(layout.file("https://example.test/a%3Ab/icon.svg",".svg")!=layout.file("https://example.test/a_b/icon.svg",".svg"),"Sanitized server folders cannot silently merge");
 check(layout.file("https://example.test/repeated//slashes/style",".css")=="repeated/slashes/style.css","Empty URL path components do not change folder identity");
 OfflineArchiveLayout rootLayout("https://example.test/");
 check(rootLayout.file("https://example.test/",".html",true)=="index.html","A root starting page uses the reserved entry point");
 check(offlineArchiveReference("index.html","docs/start.html")=="docs/start.html"&&offlineArchiveReference("docs/next.html","docs/start.html")=="start.html","Root and sibling document links are relative to their containing page");
 check(offlineArchiveReference("styles/sub/theme.css",u8"文 space/a#b%.svg")=="../../%E6%96%87%20space/a%23b%25.svg","Nested CSS references preserve Unicode and URL-sensitive path characters");
 check(offlineArchiveReference("a/page.html","a/page.html")=="page.html","Self references point at the current local file");
 for(const auto& name:std::vector<std::string>{"","/root.html","../outside","a/../b","a/./b","a//b","a/","C:/x","a\\b","a:b","a/nul.txt","a/CON","a/trailing.","a/ space",std::string("bad\0name",8),std::string("\xff",1)})
  rejects([&]{validateOfflineArchivePath(name);},"Archive paths reject traversal, devices, invalid encoding and ambiguous Windows names");
 rejects([&]{layout.file("https://example.test/a/%2f/b.png",".png");},"Encoded path separators cannot enter an archive layout");
 std::map<std::string,std::string> files={{"https://example.test/docs/next.html","docs/next.html"},{"https://example.test/styles/theme.css","styles/theme.css"},{"https://example.test/images/pic.svg","images/pic.svg"},{"https://example.test/docs/","docs/index.html"}};
 auto html=rewriteSiteDocument("<head><base href='/docs/'><link href='/styles/theme.css'></head><a href='next.html#part'>Next</a><a href='#same'>Base anchor</a><img src='/images/pic.svg'>","https://example.test/landing",false,files,"other/start.html");
 check(html.find("../styles/theme.css")!=std::string::npos&&html.find("../docs/next.html#part")!=std::string::npos&&html.find("../images/pic.svg")!=std::string::npos,"HTML links are rewritten relative to each archived page");
 check(html.find("../docs/index.html#same")!=std::string::npos&&html.find("<base")==std::string::npos,"Fragment links retain the original base document after removing base tags");
 auto css=rewriteSiteDocument("@import '/styles/theme.css';p{background:url('/images/pic.svg#x)')}", "https://example.test/styles/main.css",true,files,"styles/main.css");
 check(css.find("'theme.css'")!=std::string::npos&&css.find("../images/pic.svg#x%29")!=std::string::npos,"Stylesheet links and CSS-sensitive fragments remain valid CSS");
 rejects([&]{validateOfflineProject({{"StartUrl","https://example.test/"},{"OriginalSubfolders","yes"}});},"Offline jobs reject malformed folder-layout values");
 Manager manager(root/L"offline-layout-models");auto folder=root/L"offline-layout-files";
 auto project=Json{{"Id",guid()},{"Name","Nested offline site"},{"Template","Offline website (ZIP)"},{"StartUrl","https://example.test/docs/start.html"},{"SaveMode","Folder"},{"Folder",utf8(folder.wstring())},{"OriginalSubfolders",true},{"Links",Json::array()}};
 check(manager.addProject(project,"Main queue",true)==1,"Offline ZIP projects accept original subfolders with an explicit destination");
 auto job=manager.jobs.back();
 check(job->target().parent_path()==folder&&yes(job->data["OfflineProject"],"OriginalSubfolders"),"The archive stays in the selected folder and stores its internal layout option");
 Manager restored(manager.root);check(yes(restored.jobs[0]->data["OfflineProject"],"OriginalSubfolders"),"Offline hierarchy persists across application restart");
 auto catalog=exportCatalog(manager,{job});Manager imported(root/L"offline-layout-catalog");importCatalog(imported,catalog,utf8((root/L"offline-layout-imported").wstring()));
 check(imported.jobs[0]->data["OfflineProject"]==job->data["OfflineProject"],"Offline hierarchy survives catalog export and import");
}
