from pathlib import Path
import struct,sys,json
root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=True)
def bmp(name,w,h,color,top=False,bits=24):
 width=w*12;stride=(width*bits+31)//32*4;body=bytearray(stride*h)
 for y in range(h):
  for x in range(width):
   at=y*stride+x*(bits//8);b,g,r=color
   if x%w==0 or y==0:r=g=b=192
   body[at:at+3]=bytes((b,g,r))
   if bits==32:body[at+3]=128
 header=struct.pack('<2sIHHI',b'BM',54+len(body),0,0,54)+struct.pack('<IiiHHIIiiII',40,width,-h if top else h,1,bits,0,len(body),0,0,0,0)
 (root/name).write_bytes(header+body)
for name,w,h,color,top,bits in [
 ('normal.bmp',40,30,(31,71,111),False,24),('hot.bmp',40,30,(45,135,180),False,24),('disabled.bmp',40,30,(145,145,145),False,24),
 ('small.bmp',16,16,(111,31,71),True,24),('hdpi.bmp',60,45,(71,111,31),False,32),('hdpiHot.bmp',60,45,(31,111,180),False,32)]:
 bmp(name,w,h,color,top,bits)
(root/'fixture.tbi').write_text('v=3\nname=UDM fixture\nlarge=normal.bmp\nlargeHot=hot.bmp\nlargeDisabled=disabled.bmp\nsmall=small.bmp\nhdpi=hdpi.bmp\nhdpiHot=hdpiHot.bmp\n',encoding='utf-8')
print(json.dumps({'fixture':str(root/'fixture.tbi'),'files':7}))

