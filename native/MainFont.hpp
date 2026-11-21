// Included inside MainWindow. Fonts and their native bindings belong to the UI thread.
#define UDM_MAIN_FONT_REUSE_AVAILABLE
 struct MainFontKey {
  int height=0,weight=0;std::wstring face;
  friend bool operator==(const MainFontKey& a,const MainFontKey& b)noexcept{return a.height==b.height&&a.weight==b.weight&&a.face==b.face;}
 };
 std::optional<MainFontKey> mainFontKey;
#ifdef UDM_TOOLBAR_COMPONENT_TEST
 std::function<void()> mainFontCreationObserver;
 std::function<HFONT(const MainFontKey&)> mainFontFactory;
#endif
 void applyMainFont(const Json& settings){
  MainFontKey key{-px((int)num(settings,"FontHeight",11)),(int)num(settings,"FontWeight",400),wide(str(settings,"FontName","Tahoma"))};
  // CreateFont uses at most LF_FACESIZE-1 characters for its face name.
  if(key.face.size()>=LF_FACESIZE)key.face.resize(LF_FACESIZE-1);
  CFont replacement,previous;
  if(!font.GetSafeHandle()||GetObjectType(font.GetSafeHandle())!=OBJ_FONT||!mainFontKey||!(*mainFontKey==key)){
#ifdef UDM_TOOLBAR_COMPONENT_TEST
   if(mainFontCreationObserver)mainFontCreationObserver();
   if(mainFontFactory){if(auto handle=mainFontFactory(key))replacement.Attach(handle);}else
#endif
   replacement.CreateFontW(key.height,0,0,0,key.weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,key.face.c_str());
   if(!replacement.GetSafeHandle())throw std::runtime_error("Cannot create the download-list font.");
   if(font.GetSafeHandle())previous.Attach(font.Detach());font.Attach(replacement.Detach());mainFontKey=std::move(key);
  }
  const auto handle=font.GetSafeHandle();
  for(CWnd* control:{static_cast<CWnd*>(&table),static_cast<CWnd*>(&tree),static_cast<CWnd*>(&categoryHeading),static_cast<CWnd*>(&status),static_cast<CWnd*>(&search),static_cast<CWnd*>(&searchLabel),static_cast<CWnd*>(&searchFind),static_cast<CWnd*>(&searchSettings),static_cast<CWnd*>(&searchClose),static_cast<CWnd*>(&clipboard),static_cast<CWnd*>(table.GetHeaderCtrl()),static_cast<CWnd*>(&toolbar)}){
   if(control&&control->GetSafeHwnd()&&reinterpret_cast<HFONT>(control->SendMessage(WM_GETFONT))!=handle)control->SetFont(&font);
  }
  // previous stays alive until every control has stopped borrowing it.
 }
