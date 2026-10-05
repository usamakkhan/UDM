"""Preserve a static x64 WFP driver dispatch map; never open or invoke the device."""
import argparse
import hashlib
import json
import re
from pathlib import Path

EXPECTED = '8acffb0181146e96c44a94c5b364d657b775936ebb4d0fcce591068f74803c4a'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('evidence')
    args = ap.parse_args()
    root = Path(args.evidence)
    original = root / 'objects' / EXPECTED / 'idmwfp64.sys'
    if hashlib.sha256(original.read_bytes()).hexdigest() != EXPECTED:
        raise ValueError('Driver specimen mismatch')
    folder = root / 'ghidra' / (EXPECTED[:12] + '__idmwfp64.sys')
    source = folder / 'all-functions.c'
    raw = source.read_bytes()
    text = raw.decode('utf-8')

    def function(entry):
        start = text.index('/* FUNCTION ' + entry + ';')
        stop = text.find('/* FUNCTION ', start + 12)
        stop = len(text) if stop < 0 else stop
        return {'entry': entry, 'byteOffset': len(text[:start].encode('utf-8')),
                'line': text[:start].count('\n') + 1, 'text': text[start:stop]}

    functions = {name: function(entry) for name, entry in {
        'entry': '14000f8a0', 'deviceControl': '1400108b0',
        'configurationReader': '1400165a0', 'calloutAndFilter': '140004880',
    }.items()}
    dispatch = functions['deviceControl']['text']
    observed = sorted(set(int(x, 16) for x in re.findall(r'case (0x12c[0-9a-f]+):', dispatch)))
    inputs = {0x12c004: 4, 0x12c008: 12, 0x12c00c: 24, 0x12c010: 4,
              0x12c014: 8, 0x12c018: 8, 0x12c01c: 24, 0x12c020: 24,
              0x12c024: 12, 0x12c02c: 44, 0x12c030: 48}
    if observed != sorted(inputs):
        raise ValueError('Dispatch case set changed')
    rows = [{'code': hex(code), 'deviceTypeBits': code >> 16,
             'accessBits': (code >> 14) & 3, 'functionBits': (code >> 2) & 0xfff,
             'methodBits': code & 3, 'observedMinimumInputBytes': inputs[code]}
            for code in observed]
    strings = (folder / 'strings.tsv').read_text(encoding='utf-8')
    selected_strings = [line for line in strings.splitlines()
                        if any(needle in line for needle in
                               ['\\Device\\IDMWFP', '\\DosDevices\\IDMWFP', 'D:P(A;;GA;;;AU)'])]
    if len(selected_strings) != 3:
        raise ValueError('Driver device/security string evidence changed')
    instructions = (folder / 'instructions.asm').read_text(encoding='utf-8').splitlines()
    setup_asm = [line for line in instructions
                 if 0x14000fa3c <= int(line.split('\t')[0], 16) < 0x14000fadc]
    if not setup_asm:
        raise ValueError('Device setup assembly missing')
    output = root / 'selected-evidence'
    output.mkdir(exist_ok=True)
    (output / 'driver-device-setup.asm').write_text('\n'.join(setup_asm) + '\n', encoding='utf-8')
    result = {
        'driver': str(original), 'sha256': EXPECTED,
        'pseudocodeSource': str(source), 'pseudocodeSha256': hashlib.sha256(raw).hexdigest(),
        'deviceAndSecurityStrings': selected_strings,
        'dispatchCases': rows, 'functions': functions,
        'observations': [
            'Entry references device \\Device\\IDMWFP and symbolic link \\DosDevices\\IDMWFP.',
            'Assembly at 0x14000fa4d loads the SDDL string before WdmlibIoCreateDeviceSecure at 0x14000fa9d.',
            'Entry stores device-control handler 0x1400108b0 at driver-object offset 0xe0.',
            'The 0x12c008 case invokes 0x1400165a0 after a 12-byte minimum-input check.',
            'Configuration reader includes tag 0x12 and compares the supplied requestor identifier with the located client object when nonzero.',
            '0x140004880 calls FwpmCalloutAdd0 and FwpmFilterAdd0; registration and removal helpers occur elsewhere.',
        ],
        'limits': [
            'Minimum sizes are observed dispatcher gates, not complete valid-message schemas.',
            'Ghidra incorrectly types the message buffer as IMAGE_DOS_HEADER; those field names are not protocol names.',
            'Raw SDDL is recorded without claiming the effective runtime ACL; installation state can matter.',
            'No device opened, control request submitted, driver loaded, or exploitability conclusion made.',
        ],
    }
    (output / 'driver-interface.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'driverSha256': EXPECTED, 'dispatchCases': len(rows), 'functionExcerpts': len(functions)}))


if __name__ == '__main__':
    main()
