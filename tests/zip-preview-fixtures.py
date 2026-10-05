from pathlib import Path
import zipfile,json,struct,zlib,sys
root=Path(sys.argv[1]).resolve();root.mkdir(parents=True,exist_ok=True)
for name,large in [('small',False),('large',True)]:
 with zipfile.ZipFile(root/(name+'.zip'),'w',compression=zipfile.ZIP_STORED) as z:
  z.writestr('folder/readme.txt','Preview does not extract this file.')
  z.writestr('unicode/文件-é.txt','unicode entry')
  z.writestr('empty/',b'')
  if large:z.writestr('payload.bin',b'z'*(12*1024*1024));z.comment=b'c'*65535
 with zipfile.ZipFile(root/(name+'.zip')) as z:
  assert z.testzip() is None
  (root/(name+'.json')).write_text(json.dumps([{'Name':i.filename,'Size':i.file_size,'Packed':i.compress_size,'Encrypted':False,'Method':i.compress_type} for i in z.infolist()],ensure_ascii=False),encoding='utf-8')
with zipfile.ZipFile(root/'empty.zip','w'):pass
# ZIP64 representation with a virtual stored payload; only ranges requested are served.
size=5*1024**3;start=6*1024**3;name=b'huge.bin';extra=struct.pack('<HHQQ',1,16,size,size)
central=struct.pack('<IHHHHHHIIIHHHHHII',0x02014b50,45,45,0,0,0,0,0,0xffffffff,0xffffffff,len(name),len(extra),0,0,0,0,0)+name+extra
end64=struct.pack('<IQHHIIQQQQ',0x06064b50,44,45,45,0,0,1,1,len(central),start)
locator=struct.pack('<IIQI',0x07064b50,0,start+len(central),1)
end=struct.pack('<IHHHHIIH',0x06054b50,0,0,65535,65535,0xffffffff,0xffffffff,0)
(root/'zip64-tail.bin').write_bytes(central+end64+locator+end)
(root/'zip64.json').write_text(json.dumps({'start':start,'size':start+len(central+end64+locator+end),'entries':[{'Name':'huge.bin','Size':size,'Packed':size,'Encrypted':False,'Method':0}]}))
print('Independent Python ZIP archives verified; virtual >4GB ZIP64 directory generated')
