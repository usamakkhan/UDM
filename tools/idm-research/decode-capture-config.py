"""Decode the static idmfc.dat configuration without loading IDM code.
Derived from build-11 IDMNetMon.dll functions 10004e90, 10005080,
10005210, 10005540 and its config reader 100169c0. Unsigned arithmetic
is explicit. This does not read or modify IDM license/account state.
"""
import argparse,hashlib,json,pathlib,struct
MASK=0xffffffff
def u(n):return n&MASK
def ror(x,n):return u((x>>n)|(x<<(32-n)))
def schedule(counter):
    initial=[0x4267e26c,0x2d60319d,0xddbb4b82,0x766db5df,0x35ac7b09]
    w=[counter]+[0]*15
    for i in range(16,80):w.append(w[i-3]^w[i-8]^w[i-14]^w[i-16])
    a,b,c,d,e=initial
    for i in range(80):
        if i<20:f=(b&c)|((~b)&d);k=0x5a827999
        elif i<40:f=b^c^d;k=0x6ed9eba1
        elif i<60:f=(b&c)|(b&d)|(c&d);k=0x8f1bbcdc
        else:f=b^c^d;k=0xca62c1d6
        a,b,c,d,e=u(ror(a,27)+f+e+w[i]+k),a,ror(b,2),c,d
    return [u(x+y) for x,y in zip(initial,[a,b,c,d,e])]
def table(offset,n):
    cache={};out=[]
    for i in range(offset,offset+n):
        if i//5 not in cache:cache[i//5]=schedule(i//5)
        out.append(cache[i//5][i%5])
    return out
def stream_block(counter,T,S,R):
    out=[]
    for p in range(4):
        a=R[p*4]^counter
        b=u((R[p*4+1]^ror(counter,8))+T[(a>>2)&511])
        c=u((R[p*4+2]^ror(counter,16))+T[(b>>2)&511])
        d=u((R[p*4+3]^ror(counter,24))+T[(c>>2)&511])
        a=u(ror(a,9)+T[(d>>2)&511])
        b=u(ror(b,9)+T[(a>>2)&511]);save_b=ror(b,9)
        c=u(ror(c,9)+T[(b>>2)&511]);save_c=ror(c,9)
        d=u(ror(d,9)+T[(c>>2)&511]);save_d=ror(d,9)
        save_a=u(ror(a,9)+T[(d>>2)&511])
        b=u(save_b+T[(save_a>>2)&511]);rb=ror(b,9)
        c=u(save_c+T[(b>>2)&511]);rc=ror(c,9)
        d=u(save_d+T[(c>>2)&511]);rd=ror(d,9)
        a=u(ror(save_a,9)+T[(d>>2)&511]);b=rb;c=rc;d=rd
        for j in range(16):
            ra=ror(a,9)
            b=u(b+T[(a&0x7fc)//4])^ra;rb=ror(b,9)
            c=u((c^T[(b&0x7fc)//4])+rb);rc=ror(c,9)
            idx1=((a&0x7fc)+c)&0x7fc
            d=u(d+T[idx1//4])^rc;rd=ror(d,9)
            idx2=((b&0x7fc)+d)&0x7fc
            a=u((T[idx2//4]^ra)+rd)
            idx3=(idx1+a)&0x7fc;rb=rb^T[idx3//4];b=ror(rb,9)
            idx4=(idx2+rb)&0x7fc;rc=u(rc+T[idx4//4]);c=ror(rc,9)
            rd=rd^T[((rc+idx3)>>2)&511];d=ror(rd,9)
            a=u(ror(a,9)+T[((idx4+rd)>>2)&511])
            out.extend([u(S[j*4]+b),S[j*4+1]^c,u(S[j*4+2]+d),S[j*4+3]^a])
            a=u(a+(save_d if j%2==0 else save_a));c=u(c+(save_b if j%2==0 else save_c))
    return struct.pack('<256I',*out)
def transform(raw):
    T,S,R=table(0,512),table(0x1000,256),table(0x2000,16);out=bytearray()
    for pos in range(0,len(raw),1024):out.extend(x^y for x,y in zip(raw[pos:pos+1024],stream_block(pos//1024,T,S,R)))
    return bytes(out)
def main():
    ap=argparse.ArgumentParser();ap.add_argument('input');ap.add_argument('--output',required=True);a=ap.parse_args();raw=pathlib.Path(a.input).read_bytes();decoded=transform(raw)
    if not decoded.lstrip().startswith(b'FORMAT='):raise ValueError('decoded configuration header not recognized; no success claimed')
    if transform(decoded)!=raw:raise ValueError('round-trip failed')
    out=pathlib.Path(a.output);out.mkdir(parents=True,exist_ok=True);(out/'decoded-config.txt').write_bytes(decoded)
    text=decoded.decode('utf-8-sig');lines=text.splitlines();active=[];inside=False
    for line in lines[1:]:
        line=line.strip()
        if line.startswith('/*'):inside=True
        if inside:
            if '*/' in line:inside=False
            continue
        if line and not line.startswith('//'):active.append(line)
    import re,collections
    rules=[]
    for line in active:
        m=re.match(r'^(?:(?P<flags>[A-Z]+)?(?:#(?P<extension>\w+))?!)?(?P<pattern>.+)$',line)
        rules.append({'flags':m['flags'] or '', 'extensionOverride':m['extension'], 'pattern':m['pattern'], 'captures':[{'tag':tag,'pattern':value} for tag,value in re.findall(r'<([A-Z]):(.*?)>',m['pattern'])]})
    report={'sha256':hashlib.sha256(raw).hexdigest(),'decodedSha256':hashlib.sha256(decoded).hexdigest(),'bytes':len(raw),'header':lines[0],'activeLines':len(active),'roundTripVerified':True,'readFunction':'IDMNetMon.dll build 11:0x100169c0','flagCounts':dict(collections.Counter(flag for rule in rules for flag in rule['flags'])),'consumerEvidence':{'nativeDriverConfig':'0x10030c20 serializes object+0x1240 under tag 0x12 before DeviceIoControl(0x12c008)','nativeBrowserConfig':'0x1003a910 serializes object+0x1240 under tag 0x12 with format/version gating','browserSha256':'3590408894c701937ffa51048c14c3e6d1b534e9616b76d99e4af523925294d7','browserFunctions':['yc (rule compiler)','Gc (rule evaluation)'],'confirmedSyntax':{'G':'request method GET filter','P':'request method POST filter','F_capture':'filename-like capture; W derives extension from the captured text','T_capture':'extension-like capture used directly','T_flag':'allows request-path extension fallback','hashWordPrefix':'literal extension fallback'},'remaining':'Other flags are retained verbatim; human meanings and complete runtime effects unverified.'},'scope':'Configuration syntax and selected static consumers; runtime effect not tested.'}
    (out/'rules.json').write_text(json.dumps(rules,indent=2)+'\n',encoding='utf-8')
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print(json.dumps(report,indent=2));print('\n'.join(lines[:12]))
if __name__=='__main__':main()
