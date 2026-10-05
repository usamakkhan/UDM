"""Decode every language file using its LANGID instead of a blanket Western codec."""
import argparse, collections, ctypes, json, pathlib, re

def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');a=ap.parse_args();root=pathlib.Path(a.evidence)
    inv=json.loads((root/'inventory.json').read_text());out=root/'languages';out.mkdir(exist_ok=True);rows=[]
    locale=ctypes.windll.kernel32.GetLocaleInfoW;locale.argtypes=[ctypes.c_uint,ctypes.c_uint,ctypes.c_wchar_p,ctypes.c_int];locale.restype=ctypes.c_int
    for o in inv['objects']:
        if not o['name'].endswith('.lng'):continue
        b=pathlib.Path(o['snapshot']).read_bytes();m=re.search(rb'(?im)^(?:lang|instlng|instlang)=0x([0-9a-f]+)',b);lang=int(m[1],16) if m else None
        row={'name':o['name'],'sha256':o['sha256'],'languageId':hex(lang) if lang is not None else None}
        try:text=b.decode('utf-8-sig');enc='utf-8-sig' if b.startswith(b'\xef\xbb\xbf') else 'utf-8';method='valid UTF-8'
        except UnicodeDecodeError:
            if lang is None:raise ValueError('Language ID missing in '+o['name'])
            lcid=lang|0x400 if lang<0x400 else lang;buf=ctypes.create_unicode_buffer(32)
            if not locale(lcid,0x1004,buf,32) or not int(buf.value):raise ValueError('Windows ANSI code page missing for '+o['name'])
            enc='cp'+buf.value;method='Windows LOCALE_IDEFAULTANSICODEPAGE for '+hex(lcid)
            try:text=b.decode(enc)
            except UnicodeDecodeError as e:
                text=b.decode(enc,errors='replace');row['decodeWarning']=str(e)
        row.update({'encoding':enc,'encodingEvidence':method,'roundTripExact':text.encode(enc,errors='replace')==b,'replacementCharacters':text.count('\ufffd')})
        values=collections.defaultdict(list)
        for line in text.splitlines():
            line=line.strip()
            if not line or line.startswith(('//',';','#')) or '=' not in line:continue
            key,value=line.split('=',1);values[key.strip()].append(value)
        row['entries']=sum(map(len,values.values()));row['uniqueKeys']=len(values);row['duplicateKeys']={k:len(v) for k,v in values.items() if len(v)>1}
        p=out/o['sha256'];p.mkdir(exist_ok=True);(p/'decoded.txt').write_text(text,encoding='utf-8');(p/'entries.json').write_text(json.dumps(values,ensure_ascii=True,indent=2)+'\n',encoding='utf-8');rows.append(row)
    (out/'summary.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'files':len(rows),'encodings':dict(collections.Counter(x['encoding'] for x in rows)),'exactRoundTrips':sum(x['roundTripExact'] for x in rows),'warnings':[{k:x[k] for k in ('name','encoding','decodeWarning') if k in x} for x in rows if not x['roundTripExact']],'filesWithDuplicateKeys':sum(bool(x['duplicateKeys']) for x in rows)},indent=2))
if __name__=='__main__':main()
