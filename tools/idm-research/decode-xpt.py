"""Read legacy Mozilla XPT interfaces; never load the native browser components.

Format: https://www-archive.mozilla.org/scriptable/typelib_file
Directory offset correction: https://bugzilla.mozilla.org/show_bug.cgi?id=575343
"""
import argparse, json, pathlib, struct, uuid

NAMES = ['int8','int16','int32','int64','uint8','uint16','uint32','uint64',
         'float','double','boolean','char','wchar','void','iid','domstring',
         'cstring','wstring','interface','interface_is','array','string_size',
         'wstring_size','utf8string','cstring_class','astring','jsval']

class Reader:
    def __init__(self, b):
        self.b=b; self.used=set(); self.pos=0
    def take(self, n):
        start=self.pos; self.pos+=n
        if self.pos>len(self.b): raise ValueError('XPT field exceeds file')
        self.used.update(range(start,self.pos)); return self.b[start:self.pos]
    def number(self, fmt): return struct.unpack('>'+fmt,self.take(struct.calcsize('>'+fmt)))[0]
    def string(self, off):
        if off==0:return None
        start=self.pool+off-1
        if not 0<=start<len(self.b):raise ValueError('Invalid string offset')
        end=self.b.index(b'\0',start); self.used.update(range(start,end+1))
        return self.b[start:end].decode('utf-8')
    def typ(self):
        prefix=self.number('B');tag=prefix&31
        if tag>=len(NAMES):raise ValueError('Unsupported XPT type '+str(tag))
        t={'tag':tag,'name':NAMES[tag],'flags':prefix&224}
        if tag==18:t['interfaceIndex']=self.number('H')
        elif tag==19:t['argumentIndex']=self.number('B')
        elif tag in (20,21,22):
            t['sizeArgument']=self.number('B');t['lengthArgument']=self.number('B')
            if tag==20:t['element']=self.typ()
        return t
    def param(self):
        flags=self.number('B');return {'flags':flags,'in':bool(flags&128),'out':bool(flags&64),'retval':bool(flags&32),'type':self.typ()}
    def parse(self):
        if self.take(16)!=b'XPCOM\nTypeLib\r\n\x1a':raise ValueError('Not XPT')
        major=self.number('B');minor=self.number('B');count=self.number('H')
        length=self.number('I');directory=self.number('I');self.pool=self.number('I')
        if major!=1 or length!=len(self.b):raise ValueError('Unsupported version or length mismatch')
        # Actual Mozilla writer uses a one-based directory pointer; pool is zero-based.
        annotation=self.take(directory-1-self.pos).hex();entries=[]
        for index in range(count):
            self.pos=directory-1+28*index
            iid=str(uuid.UUID(bytes=self.take(16)));name=self.string(self.number('I'));namespace=self.string(self.number('I'));off=self.number('I')
            entry={'index':index+1,'iid':iid,'name':name,'namespace':namespace,'descriptorOffset':off,'resolved':bool(off)}
            if off:
                self.pos=self.pool+off-1;entry['parentIndex']=self.number('H');num=self.number('H');methods=[]
                for slot in range(num):
                    flags=self.number('B');method=self.string(self.number('I'));argc=self.number('B')
                    methods.append({'localSlot':slot,'name':method,'flags':flags,'parameters':[self.param() for _ in range(argc)],'result':self.param()})
                entry['methods']=methods;constants=[]
                for _ in range(self.number('H')):
                    cname=self.string(self.number('I'));typ=self.typ();fmts={0:'b',1:'h',2:'i',3:'q',4:'B',5:'H',6:'I',7:'Q'}
                    constants.append({'name':cname,'type':typ,'value':self.number(fmts[typ['tag']])})
                entry['constants']=constants;entry['flags']=self.number('B')
            entries.append(entry)
        missing=[i for i in range(len(self.b)) if i not in self.used]
        return {'version':f'{major}.{minor}','bytes':length,'interfaces':entries,'annotationBytes':annotation,'bytesParsed':len(self.used),'unparsedOffsets':missing}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');a=ap.parse_args();root=pathlib.Path(a.evidence)
    out=root/'xpcom-type-libraries';out.mkdir(exist_ok=True);inv=json.loads((root/'inventory.json').read_text());rows=[]
    for o in inv['objects']:
        if o['kind']!='XPCOM type library':continue
        row={'name':o['name'],'sha256':o['sha256'],**Reader(pathlib.Path(o['snapshot']).read_bytes()).parse()}
        (out/(o['sha256']+'.json')).write_text(json.dumps(row,indent=2)+'\n',encoding='utf-8');rows.append(row)
    (out/'summary.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'libraries':len(rows),'methods':sum(len(i.get('methods',[])) for r in rows for i in r['interfaces']),'unparsedBytes':sum(len(r['unparsedOffsets']) for r in rows)}))
if __name__=='__main__':main()
