#pragma once
#include "Core.hpp"
namespace udm {
inline int queueIndicatorKind(const Json& queue){
 return (str(queue,"Name")=="Main queue"||str(queue,"DefaultQueueRole")=="Download"||str(queue,"DefaultQueueRole")=="Synchronization"?0:1)+(yes(queue,"Synchronize")?2:0)+((yes(queue,"Scheduled")||yes(queue,"RunOnce"))?4:0);
}
inline std::string queueIndicatorDescription(const Json& queue){
 return str(queue,"Name")+" — "+(yes(queue,"Synchronize")?"Synchronization queue":"Download queue")+((yes(queue,"Scheduled")||yes(queue,"RunOnce"))?"; scheduled start":"")+(!yes(queue,"Enabled",true)?"; stopped":"");
}
// Original UDM glyphs: stacked sheets with a clock for a configured start.
inline HBITMAP queueIndicatorBitmap(CDC& screen,int side,int kind){
 CDC dc;dc.CreateCompatibleDC(&screen);CBitmap bitmap;if(!bitmap.CreateCompatibleBitmap(&screen,side,side))throw std::runtime_error("Cannot create queue indicator.");
 auto oldBitmap=dc.SelectObject(&bitmap);dc.FillSolidRect(0,0,side,side,RGB(255,0,255));
 auto p=[&](int value){return MulDiv(value,side,16);};
 const bool sync=(kind&2)!=0;CPen outline(PS_SOLID,std::max(1,p(1)),sync?RGB(29,94,54):RGB(117,83,13));CBrush fill(sync?RGB(98,191,123):RGB(245,201,78));
 auto oldPen=dc.SelectObject(&outline);auto oldBrush=dc.SelectObject(&fill);int count=(kind&1)?2:3;
 for(int i=0;i<count;++i){int offset=3-count+i,x=1+2*offset,y=5-2*offset;dc.Rectangle(p(x),p(y),p(x+8),p(y+10));dc.FillSolidRect(p(x+2),p(y+3),p(4),std::max(1,p(1)),sync?RGB(198,236,208):RGB(255,239,185));dc.FillSolidRect(p(x+2),p(y+6),p(4),std::max(1,p(1)),sync?RGB(198,236,208):RGB(255,239,185));}
 if(kind&4){CBrush face(RGB(249,250,251));CPen hand(PS_SOLID,std::max(1,p(1)),RGB(44,64,84));dc.SelectObject(&face);dc.SelectObject(&hand);dc.Ellipse(p(8),p(8),p(16),p(16));dc.MoveTo(p(12),p(9));dc.LineTo(p(12),p(12));dc.LineTo(p(14),p(13));dc.SelectObject(oldBrush);dc.SelectObject(oldPen);}else{dc.SelectObject(oldBrush);dc.SelectObject(oldPen);}
 dc.SelectObject(oldBitmap);return (HBITMAP)bitmap.Detach();
}
}
