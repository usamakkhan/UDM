// Owned fixture support only. Legacy interactions wait for their fast injected
// reads without pumping unrelated UI timers; held-read scenarios do not use it.
struct DialReadTestAccess {
 static bool childOpen(const Options& o){return o.dialChild;}
 static bool ready(const Options& o){return o.settingsReady&&!o.startupLookup.pending()&&o.dialListReady&&(!o.dialCredentials||!o.dialCredentials->readPending());}
 static void wait(HWND w){auto o=dynamic_cast<Options*>(CWnd::FromHandlePermanent(w));if(!o)return;auto end=GetTickCount64()+3000;for(;;){if(o->pulse)o->pulse();if(ready(*o))return;if(GetTickCount64()>end)throw std::runtime_error("Fast fixture connection lookup deadline");Sleep(1);}}
};
