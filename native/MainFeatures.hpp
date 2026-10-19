// Included inside MainWindow: persisted desktop interaction preferences.
 static constexpr int columnCount=12;
 const wchar_t* columnName(int index)const{static const wchar_t* names[]={L"File Name",L"Q",L"Size",L"Status",L"Time left",L"Transfer rate",L"Last Try Date",L"Description",L"Date added",L"Save to",L"Referer",L"Parent web page"};return names[index];}
 Json preferences(){Lock lock(manager.mutex);return manager.state["Settings"];}
 void preference(const char* key,const Json& value){auto p=preferences();p[key]=value;manager.setSettings(p);}
 Json listLayout(){Json widths=Json::array(),order=Json::array();int indices[columnCount]{};table.GetColumnOrderArray(indices,columnCount);for(int i=0;i<columnCount;++i){widths.push_back(MulDiv(table.GetColumnWidth(i),96,windowDpi()));order.push_back(indices[i]);}return {{"Widths",widths},{"Order",order},{"SortColumn",sortColumn},{"Ascending",ascending}};}
 void restoreLayout(){auto p=preferences();auto layout=p.value("ListLayout",Json::object());if(layout.contains("Widths")&&layout["Widths"].is_array()&&layout["Widths"].size()==columnCount)for(int i=0;i<columnCount;++i){int width=std::clamp<int>((int)layout["Widths"][i],0,1200);table.SetColumnWidth(i,MulDiv(width,windowDpi(),96));}if(layout.contains("Order")&&layout["Order"].is_array()&&layout["Order"].size()==columnCount){int order[columnCount]{};std::set<int> seen;for(int i=0;i<columnCount;++i){order[i]=layout["Order"][i];if(order[i]<0||order[i]>=columnCount||!seen.insert(order[i]).second)return;}table.SetColumnOrderArray(columnCount,order);}sortColumn=(int)std::clamp<i64>(num(layout,"SortColumn",-1),-1,columnCount-1);ascending=yes(layout,"Ascending",true);table.sortIndicator(sortColumn,ascending);}
 void rememberLayout(){if(!table.GetSafeHwnd())return;Lock lock(manager.mutex);manager.state["Settings"]["ListLayout"]=listLayout();}
 void customizeColumns(){
  struct ColumnsForm:Form{using Form::Form;std::function<void(NMHDR*)> changed;BOOL OnNotify(WPARAM w,LPARAM l,LRESULT* result)override{if(changed)changed(reinterpret_cast<NMHDR*>(l));return Form::OnNotify(w,l,result);}};
  ColumnsForm d("Columns",236,177,this);d.dialogUnits=true;std::vector<int> order(columnCount),widths(columnCount);std::vector<bool> shown(columnCount);table.GetColumnOrderArray(order.data(),columnCount);for(int i=0;i<columnCount;++i){static const int defaults[]={205,38,72,97,77,91,132,160,132,220,200,200};auto saved=preferences().value("ColumnWidths",Json::array());widths[i]=table.GetColumnWidth(i)>0?MulDiv(table.GetColumnWidth(i),96,windowDpi()):saved.is_array()&&saved.size()==columnCount?(int)saved[i]:defaults[i];shown[i]=table.GetColumnWidth(i)>0;}
  CListCtrl* list=nullptr;CWnd *width=nullptr,*up=nullptr,*down=nullptr,*showColumn=nullptr,*hideColumn=nullptr;
  auto syncVisibility=[&]{if(!list||!showColumn||!hideColumn)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);const bool selected=row>=0&&row<columnCount;showColumn->EnableWindow(selected&&!list->GetCheck(row));hideColumn->EnableWindow(selected&&list->GetCheck(row));};
  bool syncingWidth=false;
  auto syncSelection=[&]{if(!list||!width||!up||!down)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);const bool selected=row>=0&&row<columnCount;width->EnableWindow(selected);up->EnableWindow(selected&&row>0);down->EnableWindow(selected&&row+1<columnCount);syncingWidth=true;width->SetWindowText(cs(selected?std::to_string(widths[order[row]]):std::string()));syncingWidth=false;};
  d.changed=[&](NMHDR* h){if(list&&h&&h->hwndFrom==list->m_hWnd&&h->code==LVN_ITEMCHANGED){const auto item=reinterpret_cast<NMLISTVIEW*>(h);if((item->uChanged&LVIF_STATE)&&((item->uOldState^item->uNewState)&LVIS_SELECTED))syncSelection();syncVisibility();}};
  auto collect=[&]{for(int row=0;row<columnCount;++row)shown[order[row]]=list->GetCheck(row)!=FALSE;};
  auto fill=[&](int selection){list->DeleteAllItems();for(int row=0;row<columnCount;++row){int index=order[row];list->InsertItem(row,columnName(index));list->SetCheck(row,shown[index]);}if(selection>=0)list->SetItemState(selection,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);};
  auto storeWidth=[&](bool validate){
   if(syncingWidth||!list||!width)return;const int row=list->GetNextItem(-1,LVNI_SELECTED);if(row<0)return;
   const auto value=trim(text(width));const int minimum=order[row]==0?40:1;
   if(!std::regex_match(value,std::regex("[0-9]{1,4}"))){if(validate)throw std::runtime_error("Enter a whole-number column width.");return;}
   const int n=std::stoi(value);if(n<minimum||n>1200){if(validate)throw std::runtime_error("Use "+std::to_string(minimum)+" to 1200 pixels.");return;}
   widths[order[row]]=n;
  };
  d.init=[&]{
   d.SetIcon(nullptr,TRUE);d.SetIcon(nullptr,FALSE);d.resizeClient(236,177);
   d.label("Check the columns that you would like visible in this list. Use the Move Up and Move Down buttons to reorder the columns however you like.",7,7,222,25);
   list=d.make<CListCtrl>(WS_TABSTOP|WS_BORDER|LVS_SHOWSELALWAYS|LVS_REPORT|LVS_SINGLESEL|LVS_NOCOLUMNHEADER,7,36,139,87);
   list->SetExtendedStyle(LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT);list->InsertColumn(0,L"Column",LVCFMT_LEFT,d.pixelsX(119));
   auto move=[&](int step){int row=list->GetNextItem(-1,LVNI_SELECTED);if(row<0||row+step<0||row+step>=columnCount)return;storeWidth(true);collect();std::swap(order[row],order[row+step]);fill(row+step);};
   up=d.button("Move Up",158,36,71,[move]{move(-1);});down=d.button("Move Down",158,54,71,[move]{move(1);});
   showColumn=d.button("Show",158,72,71,[&]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0)list->SetCheck(row,TRUE);syncVisibility();});
   hideColumn=d.button("Hide",158,90,71,[&]{int row=list->GetNextItem(-1,LVNI_SELECTED);if(row>=0)list->SetCheck(row,FALSE);syncVisibility();});
   d.button("Reset",158,108,71,[&]{int defaults[]={205,38,72,97,77,91,132,160,132,220,200,200};for(int i=0;i<columnCount;++i){order[i]=i;widths[i]=defaults[i];shown[i]=i<8;}fill(0);});
   d.control(L"STATIC","The selected column should be",SS_RIGHT,7,132,123,10);
   width=d.edit("",136,130,20);width->SendMessage(EM_SETLIMITTEXT,4);d.bindChange(width,[&]{storeWidth(false);});d.label("pixels wide",159,132,66,10);
   d.control(L"STATIC","",SS_ETCHEDHORZ,7,149,222,1);
   d.accept=[&]{storeWidth(true);collect();auto next=preferences();auto layout=listLayout();layout["Order"]=order;for(int i=0;i<columnCount;++i)layout["Widths"][i]=shown[i]?widths[i]:0;next["ColumnWidths"]=widths;next["ListLayout"]=layout;manager.setSettings(next);restoreLayout();d.close();};
   d.defaultButton(d.button("OK",107,155,50,d.accept));d.button("Cancel",179,155,50,[&]{d.close(IDCANCEL);});fill(-1);syncSelection();syncVisibility();

  };d.DoModal();
 }
 void chooseFont(){LOGFONTW value{};font.GetLogFont(&value);CFontDialog d(&value,CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_LIMITSIZE,nullptr,this);d.m_cf.nSizeMin=6;d.m_cf.nSizeMax=18;if(d.DoModal()!=IDOK)return;d.GetCurrentFont(&value);auto p=preferences();p["FontName"]=utf8(value.lfFaceName);p["FontHeight"]=std::clamp((int)(std::abs(value.lfHeight)/scale),8,24);p["FontWeight"]=value.lfWeight;manager.setSettings(p);applyAppearance();}
 void refreshSystemTheme(){uiDark=useDarkUi(yes(preferences(),"DarkMode"),windowsHighContrast());themeFrame(m_hWnd);menuTheme.apply(m_hWnd,menu.GetSafeHmenu(),uiDark,windowDpi());table.theme();themeScrollbars(tree.GetSafeHwnd());status.Invalidate();tree.SetBkColor(uiBackground());tree.SetTextColor(uiForeground());refreshOpenForms();if(basket&&basket->GetSafeHwnd())basket->refreshTheme();RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);}
 afx_msg void OnSysColorChange(){CFrameWnd::OnSysColorChange();if(table.GetSafeHwnd())refreshSystemTheme();}
 afx_msg void OnSettingChange(UINT flags,LPCTSTR section){CFrameWnd::OnSettingChange(flags,section);if(table.GetSafeHwnd())refreshSystemTheme();}
 void applyAppearance(){auto p=preferences();uiDark=useDarkUi(yes(p,"DarkMode"),windowsHighContrast());themeFrame(m_hWnd);menuTheme.apply(m_hWnd,menu.GetSafeHmenu(),uiDark,windowDpi());font.DeleteObject();font.CreateFontW(-px((int)num(p,"FontHeight",11)),0,0,0,(int)num(p,"FontWeight",400),FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,wide(str(p,"FontName","Tahoma")).c_str());for(CWnd* w:{static_cast<CWnd*>(&table),static_cast<CWnd*>(&tree),static_cast<CWnd*>(&categoryHeading),static_cast<CWnd*>(&status),static_cast<CWnd*>(&search),static_cast<CWnd*>(&clipboard)})w->SetFont(&font);table.GetHeaderCtrl()->SetFont(&font);rebuildToolbar();table.SetExtendedStyle(table.GetExtendedStyle()|LVS_EX_GRIDLINES);table.theme();themeScrollbars(tree.GetSafeHwnd());status.Invalidate();tree.SetBkColor(uiBackground());tree.SetTextColor(uiForeground());tree.SetItemHeight((SHORT)px(std::max(17,(int)num(p,"FontHeight",11)+5)));updateTray();CRect area;GetClientRect(&area);layout(area.Width(),area.Height());RedrawWindow(nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_ERASE);refreshOpenForms();if(basket&&basket->GetSafeHwnd())basket->refreshTheme();}
 bool updateTray(){auto choice=str(preferences(),"TrayIcon","Color");tray.hIcon=classicTrayStyle(choice)?classicTrayIcon():AfxGetApp()->LoadIcon(1);if(!tray.hIcon)tray.hIcon=AfxGetApp()->LoadIcon(1);if(choice=="Hidden"){Shell_NotifyIconW(NIM_DELETE,&tray);return true;}const bool added=Shell_NotifyIconW(NIM_ADD,&tray)!=FALSE;const bool modified=Shell_NotifyIconW(NIM_MODIFY,&tray)!=FALSE;return added||modified;}
 afx_msg BOOL OnEraseBkgnd(CDC* dc){CRect area;GetClientRect(&area);dc->FillSolidRect(area,uiDark?uiBackground():GetSysColor(COLOR_BTNFACE));return TRUE;}
 afx_msg HBRUSH OnCtlColor(CDC* dc,CWnd* window,UINT type){if(uiDark&&(type==CTLCOLOR_STATIC||type==CTLCOLOR_EDIT)){dc->SetTextColor(uiForeground());dc->SetBkColor(uiBackground());return (HBRUSH)uiBrush().GetSafeHandle();}return CFrameWnd::OnCtlColor(dc,window,type);}
