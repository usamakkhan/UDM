import json,pathlib,subprocess,sys,zlib
exe,root=sys.argv[1:3];root=pathlib.Path(root);root.mkdir();checks=[]
def sync(n):return bytes([(n>>21)&127,(n>>14)&127,(n>>7)&127,n&127])
def frame(v,name,data,flags=0):
 if v==2:return name.encode()+len(data).to_bytes(3,'big')+data
 return name.encode()+(sync(len(data)) if v==4 else len(data).to_bytes(4,'big'))+bytes([0,flags])+data
def tag(v,data,flags=0):
 h=b'ID3'+bytes([v,0,flags])+sync(len(data));return h+data+(b'3DI'+h[3:] if v==4 and flags&16 else b'')
def run(name,data,title='',artist='',album='',source=None):
 path=root/(name+'.mp3');path.write_bytes(data)
 p=subprocess.run([exe,str(path)],capture_output=True,text=True,encoding='utf-8',timeout=5);assert p.returncode==0,(name,p.stderr)
 actual=json.loads(p.stdout);expected=dict(title=title,artist=artist,album=album)
 assert all(actual[k]==v for k,v in expected.items()),(name,actual,expected)
 if source is not None:assert actual['source']==source,(name,actual)
 checks.append(name)
v1=bytearray(128);v1[:3]=b'TAG';v1[3:11]=b'OldTitle';v1[33:42]=b'OldArtist';v1[63:71]=b'OldAlbum'
for v in (2,3,4):
 title,artist,album=('TT2','TP1','TAL') if v==2 else ('TIT2','TPE1','TALB')
 payload=frame(v,title,b'\0Title')+frame(v,artist,b'\0Artist')+frame(v,album,b'\0Album')
 run(f'v{v}-basic',tag(v,payload),'Title','Artist','Album',f'ID3v2.{v}')
 run(f'v{v}-latin1',tag(v,frame(v,title,b'\0caf\xe9')),'café')
 for endian,bom in [('le',b'\xff\xfe'),('be',b'\xfe\xff')]:
  run(f'v{v}-utf16-{endian}',tag(v,frame(v,title,b'\1'+bom+('Music '+chr(0x1f3b5)).encode('utf-16-'+endian))),('Music '+chr(0x1f3b5)))
 run(f'v{v}-first-value',tag(v,frame(v,title,b'\0One\0Two')),'One')
 run(f'v{v}-truncated',tag(v,payload)[:-1])
 run(f'v{v}-padding',tag(v,payload+bytes(20)),'Title','Artist','Album')
 run(f'v{v}-mixed',tag(v,frame(v,title,b'\0New'))+v1,'New','OldArtist','OldAlbum',f'ID3v2.{v} + ID3v1')
 run(f'v{v}-bad-utf16',tag(v,frame(v,title,b'\1\xff\xfe\x00\xd8')))
 run(f'v{v}-oversize-text',tag(v,frame(v,title,b'\0'+b'x'*4097)))
 run(f'v{v}-unknown-frame',tag(v,frame(v,'XYZ' if v==2 else 'XXXX',b'other')+payload),'Title','Artist','Album')
 run(f'v{v}-duplicate',tag(v,frame(v,title,b'\0First')+frame(v,title,b'\0Second')),'First')
 if v<4:
  raw=frame(v,title,b'\0A\xff\xe0B');run(f'v{v}-unsync',tag(v,raw.replace(b'\xff',b'\xff\0'),128),'AÿàB')
run('v4-utf8',tag(4,frame(4,'TIT2',b'\3'+ ('日本 '+chr(0x1f3b5)).encode())),('日本 '+chr(0x1f3b5)))
run('v4-utf16be',tag(4,frame(4,'TIT2',b'\2'+'日本'.encode('utf-16-be'))),'日本')
for name,bad in [('overlong',b'\xc0\xaf'),('surrogate',b'\xed\xa0\x80'),('range',b'\xf4\x90\x80\x80'),('truncated',b'\xe2\x82')]:run('utf8-'+name,tag(4,frame(4,'TIT2',b'\3'+bad)))
run('v4-footer',tag(4,frame(4,'TIT2',b'\3Title'),16),'Title')
run('v4-bad-footer',tag(4,frame(4,'TIT2',b'\3Title'),16)[:-1]+b'X')
run('v3-extended',tag(3,(6).to_bytes(4,'big')+bytes(6)+frame(3,'TIT2',b'\0Title'),64),'Title')
run('v4-extended',tag(4,sync(6)+b'\1\0'+frame(4,'TIT2',b'\3Title'),64),'Title')
for v in (3,4):
 for name,flag in [('compressed',128 if v==3 else 8),('encrypted',64 if v==3 else 4)]:run(f'v{v}-{name}',tag(v,frame(v,'TIT2',b'\0No',flag)+frame(v,'TPE1',b'\0Artist')),artist='Artist')
 run(f'v{v}-grouped',tag(v,frame(v,'TIT2',b'\7\0Title',32 if v==3 else 64)),'Title')
run('v4-dli',tag(4,frame(4,'TIT2',sync(6)+b'\3Title',1)),'Title')
run('v4-bad-dli',tag(4,frame(4,'TIT2',sync(99)+b'\3Title',1)))
run('v4-frame-unsync',tag(4,frame(4,'TIT2',b'\0A\xff\0\xe0B',2)),'AÿàB')
run('v4-tag-unsync',tag(4,frame(4,'TIT2',b'\0A\xff\0\xe0B'),128),'AÿàB')
run('fallback',b'ID3\x05\0\0'+sync(0)+v1,'OldTitle','OldArtist','OldAlbum','ID3v1')
run('v2-compressed',tag(2,frame(2,'TT2',b'\0No'),64))
run('bad-sync',b'ID3\4\0\0\x80\0\0\0')
run('huge-tag',b'ID3\4\0\0'+sync(5*1024*1024))
run('v4-bad-frame-size',tag(4,b'TIT2\x80\0\0\1\0\0X'))
appended=tag(4,frame(4,'TIT2',b'\3Appended'),16)
run('appended-only',b'\xff\xfb\x90\0'+bytes(500)+appended,'Appended',source='ID3v2.4')
run('appended-with-v1',bytes(500)+appended+v1,'Appended','OldArtist','OldAlbum','ID3v2.4 + ID3v1')
run('appended-bad-footer',bytes(500)+appended[:-1]+b'X')
run('appended-bad-header',bytes(500)+b'X'+appended[1:])
run('appended-impossible-offset',b'3DI\4\0\x10'+sync(1000))
run('prepended-priority',tag(4,frame(4,'TIT2',b'\3First'))+bytes(500)+appended,'First')
run('huge-complete-tag',b'ID3\4\0\0'+sync(4194305)+bytes(4194305))
for v in (3,4):
 def compressed(data,declared=None,group=False,encoded=None):
  length=len(data) if declared is None else declared
  size=length.to_bytes(4,'big') if v==3 else sync(length)
  prefix=(size+(b'\7' if group else b'')) if v==3 else ((b'\7' if group else b'')+size)
  flags=(128 if v==3 else 9)|((32 if v==3 else 64) if group else 0)
  return frame(v,'TIT2',prefix+(zlib.compress(data) if encoded is None else encoded),flags)
 run(f'v{v}-zlib',tag(v,compressed(b'\0Title')),'Title')
 run(f'v{v}-zlib-group',tag(v,compressed(b'\0Title',group=True)),'Title')
 run(f'v{v}-zlib-unicode',tag(v,compressed(b'\1\xff\xfe'+('Music '+chr(0x1f3b5)).encode('utf-16-le'))),'Music '+chr(0x1f3b5))
 for label,options in [('short',{'declared':5}),('long',{'declared':7}),('zero',{'declared':0}),('limit',{'declared':65537}),('corrupt',{'encoded':b'broken'}),('truncated',{'encoded':zlib.compress(b'\0Title')[:-1]}),('trailing',{'encoded':zlib.compress(b'\0Title')+b'x'})]:
  run(f'v{v}-zlib-{label}',tag(v,compressed(b'\0Title',**options)+frame(v,'TPE1',b'\0Artist')),artist='Artist')
 run(f'v{v}-zlib-bomb',tag(v,compressed(b'\0'+b'x'*100000,declared=6)))
(root/'results.json').write_text(json.dumps({'passed':True,'checks':checks},indent=2))
print(f'PASS {len(checks)} ID3v2/version/encoding/boundary checks')
