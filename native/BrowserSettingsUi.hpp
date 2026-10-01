#pragma once
#include "BrowserCustomizationUi.hpp"
namespace udm {
inline bool browserIntegration(CWnd* owner,Json& prefs){
 Form d("Browser integration",300,179,owner);d.dialogUnits=true;Json draft=prefs;
 d.init=[&]{auto enabled=d.check("Allow browser download capture",yes(draft,"BrowserCaptureEnabled",true),12,12,276,10);
 d.label("Automatic capture also needs to be enabled in the browser extension.",12,28,276,22);
 d.button("Keys...",12,58,84,[&]{browserKeysDialog(&d,draft);});d.button("Browser menus...",105,58,87,[&]{browserMenusDialog(&d,draft);});d.button("Video panels...",201,58,87,[&]{browserPanelsDialog(&d,draft);});
 d.label("Address exceptions (one HTTP/HTTPS pattern per line):",12,83,276,10);auto exceptions=d.edit(str(draft,"CaptureExcludedUrls"),12,98,276,49,false,true);
 d.accept=[&,enabled,exceptions]{draft["BrowserCaptureEnabled"]=d.checked(enabled);draft["CaptureExcludedUrls"]=text(exceptions);validateBrowserSettings(draft);prefs=draft;d.close();};d.defaultButton(d.button("OK",176,158,50,d.accept));d.button("Cancel",238,158,50,[&]{d.close(IDCANCEL);});
 };return d.DoModal()==IDOK;
}
}
