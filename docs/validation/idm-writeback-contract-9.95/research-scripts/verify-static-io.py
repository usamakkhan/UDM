from pathlib import Path
import struct,json,hashlib
root=Path(r'<research>');exe=Path(r'<idm-install>\IDMan.exe');raw=exe.read_bytes();digest=hashlib.sha256(raw).hexdigest().upper();assert digest=='03CC62E9ADB77A380F9DC12F67CCAAEE5106F12844AA73CE32C914DDD16D607C';pe=struct.unpack_from('<I',raw,0x3c)[0];assert raw[:2]==b'MZ' and raw[pe:pe+4]==b'PE\0\0';machine,nsections=struct.unpack_from('<HH',raw,pe+4);optsize=struct.unpack_from('<H',raw,pe+20)[0];opt=pe+24;assert machine==0x14c and struct.unpack_from('<H',raw,opt)[0]==0x10b;base=struct.unpack_from('<I',raw,opt+28)[0];sections=[]
for i in range(nsections):
 at=opt+optsize+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',raw,at+8);sections.append((va,rs,rp))
def offset(rva):
 for va,size,pointer in sections:
  if va<=rva<va+size:return pointer+rva-va
 raise ValueError('RVA has no source bytes')
def data(va,n):
 pos=offset(va-base);return raw[pos:pos+n]
def word(rva):return struct.unpack_from('<I',raw,offset(rva))[0]
def string(rva):
 pos=offset(rva);return raw[pos:raw.index(0,pos)].decode('ascii')
imports={};rva=struct.unpack_from('<I',raw,opt+104)[0]
for i in range(200):
 desc=struct.unpack_from('<IIIII',raw,offset(rva+i*20));original,_,_,name,iat=desc
 if not name:break
 dll=string(name)
 for j in range(5000):
  value=word((original or iat)+j*4)
  if not value:break
  if not value&0x80000000:imports[base+iat+j*4]={'module':dll,'name':string(value+2)}
assert imports[0x6944a0]['name']=='CreateFileW' and imports[0x6943a8]['name']=='FlushFileBuffers'
assert data(0x56fc4c,16).hex()=='6a006a006a026a006a0168000000c050'
assert data(0x56fc5c,6).hex()=='ff15a0446900'
assert data(0x570290,11).hex()=='8b411450ff15a8436900c3'
assert 0x55ac74+struct.unpack('<i',data(0x55ac70,4))[0]==0x570290
area=Path(r'<repo>\benchmarks\idm-complete-20260927\ghidra\03cc62e9adb7__IDMan.exe');artifactHashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest().upper() for p in [area/'all-functions.c',area/'functions.tsv',area/'instructions.asm',area/'summary.json']}
summary={'passed':True,'targetSha256':digest,'machine':'x86','imageBase':hex(base),'peImportsIndependentlyParsed':True,'rawCreateArgumentAndCallBytesVerified':True,'rawPartFlushHandleOffsetAndImportCallVerified':True,'rawCallerRelativeTargetVerified':True,'partCreate':{'procedure':'0x56fc40','access':0xc0000000,'share':1,'disposition':2,'flags':0,'iat':hex(0x6944a0)},'partFlush':{'procedure':'0x570290','handleOffset':20,'iat':hex(0x6943a8)},'directFlushCallerAddresses':['0x455d10','0x570290','0x61e776','0x6550a6'],'staticCoverageDoesNotProveAllRuntimePaths':True,'existingArtifactHashes':artifactHashes,'providerSessionClosed':json.loads((root/'terminal.json').read_text(encoding='utf-8'))['closed'],'globalOrInstalledProgramChanged':False};(root/'static-verification.json').write_text(json.dumps(summary,indent=2),encoding='utf-8');print(json.dumps(summary))
