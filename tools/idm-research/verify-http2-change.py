"""Independently disassemble the observed build-to-build fallback branches.

Pinned specimen hashes and observed instruction boundaries prevent accidental
comparison against a different build or a mid-instruction starting offset.
"""
import argparse, json, pathlib, sys

def main():
    ap=argparse.ArgumentParser();ap.add_argument('evidence');ap.add_argument('--python-libs',required=True);a=ap.parse_args();sys.path.insert(0,a.python_libs)
    import pefile,capstone
    r=pathlib.Path(a.evidence);out=r/'selected-evidence';out.mkdir(exist_ok=True);evidence=[]
    targets=[('build11-http2','e8b0459d59a3fbad73805fd544cc80472c1a08aaef5b3cdbb4088c9ab9dd4e69',0x593070,0x59314d),('installed-http2','03cc62e9adb77a380f9dc12f67ccaaee5106f12844aa73ce32c914ddd16d607c',0x5931d0,0x593275)]
    for label,h,lo,hi in targets:
        expected=[]
        with (r/'ghidra'/(h[:12]+'__IDMan.exe')/'instructions.asm').open(encoding='utf-8') as f:
            for line in f:
                v=line.rstrip().split('\t');address=int(v[0],16)
                if lo<=address<hi:expected.append((address,bytes.fromhex(v[1])))
        if not expected:raise ValueError('No instruction evidence for '+label)
        start=expected[0][0];end=expected[-1][0]+len(expected[-1][1]);pe=pefile.PE(str(r/'objects'/h/'IDMan.exe'));code=pe.get_data(start-pe.OPTIONAL_HEADER.ImageBase,end-start)
        cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);instructions=list(cs.disasm(code,start))
        if [(i.address,bytes(i.bytes)) for i in instructions]!=expected:raise ValueError('Independent instruction-boundary comparison failed')
        lines=[f'{i.address:08x} {i.bytes.hex():24} {i.mnemonic} {i.op_str}' for i in instructions]
        (out/(label+'.asm')).write_text('\n'.join(lines)+'\n',encoding='utf-8');evidence.append({'label':label,'sha256':h,'start':hex(start),'endExclusive':hex(end),'capstoneInstructions':len(lines),'instructionBoundariesMatchGhidra':True})
    (out/'http2-comparison.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8');print(json.dumps(evidence,indent=2))
if __name__=='__main__':main()
