"""Reconcile all static evidence and produce an honest, file-by-file coverage ledger."""
import argparse, collections, csv, hashlib, importlib.util, json, pathlib, re, sys, zlib

def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()
def write(p,v):p.write_text(json.dumps(v,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
def csvwrite(p,rows):
    with p.open('w',encoding='utf-8-sig',newline='') as f:
        w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');ap.add_argument('--setup',required=True);ap.add_argument('--strict',action='store_true');ap.add_argument('--report');ap.add_argument('--native-report');a=ap.parse_args()
    root=pathlib.Path(a.evidence);setup=pathlib.Path(a.setup);inv=read(root/'inventory.json');errors=[];native=[];residuals=[];objects={o['sha256']:o for o in inv['objects']}
    for o in objects.values():
        if sha(pathlib.Path(o['snapshot']))!=o['sha256']:errors.append('Snapshot hash mismatch '+o['name'])
        if o['kind']!='PE':continue
        d=root/'ghidra'/(o['sha256'][:12]+'__'+o['name']);p=d/'summary.json';audit=d/'audit.json'
        s=read(p) if p.exists() else {};v=read(audit) if audit.exists() else {};retries=v.get('retries',[])
        recovered=sum(r['completed'] for r in retries);final_failed=s.get('failed',0)-recovered
        row={'name':o['name'],'sha256':o['sha256'],'architecture':s.get('language','pending'),'recognizedFunctions':s.get('functions',''),'eligibleFunctions':s.get('eligible',''),'initialSuccess':s.get('successful',''),'recoveredOnRetry':recovered,'remainingFailures':final_failed,'analysisTimedOut':s.get('analysisTimedOut',''),'exportComplete':bool(s),'assemblyAuditComplete':bool(v),'executableBytes':sum(b['bytes'] for b in v.get('executableBlocks',[])),'instructionBytes':sum(b['instructionBytes'] for b in v.get('executableBlocks',[])),'bytesOutsideRecognizedFunctions':sum(b['bytesOutsideRecognizedFunctions'] for b in v.get('executableBlocks',[])),'evidence':str(d)}
        if s:
            if s['sha256']!=o['sha256']:errors.append('Ghidra hash mismatch '+o['name'])
            statuses=[json.loads(l) for l in (d/'decompilation.jsonl').read_text(encoding='utf-8').splitlines()]
            if len(statuses)!=s['eligible'] or sum(x['completed'] for x in statuses)!=s['successful']:errors.append('Inconsistent function ledger '+o['name'])
            if len({x['entry'] for x in statuses})!=len(statuses) or s['failed']!=s['eligible']-s['successful']:errors.append('Duplicate or inconsistent function outcomes '+o['name'])
            if s['functions']!=s['eligible']+s['external']+s['thunks']:errors.append('Function accounting mismatch '+o['name'])
            if (d/'all-functions.c').read_text(encoding='utf-8').count('/* FUNCTION ')!=s['successful']:errors.append('Pseudocode body count mismatch '+o['name'])
            retry_by_entry={x['entry']:x for x in retries}
            if v and len(retries)!=s['failed']:errors.append('Retry accounting mismatch '+o['name'])
            if v:
                if v['sha256']!=o['sha256']:errors.append('Assembly audit source hash mismatch '+o['name'])
                if len(retry_by_entry)!=len(retries) or set(retry_by_entry)!={x['entry'] for x in statuses if not x['completed']}:errors.append('Assembly audit retry set mismatch '+o['name'])
                if (d/'retry-functions.c').read_text(encoding='utf-8').count('/* FUNCTION ')!=recovered:errors.append('Retry pseudocode body count mismatch '+o['name'])
                with (d/'instructions.asm').open(encoding='utf-8') as listing:
                    if sum(1 for line in listing if line.strip())!=v['instructions']:errors.append('Assembly instruction count mismatch '+o['name'])
                if (d/'failed-functions.asm').read_text(encoding='utf-8').count('; FUNCTION ')!=final_failed:errors.append('Residual function assembly count mismatch '+o['name'])
                for block in v['executableBlocks']:
                    if block['functionBodyBytes']+block['bytesOutsideRecognizedFunctions']!=block['bytes']:errors.append('Executable byte accounting mismatch '+o['name'])
            for x in statuses:
                if x['completed']:continue
                retry=retry_by_entry.get(x['entry'])
                if not retry or not retry['completed']:residuals.append({'name':o['name'],'sha256':o['sha256'],'entry':x['entry'],'initialError':x['error'],'retryError':retry['error'] if retry else 'audit pending','assembly':str(d/'failed-functions.asm')})
        native.append(row)
    # Re-read original program/package files; nested archive members were checked via snapshots above.
    originals=0;changed=[]
    for f in inv['files']:
        if f['origin'].startswith('archive:'):continue
        p=pathlib.Path(f['origin'])/f['path'];originals+=1
        if not p.exists() or sha(p)!=f['sha256']:changed.append(str(p))
    record=read(setup/'setup-records.json');b=pathlib.Path(record['installer']).read_bytes();payload_ok=True
    for row in record['records']:
        stream=b[row['compressedOffset']:row['compressedOffset']+row['compressedBytes']];raw=zlib.decompress(stream)
        if hashlib.sha256(raw).hexdigest()!=row['sha256']:payload_ok=False
    bad=bytearray(b[record['records'][1]['compressedOffset']:record['records'][1]['compressedOffset']+record['records'][1]['compressedBytes']]);bad[len(bad)//2]^=1
    rejected=False
    try:zlib.decompress(bad)
    except zlib.error:rejected=True
    xpts=read(root/'xpcom-type-libraries'/'summary.json');tlb=read(root/'type-libraries'/'status.json');ft=read(root/'filetype-database'/'summary.json');capture=read(root/'capture-config'/'summary.json');signatures=read(root/'signatures.json');languages=read(root/'languages'/'summary.json');lang_by_hash={x['sha256']:x for x in languages}
    resources=read(root/'resource-coverage.json')
    for res in resources:
        if sha(pathlib.Path(res['path']))!=res['sha256']:errors.append('Resource hash mismatch '+res['path'])
    if not all(x['roundTripVerified'] for x in ft['blocks']) or not capture['roundTripVerified']:errors.append('Configuration round-trip failure')
    def decoder(filename):
        sys.dont_write_bytecode=True
        spec=importlib.util.spec_from_file_location(filename.replace('-','_'),pathlib.Path(__file__).with_name(filename));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
    ft_decode=decoder('decode-filetypes.py');capture_decode=decoder('decode-capture-config.py');ft_original=pathlib.Path(objects[ft['sha256']]['snapshot']).read_bytes()
    for block in ft['blocks']:
        data=(root/'filetype-database'/(block['tag'][2:]+'.bin')).read_bytes();start=block['headerOffset']+5
        if hashlib.sha256(data).hexdigest()!=block['decodedSha256'] or ft_decode.transform(data)!=ft_original[start:start+block['bytes']]:errors.append('Decoded file-type block changed: '+block['tag'])
    capture_bytes=(root/'capture-config'/'decoded-config.txt').read_bytes()
    if hashlib.sha256(capture_bytes).hexdigest()!=capture['decodedSha256'] or capture_decode.transform(capture_bytes)!=pathlib.Path(objects[capture['sha256']]['snapshot']).read_bytes():errors.append('Decoded capture configuration changed')
    browser_rules=read(root/'capture-config'/'browser-rule-effects.json');browser_source=pathlib.Path(browser_rules['source']).read_bytes()
    if hashlib.sha256(browser_source).hexdigest()!=browser_rules['sha256']:errors.append('Browser rule evidence source changed')
    for label,fragment in browser_rules['evidence'].items():
        raw_fragment=fragment['text'].encode('utf-8');offset=fragment['byteOffset']
        if browser_source[offset:offset+len(raw_fragment)]!=raw_fragment:errors.append('Browser source evidence mismatch '+label)
    driver_interface=read(root/'selected-evidence'/'driver-interface.json');driver_pseudocode=pathlib.Path(driver_interface['pseudocodeSource']).read_bytes()
    if sha(pathlib.Path(driver_interface['driver']))!=driver_interface['sha256'] or hashlib.sha256(driver_pseudocode).hexdigest()!=driver_interface['pseudocodeSha256']:errors.append('Driver interface evidence source changed')
    for label,fragment in driver_interface['functions'].items():
        raw_fragment=fragment['text'].encode('utf-8');offset=fragment['byteOffset']
        if driver_pseudocode[offset:offset+len(raw_fragment)]!=raw_fragment:errors.append('Driver pseudocode excerpt mismatch '+label)
    if any(x['unparsedOffsets'] for x in xpts):errors.append('Unparsed XPT bytes')
    if any(x['status']!='parsed' for x in tlb):errors.append('Unparsed COM library')
    if hashlib.sha256(b).hexdigest()!=record['sha256']:errors.append('Installer original hash changed')
    aux=read(root/'auxiliary'/'summary.json');aux_by_hash={x['sha256']:x for x in aux}
    for x in aux:
        if 'error' in x:errors.append('Auxiliary parser: '+x['name']+': '+x['error'])
    disassembly=read(root/'dumpbin-disasm'/'summary.json');disassembly_by_hash={x['sha256']:x for x in disassembly}
    expected_native={x['sha256'] for x in native}
    if len(disassembly)!=len(disassembly_by_hash) or set(disassembly_by_hash)!=expected_native:errors.append('DUMPBIN disassembly input set mismatch')
    for x in disassembly:
        output=pathlib.Path(x['output'])
        if x['exitCode']!=0 or not output.exists() or not x['outputBytes']:errors.append('DUMPBIN disassembly failed '+x['name']);continue
        if output.stat().st_size!=x['outputBytes'] or sha(output)!=x['outputSha256']:errors.append('DUMPBIN output hash/size mismatch '+x['name'])
        with output.open('rb') as f:prefix=f.read(8192)
        encoding='utf-16' if prefix.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig'
        if not re.search(r'^\s+[0-9A-Fa-f]{8,16}:\s+[0-9A-Fa-f]',prefix.decode(encoding),re.M):errors.append('DUMPBIN instruction output missing '+x['name'])
    indexed={x['sha256']:x for x in native};ledger=[]
    for f in inv['files']:
        o=objects[f['sha256']];stage=o['status'];limitation='Semantic behavior not exhaustively reviewed'
        if o['kind']=='PE':
            n=indexed[o['sha256']];stage=f"Headers/imports/exports/resources; native export={n['exportComplete']}; assembly audit={n['assemblyAuditComplete']}; independent DUMPBIN disassembly={o['sha256'] in disassembly_by_hash}";limitation=f"Remaining decompilation failures={n['remainingFailures']}; static function discovery is not proof of complete code recovery"
        elif o['kind']=='XPCOM type library':stage='All interface/method descriptors decoded; every byte accounted';limitation='External inherited interface definitions not embedded'
        elif o['kind']=='COM type library':stage='All COM type/member metadata decoded without registration';limitation='Interface declarations do not prove implementation behavior'
        elif o['name'].lower()=='idmftype.dat':stage='Six blocks decoded; complete byte consumption and round trip; MIME mapping recovered';limitation='All 38-byte rule fields not semantically named'
        elif o['name'].lower()=='idmfc.dat':stage='81 active capture rules decoded; round trip verified; all seven active flags traced in pinned browser evaluator';limitation='Complete native/driver meanings and runtime effects unverified'
        elif o['sha256'] in lang_by_hash:
            l=lang_by_hash[o['sha256']];stage=f"Locale-aware decode ({l['encoding']}); {l['entries']} entries; exact round trip={l['roundTripExact']}";limitation=l.get('decodeWarning','Translation correctness and duplicate-key runtime precedence not validated')
        elif o['kind']=='signed catalog':stage='Authenticode status and certutil catalog dump recorded';limitation='Catalog dump does not establish every installation/runtime outcome'
        elif o['sha256'] in aux_by_hash:
            x=aux_by_hash[o['sha256']];stage=x['parser'];limitation=x.get('limitation','Container consistency checked; visual appearance and runtime use not reviewed')
        ledger.append({'origin':f['origin'],'path':f['path'],'sha256':f['sha256'],'bytes':f['bytes'],'kind':o['kind'],'completedWork':stage,'remainingLimit':limitation,'evidenceDirectory':str(pathlib.Path(o['snapshot']).parent)})
    csvwrite(root/'file-coverage-final.csv',ledger);csvwrite(root/'native-coverage.csv',native)
    stats={'fileOccurrences':len(inv['files']),'uniqueContents':len(objects),'kinds':dict(collections.Counter(o['kind'] for o in objects.values())),'nativeBinaries':len(native),'nativeExportsComplete':sum(n['exportComplete'] for n in native),'assemblyAuditsComplete':sum(n['assemblyAuditComplete'] for n in native),'recognizedFunctions':sum(n['recognizedFunctions'] or 0 for n in native),'eligibleFunctions':sum(n['eligibleFunctions'] or 0 for n in native),'initialDecompilationSuccess':sum(n['initialSuccess'] or 0 for n in native),'recoveredOnRetry':sum(n['recoveredOnRetry'] for n in native),'remainingDecompilationFailures':sum(n['remainingFailures'] for n in native),'analysisTimeouts':[n['name'] for n in native if n['analysisTimedOut'] is True],'resourceEntries':len(read(root/'resource-coverage.json')),'signatureStatuses':dict(collections.Counter(x['signatureStatus'] for x in signatures)),'xpcomLibraries':len(xpts),'xpcomMethods':sum(len(e.get('methods',[])) for x in xpts for e in x['interfaces']),'xpcomUnparsedBytes':sum(len(x['unparsedOffsets']) for x in xpts),'comLibraries':len(tlb),'setupPayloadFiles':record['payloadFiles'],'setupAllRecordsVerified':payload_ok,'setupCorruptedStreamRejected':rejected,'setupSha256Matches':hashlib.sha256(b).hexdigest()==record['sha256'],'filetypeRoundTrip':all(x['roundTripVerified'] for x in ft['blocks']),'captureRoundTrip':capture['roundTripVerified'],'snapshotsVerified':len(objects),'originalFilesRechecked':originals,'originalFilesChanged':changed,'inventoryErrors':inv['errors'],'reparseDirectoriesNotFollowed':inv['reparseDirectoriesNotFollowed'],'validationErrors':errors,'limits':['Static analysis only; target code was not executed.','Recognized-function decompilation is inferred pseudocode, not original source or proof of full behavioral understanding.','Download contents and personal state values are excluded from copied evidence; a separate metadata footprint covers them.']}
    stats.update({'resourceHashesVerified':len(resources),'languageFiles':len(languages),'languageRoundTripsExact':sum(x['roundTripExact'] for x in languages),'languageDecodingWarnings':[x['name'] for x in languages if not x['roundTripExact']],'auxiliaryContentsChecked':len(aux),'auxiliaryParserErrors':sum('error' in x for x in aux)})
    stats.update({'independentDisassemblies':len(disassembly),'independentDisassemblyFailures':sum(x['exitCode']!=0 for x in disassembly),'independentDisassemblyBytes':sum(x['outputBytes'] for x in disassembly),'executableMemoryBytes':sum(x['executableBytes'] for x in native),'recognizedInstructionBytes':sum(x['instructionBytes'] for x in native),'executableBytesOutsideRecognizedFunctions':sum(x['bytesOutsideRecognizedFunctions'] for x in native)})
    stats.update({'captureBrowserActiveFlags':browser_rules['activeFlags'],'captureBrowserEvidenceFragmentsVerified':len(browser_rules['evidence'])})
    stats.update({'driverDispatchCasesMapped':len(driver_interface['dispatchCases']),'driverFunctionExcerptsVerified':len(driver_interface['functions'])})
    write(root/'coverage-summary.json',stats);write(root/'native-coverage.json',native);write(root/'decompilation-failures.json',residuals);print(json.dumps(stats,indent=2))
    if a.report:
        p=pathlib.Path(a.report);content=p.read_text(encoding='utf-8');s=stats
        block=f'''<!-- COVERAGE_START -->
| Coverage measure | Result |
| --- | ---: |
| Inventoried file entries / unique contents | {s['fileOccurrences']:,} / {s['uniqueContents']:,} |
| Native binaries with full recognized-function export | {s['nativeExportsComplete']} / {s['nativeBinaries']} |
| Native binaries with assembly/byte audit | {s['assemblyAuditsComplete']} / {s['nativeBinaries']} |
| Independent DUMPBIN disassemblies / failures | {s['independentDisassemblies']} / {s['independentDisassemblyFailures']} |
| Recognized functions, including thunks | {s['recognizedFunctions']:,} |
| Nonexternal, non-thunk functions attempted | {s['eligibleFunctions']:,} |
| Pseudocode recovered, including longer retries | {s['initialDecompilationSuccess']+s['recoveredOnRetry']:,} |
| Remaining pseudocode failures; assembly retained | {s['remainingDecompilationFailures']:,} |
| Automatic-analysis timeouts | {len(s['analysisTimeouts'])} |
| Embedded PE resources, all hashes rechecked | {s['resourceEntries']:,} |
| Snapshots / original file occurrences rehashed | {s['snapshotsVerified']} / {s['originalFilesRechecked']} |
| Changed originals / inventory errors / validation errors | {len(s['originalFilesChanged'])} / {len(s['inventoryErrors'])} / {len(s['validationErrors'])} |

These are static coverage counts, not percentages of fully understood behavior. Function totals include compiler/runtime and third-party code, plus variants across builds and architectures; they are not counts of distinct application algorithms. The installed IDMan and 32-bit monitor analysis databases were reused read-only after checking their SHA-256 values, then exported without the earlier function limits. Other project provenance and [per-function failures](../benchmarks/idm-complete-20260927/decompilation-failures.json) are recorded locally. See the [per-binary coverage table](idm-native-coverage-2026-09-27.md).
<!-- COVERAGE_END -->'''
        content,count=re.subn(r'<!-- COVERAGE_START -->.*?<!-- COVERAGE_END -->',lambda m:block,content,flags=re.S)
        if count!=1:raise ValueError('Report coverage marker missing or duplicated')
        p.write_text(content,encoding='utf-8')
    if a.native_report:
        lines=['# IDM native static coverage','', 'Generated from the local hash-indexed evidence. All function counts come from Ghidra recognition; successful pseudocode is not a correctness guarantee. Full SHA-256 values, source paths, per-function errors, assembly, and counts of executable bytes outside recognized functions remain in the local evidence ledger.','', '[Investigation report](idm-complete-static-analysis-2026-09-27.md) · [Full CSV](../benchmarks/idm-complete-20260927/native-coverage.csv)','','| Binary | SHA-256 prefix | Architecture | Eligible functions | Pseudocode recovered | Residual failures | Assembly audit |','| --- | --- | --- | ---: | ---: | ---: | --- |']
        for n in sorted(native,key=lambda n:(n['name'].lower(),n['sha256'])):
            architecture={'x86:LE:32:default':'x86','x86:LE:64:default':'x64','AARCH64:LE:64:v8A':'ARM64'}.get(n['architecture'],n['architecture']);success=(n['initialSuccess'] or 0)+n['recoveredOnRetry']
            lines.append(f"| {n['name']} | `{n['sha256'][:12]}` | {architecture} | {n['eligibleFunctions']} | {success} | {n['remainingFailures']} | {'complete' if n['assemblyAuditComplete'] else 'pending'} |")
        pathlib.Path(a.native_report).write_text('\n'.join(lines)+'\n',encoding='utf-8')
    failures=errors+changed+inv['errors']
    if a.strict and (failures or stats['analysisTimeouts'] or not payload_ok or not rejected or len(native)!=stats['nativeExportsComplete'] or len(native)!=stats['assemblyAuditsComplete']):raise SystemExit('Coverage validation incomplete; inspect coverage-summary.json')
if __name__=='__main__':main()
