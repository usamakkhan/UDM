// Included inside udm before OptionsUi.hpp.
// UDM's own 40x40 page icons for the Configuration dialog (IDM shows a themed icon per page).
// Flat two-tone glyphs drawn with GDI at four times the size and box-filtered down, so they
// carry alpha, stay crisp at any DPI and follow the light or dark theme without image assets.
enum class OptionsPageIcon {General,FileTypes,SaveTo,Downloads,Connection,Proxy,Logins,DialUp,Sounds};
inline void optionsIconShapes(HDC dc,double f,OptionsPageIcon page,COLORREF accent,COLORREF ink,COLORREF paper){
 auto X=[f](double v){return (int)std::lround(v*f);};
 CPen accentPen(PS_SOLID,X(3),accent),inkPen(PS_SOLID,X(3),ink),paperPen(PS_SOLID,X(3),paper);CBrush accentBrush(accent),inkBrush(ink),paperBrush(paper);
 auto use=[dc](CPen& pen,CBrush& brush){SelectObject(dc,pen);SelectObject(dc,brush);};
 auto outline=[dc](CPen& pen){SelectObject(dc,pen);SelectObject(dc,GetStockObject(NULL_BRUSH));};
 auto poly=[dc,X](std::initializer_list<std::pair<double,double>> points){std::vector<POINT> list;for(auto& p:points)list.push_back({X(p.first),X(p.second)});Polygon(dc,list.data(),(int)list.size());};
 auto box=[dc,X](double x1,double y1,double x2,double y2){Rectangle(dc,X(x1),X(y1),X(x2),X(y2));};
 auto line=[dc,X](double x1,double y1,double x2,double y2){MoveToEx(dc,X(x1),X(y1),nullptr);LineTo(dc,X(x2),X(y2));};
 auto disc=[dc,X](double cx,double cy,double r){Ellipse(dc,X(cx-r),X(cy-r),X(cx+r),X(cy+r));};
 auto arc=[dc,X](double cx,double cy,double r,double x1,double y1,double x2,double y2){Arc(dc,X(cx-r),X(cy-r),X(cx+r),X(cy+r),X(x1),X(y1),X(x2),X(y2));};
 switch(page){
 case OptionsPageIcon::General: // browser window with a download arrow
  use(inkPen,paperBrush);RoundRect(dc,X(4),X(6),X(36),X(34),X(4),X(4));use(inkPen,inkBrush);box(4,6,36,13);
  use(accentPen,accentBrush);box(18,15,22,24);poly({{13,23},{27,23},{20,30}});break;
 case OptionsPageIcon::FileTypes: // document with a folded corner and three lines
  use(inkPen,paperBrush);poly({{9,4},{25,4},{32,11},{32,36},{9,36}});use(accentPen,accentBrush);poly({{25,4},{32,11},{25,11}});
  line(14,18,27,18);line(14,23,27,23);line(14,28,22,28);break;
 case OptionsPageIcon::SaveTo: // folder
  use(inkPen,inkBrush);poly({{4,8},{15,8},{18,11},{36,11},{36,34},{4,34}});use(accentPen,accentBrush);box(4,15,36,34);break;
 case OptionsPageIcon::Downloads: // arrow into a tray
  outline(inkPen);line(7,25,7,33);line(7,33,33,33);line(33,33,33,25);
  use(accentPen,accentBrush);box(18,5,22,20);poly({{11,19},{29,19},{20,28}});break;
 case OptionsPageIcon::Connection: // three linked nodes
  outline(inkPen);line(20,9,8,31);line(20,9,32,31);line(8,31,32,31);use(accentPen,accentBrush);disc(20,9,5);disc(8,31,5);disc(32,31,5);break;
 case OptionsPageIcon::Proxy: // shield with a check mark
  use(accentPen,accentBrush);poly({{20,4},{35,9},{35,19},{20,36},{5,19},{5,9}});outline(paperPen);line(12,20,18,26);line(18,26,28,14);break;
 case OptionsPageIcon::Logins: // key
  use(accentPen,accentBrush);disc(13,20,8);box(20,18,36,22);box(29,22,32,27);box(33,22,36,26);use(paperPen,paperBrush);disc(13,20,3);break;
 case OptionsPageIcon::DialUp: // modem with signal arcs
  use(inkPen,inkBrush);RoundRect(dc,X(6),X(25),X(34),X(35),X(3),X(3));outline(accentPen);
  arc(20,24,7,27,24,13,24);arc(20,24,13,33,24,7,24);arc(20,24,19,39,24,1,24);use(accentPen,accentBrush);disc(20,24,2.5);break;
 case OptionsPageIcon::Sounds: // speaker with two waves
  use(accentPen,accentBrush);poly({{6,16},{13,16},{21,9},{21,31},{13,24},{6,24}});outline(inkPen);
  arc(21,20,7,26,25,26,15);arc(21,20,12,30,29,30,11);break;
 }
}
// Renders the page glyph at `size` pixels square with premultiplied alpha; the caller owns the icon.
inline HICON optionsPageIcon(OptionsPageIcon page,int size,bool dark){
 if(size<8||size>256)throw std::runtime_error("Invalid options icon size.");
 const COLORREF accent=dark?RGB(118,166,236):RGB(36,85,166),ink=dark?RGB(222,224,228):RGB(70,70,74),paper=dark?RGB(52,54,60):RGB(255,255,255),key=RGB(255,0,255);
 const int k=4,s=size*k;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=s;info.bmiHeader.biHeight=-s;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void* bits=nullptr;auto big=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!big||!bits){if(big)DeleteObject(big);throw std::runtime_error("Cannot allocate the options icon.");}
 {auto dc=CreateCompatibleDC(nullptr);auto previous=SelectObject(dc,big);RECT all{0,0,s,s};CBrush fill(key);FillRect(dc,&all,fill);optionsIconShapes(dc,s/40.0,page,accent,ink,paper);SelectObject(dc,previous);DeleteDC(dc);}
 info.bmiHeader.biWidth=size;info.bmiHeader.biHeight=-size;void* out=nullptr;auto color=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&out,nullptr,0);if(!color||!out){DeleteObject(big);if(color)DeleteObject(color);throw std::runtime_error("Cannot allocate the options icon.");}
 auto source=static_cast<const DWORD*>(bits);auto target=static_cast<DWORD*>(out);const DWORD keyPixel=0xff00ff; // DIB pixels are 0x00RRGGBB
 for(int y=0;y<size;++y)for(int x=0;x<size;++x){unsigned r=0,g=0,b=0,n=0;
  for(int yy=0;yy<k;++yy)for(int xx=0;xx<k;++xx){auto p=source[(y*k+yy)*s+x*k+xx]&0xffffff;if(p==keyPixel)continue;r+=(p>>16)&255;g+=(p>>8)&255;b+=p&255;++n;}
  const unsigned total=k*k;target[y*size+x]=n?((n*255/total)<<24)|((r/total)<<16)|((g/total)<<8)|(b/total):0;
 }
 DeleteObject(big);std::vector<BYTE> maskBits((size_t)((size+15)/16)*2*size);auto mask=CreateBitmap(size,size,1,1,maskBits.data());HICON icon=nullptr;
 if(mask){ICONINFO value{};value.fIcon=TRUE;value.hbmColor=color;value.hbmMask=mask;icon=CreateIconIndirect(&value);DeleteObject(mask);}DeleteObject(color);
 if(!icon)throw std::runtime_error("Windows could not create the options icon.");return icon;
}
