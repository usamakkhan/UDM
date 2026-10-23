"""Dump RT_DIALOG templates (DLGTEMPLATE and DLGTEMPLATEEX) from a PE file to JSON.

Usage: python -I dump-dialogs.py <exe> <output.json>

Read-only: the executable is parsed as data. Output mirrors docs/reference/
completion-dialog.json: per dialog id, title, size in dialog units, font, and
each control's class, title and geometry.
"""
import json,struct,sys
RT_DIALOG=5
def read(data,fmt,at):return struct.unpack_from(fmt,data,at)
def sections(data):
    pe=read(data,'<I',0x3c)[0];count=read(data,'<H',pe+6)[0];opt=read(data,'<H',pe+20)[0];base=pe+24+opt
    result=[]
    for i in range(count):
        s=base+i*40;name=data[s:s+8].rstrip(b'\0').decode('latin-1');vsize,va,rsize,raw=read(data,'<IIII',s+8)
        result.append((name,va,vsize,raw,rsize))
    return result
def rva_to_off(secs,rva):
    for _,va,vsize,raw,rsize in secs:
        if va<=rva<va+max(vsize,rsize):return raw+(rva-va)
    raise ValueError('rva outside sections: %x'%rva)
def resource_dir(data,secs):
    pe=read(data,'<I',0x3c)[0];magic=read(data,'<H',pe+24)[0];dd=pe+24+(96 if magic==0x10b else 112)
    rva,size=read(data,'<II',dd+2*8);return rva_to_off(secs,rva),rva
def walk(data,base,off,depth,path,out):
    named,ids=read(data,'<HH',off+12)
    for i in range(named+ids):
        e=off+16+i*8;name,offset=read(data,'<II',e)
        ident=name&0x7fffffff if name&0x80000000 else name
        if offset&0x80000000:walk(data,base,base+(offset&0x7fffffff),depth+1,path+[ident],out)
        else:
            rva,size=read(data,'<II',base+offset);out.append((path+[ident],rva,size))
def sz(data,at):
    chars=[]
    while True:
        c=read(data,'<H',at)[0];at+=2
        if c==0:break
        chars.append(c)
    return ''.join(chr(c) for c in chars),at
def name_or_ordinal(data,at):
    first=read(data,'<H',at)[0]
    if first==0:return '',at+2
    if first==0xffff:return {'ordinal':read(data,'<H',at+2)[0]},at+4
    return sz(data,at)
def parse(data,at,size):
    end=at+size;sig,ver=read(data,'<HH',at)
    ex=ver==1 and sig==0xffff
    if ex:
        helpId,exStyle,style,count,x,y,w,h=read(data,'<IIIHhhhh',at+4);at+=4+4+4+4+2+8
    else:
        style,exStyle,count,x,y,w,h=read(data,'<IIHhhhh',at);at+=18
    menu,at=name_or_ordinal(data,at);cls,at=name_or_ordinal(data,at);title,at=sz(data,at);font=None
    if style&0x40:
        if ex:
            points,weight,italic,charset=read(data,'<HHBB',at);at+=6;face,at=sz(data,at);font={'points':points,'face':face,'weight':weight}
        else:
            points=read(data,'<H',at)[0];at+=2;face,at=sz(data,at);font={'points':points,'face':face}
    controls=[]
    for _ in range(count):
        at=(at+3)&~3
        if ex:
            chelp,cex,cstyle,cx,cy,cw,ch,cid=read(data,'<IIIhhhhI',at);at+=24
        else:
            cstyle,cex,cx,cy,cw,ch,cid=read(data,'<IIhhhhH',at);at+=18
        ccls,at=name_or_ordinal(data,at);ctitle,at=name_or_ordinal(data,at)
        extra=read(data,'<H',at)[0];at+=2+(extra if not ex else (extra if extra else 0))
        controls.append({'id':cid,'class':ccls,'title':ctitle,'x':cx,'y':cy,'width':cw,'height':ch,'style':cstyle,'exStyle':cex})
    return {'extended':ex,'style':style,'exStyle':exStyle,'x':x,'y':y,'width':w,'height':h,'menu':menu,'class':cls,'title':title,'font':font,'controls':controls}
def main():
    exe,out=sys.argv[1],sys.argv[2];data=open(exe,'rb').read();secs=sections(data);base,_=resource_dir(data,secs)
    entries=[];walk(data,base,base,0,[],entries);dialogs=[]
    for path,rva,size in entries:
        if path[0]!=RT_DIALOG:continue
        try:d=parse(data,rva_to_off(secs,rva),size)
        except Exception as e:dialogs.append({'id':path[1],'language':path[2],'error':str(e)});continue
        d['id']=path[1];d['language']=path[2];dialogs.append(d)
    json.dump({'source':exe,'dialogs':dialogs},open(out,'w',encoding='utf-8'),indent=1)
    print(len(dialogs),'dialogs')
if __name__=='__main__':main()
