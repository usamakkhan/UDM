// Included inside udm after progressBytes.
class ProgressRateGraph:public CWnd {
 DECLARE_MESSAGE_MAP()
 using Clock=std::chrono::steady_clock;
 static constexpr auto history=std::chrono::seconds(30);
 static constexpr auto interval=std::chrono::milliseconds(500);
 struct Sample {Clock::time_point time;std::optional<double> rate;};
 std::deque<Sample> samples;
 std::optional<int> pointerX;
 static std::wstring speedText(double rate){return wide(progressBytes(rate)+"/s");}
 void draw(CDC& dc,CRect frame){
  if(frame.IsRectEmpty())return;
  const bool contrast=windowsHighContrast();
  const auto surface=contrast?GetSysColor(COLOR_WINDOW):uiDark?RGB(25,32,43):RGB(248,251,255);
  const auto foreground=contrast?GetSysColor(COLOR_WINDOWTEXT):uiDark?RGB(230,238,247):RGB(29,48,68);
  const auto grid=contrast?foreground:uiDark?RGB(60,75,92):RGB(206,220,235);
  const auto line=contrast?GetSysColor(COLOR_HIGHLIGHT):uiDark?RGB(105,210,255):RGB(15,104,173);
  const auto fill=uiDark?RGB(31,61,81):RGB(216,235,252);
  dc.FillSolidRect(frame,surface);dc.Draw3dRect(frame,grid,grid);
  const int saved=dc.SaveDC();
  if(auto font=GetFont())dc.SelectObject(font);
  dc.SetBkMode(TRANSPARENT);dc.SetTextColor(foreground);
  TEXTMETRICW font{};dc.GetTextMetrics(&font);
  const int row=std::max(12L,font.tmHeight+2),pad=std::max(5,row/3);
  CRect plot=frame;plot.DeflateRect(pad,row+pad);
  if(plot.Width()<2||plot.Height()<2){dc.RestoreDC(saved);return;}
  const auto end=samples.empty()?Clock::time_point{}:samples.back().time;
  const auto begin=end-history;
  const auto x=[&](Clock::time_point t){return plot.left+static_cast<int>(plot.Width()*std::chrono::duration<double>(t-begin).count()/history.count());};
  double peak=0;
  for(const auto& sample:samples)if(sample.rate)peak=std::max(peak,*sample.rate);
  const double scale=std::max(peak,1.0);
  const auto y=[&](double rate){return plot.bottom-1-static_cast<int>((plot.Height()-2)*rate/scale);};
  for(int i=0;i<=4;++i){
   const int tick=plot.left+(plot.Width()-1)*i/4;
   dc.FillSolidRect(tick,plot.top,1,plot.Height(),grid);
  }
  for(int i=0;i<=2;++i)dc.FillSolidRect(plot.left,plot.top+(plot.Height()-1)*i/2,plot.Width(),1,grid);
  CPen trace(PS_SOLID,std::max(1,row/8),line);
  CBrush area(fill);
  dc.SelectObject(&trace);dc.SelectObject(&area);
  std::vector<POINT> points;
  const auto paintTrace=[&]{
   if(points.empty())return;
   if(points.size()>1){
    if(!contrast){
     auto filled=points;
     filled.push_back({points.back().x,plot.bottom-1});filled.push_back({points.front().x,plot.bottom-1});
     auto pen=dc.SelectStockObject(NULL_PEN);dc.Polygon(filled.data(),static_cast<int>(filled.size()));dc.SelectObject(pen);
    }
    dc.Polyline(points.data(),static_cast<int>(points.size()));
   }else dc.FillSolidRect(points.front().x,points.front().y,2,2,line);
   points.clear();
  };
  const Sample* selected=nullptr;
  for(const auto& sample:samples){
   if(!sample.rate){paintTrace();continue;}
   points.push_back({std::clamp<LONG>(x(sample.time),plot.left,plot.right-1),y(*sample.rate)});
   if(pointerX&&(!selected||std::abs(x(sample.time)-*pointerX)<std::abs(x(selected->time)-*pointerX)))selected=&sample;
  }
  paintTrace();
  if(selected){
   const int mark=std::min<int>(plot.right-1,x(selected->time));
   dc.FillSolidRect(mark,plot.top,1,plot.Height(),line);
  }
  const auto peakLabel=peak>0?L"Peak " + speedText(peak):L"Transfer rate";
  std::wstring value;
  if(selected){
   const auto age=std::chrono::duration_cast<std::chrono::seconds>(end-selected->time).count();
   value=speedText(*selected->rate)+L"  (-"+std::to_wstring(age)+L" s)";
  }else if(!samples.empty()&&samples.back().rate)value=speedText(*samples.back().rate);
  CRect caption(frame.left+pad,frame.top+pad,frame.right-pad,plot.top);
  dc.DrawTextW(peakLabel.c_str(),-1,&caption,DT_LEFT|DT_SINGLELINE|DT_NOPREFIX|DT_END_ELLIPSIS);
  if(!value.empty()&&dc.GetTextExtent(peakLabel.c_str()).cx+dc.GetTextExtent(value.c_str()).cx+pad<caption.Width())
   dc.DrawTextW(value.c_str(),-1,&caption,DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX);
  CRect axis(plot.left,plot.bottom+pad,plot.right,frame.bottom);
  dc.DrawTextW(L"30 s ago",-1,&axis,DT_LEFT|DT_SINGLELINE);
  dc.DrawTextW(L"Latest",-1,&axis,DT_RIGHT|DT_SINGLELINE);
  if(std::none_of(samples.begin(),samples.end(),[](const Sample& sample){return sample.rate.has_value();}))
   dc.DrawTextW(L"Waiting for transfer samples",-1,&plot,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  dc.RestoreDC(saved);
 }
 afx_msg void OnPaint(){
  CPaintDC screen(this);CRect frame;GetClientRect(&frame);
  CDC buffer;CBitmap bitmap;
  if(frame.IsRectEmpty()||!buffer.CreateCompatibleDC(&screen)||!bitmap.CreateCompatibleBitmap(&screen,frame.Width(),frame.Height())){draw(screen,frame);return;}
  auto old=buffer.SelectObject(&bitmap);draw(buffer,frame);
  screen.BitBlt(0,0,frame.Width(),frame.Height(),&buffer,0,0,SRCCOPY);buffer.SelectObject(old);
 }
 afx_msg LRESULT OnPrintClient(WPARAM dc,LPARAM){CRect frame;GetClientRect(&frame);draw(*CDC::FromHandle(reinterpret_cast<HDC>(dc)),frame);return 0;}
 afx_msg void OnMouseMove(UINT flags,CPoint point){
  if(!pointerX){TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,m_hWnd,0};TrackMouseEvent(&track);}
  if(pointerX!=point.x){pointerX=point.x;Invalidate(FALSE);}
  CWnd::OnMouseMove(flags,point);
 }
 afx_msg void OnMouseLeave(){pointerX.reset();Invalidate(FALSE);}
public:
 BOOL Create(DWORD style,const RECT& rect,CWnd* parent,UINT id){return CWnd::Create(AfxRegisterWndClass(0),L"Transfer rate history",style,rect,parent,id);}
 void record(Clock::time_point now,std::optional<double> rate){
  if(rate&&(!std::isfinite(*rate)||*rate<0))rate.reset();
  if(samples.empty()&&!rate)return;
  if(!samples.empty()&&samples.back().rate.has_value()==rate.has_value()&&(!rate||now-samples.back().time<interval))return;
  samples.push_back({now,rate});
  while(now-samples.front().time>history)samples.pop_front();
  if(IsWindowVisible())Invalidate(FALSE);
 }
};
BEGIN_MESSAGE_MAP(ProgressRateGraph,CWnd)
 ON_WM_PAINT()
 ON_MESSAGE(WM_PRINTCLIENT,OnPrintClient)
 ON_WM_MOUSEMOVE()
 ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()
