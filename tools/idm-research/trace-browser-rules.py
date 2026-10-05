"""Record hash-pinned, static evidence for capture-rule effects; never execute the bundle."""
import argparse
import hashlib
import json
from pathlib import Path

EXPECTED = '3590408894c701937ffa51048c14c3e6d1b534e9616b76d99e4af523925294d7'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('evidence')
    args = ap.parse_args()
    root = Path(args.evidence)
    source = root / 'objects' / EXPECTED / 'background.js'
    raw = source.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('Different browser specimen; review its code before reusing these interpretations')
    text = raw.decode('utf-8')
    rules = json.loads((root / 'capture-config' / 'rules.json').read_text(encoding='utf-8'))
    active = sorted(set(''.join(rule['flags'] for rule in rules)))

    def excerpt(start, end):
        offset = text.index(start)
        stop = text.index(end, offset + len(start)) + len(end)
        return {'byteOffset': len(text[:offset].encode('utf-8')),
                'line': text[:offset].count('\n') + 1,
                'text': text[offset:stop]}

    evidence = {
        'schemeFlags': excerpt('wa={http:', 'idmreg:12,idmdwnlmfv9:33}'),
        'resourceGroups': excerpt('Oa=["object","image"]', 'N=Na.concat(Oa,"xmlhttprequest","media","other")'),
        'allowedFlagAlphabet': excerpt('gb=C.bind(/^[', '*$/)'),
        'ruleEvaluator': excerpt('function Gc(', '\nfunction Hc'),
        'requestClassification': excerpt('a.h=r||Pa.includes(b)', 'this.va[g])'),
        'responseSuppression': excerpt('if(m){let Pb,Qb;if(b.Y)', 'if(r||v)'),
        'callerActions': excerpt('switch(b.action=Gc(this,b))', 'if(g)return{requestHeaders:c}'),
    }
    meanings = {
        'G': 'When G/P method flags are present, GET satisfies G; a mismatch stops this rule path.',
        'P': 'When G/P method flags are present, POST satisfies P; G and P together permit either.',
        'S': 'Outside the V/A/C branch, a request with b.h truthy is rejected by this rule unless S is present. b.h includes background requests and object/image/XHR/media resource classes, although object/image types are excluded earlier from rule matching.',
        'D': 'Rejects this rule with result 1 when b.J is falsy. b.J is populated from the secure-scheme bit or HTTPS-proxy/configuration state. This is conditional routing, not proof of TLS security or a download being allowed.',
        'V': 'Shares the A/C branch: unless b.Y rejects it, sets b.h and b.g true, optionally replaces b.P with the extracted extension, and returns 6 for subsequent processing. The bundle does not distinguish V, A, and C within this evaluator.',
        'H': 'Sets request field b.Y. This rejects the V/A/C branch and later clears candidate m in Hc. It does not unconditionally suppress every response-processing branch.',
        'T': 'Permits the request-path extension b.P as fallback after captured filename/extension processing; sets the explicit-extension marker.',
        'A': 'Same evaluator branch as V; absent from this decoded rule set.',
        'C': 'Same evaluator branch as V; absent from this decoded rule set.',
        'X': 'Outside V/A/C, rejects a request whose b.h is false; absent from this decoded rule set.',
        'R': 'Accepted by the compiler alphabet but no R check is present in Gc; absent from this decoded rule set.',
    }
    result = {
        'source': str(source), 'sha256': EXPECTED, 'activeFlags': active,
        'allActiveFlagsMechanicallyTraced': all(flag in meanings for flag in active),
        'flagEffects': meanings, 'evidence': evidence,
        'resultCodes': {'1': 'Caller sets b.g true and does not enter its immediate Mc capture branches.',
                        '4': 'Caller invokes Mc(this,1,b).', '3': 'Caller invokes Mc(this,3,b).',
                        '6': 'No immediate Mc action in this caller; later processing can continue.'},
        'limits': ['Static control-flow interpretation for this pinned browser bundle only.',
                   'Obfuscated fields retain original names where a broader semantic name is unproven.',
                   'Native/driver consumers may interpret flags differently; no runtime verification.',
                   'No original vendor expansion of the flag letters is inferred.'],
    }
    if not result['allActiveFlagsMechanicallyTraced']:
        raise ValueError('Active rule flag lacks a recorded interpretation')
    output = root / 'capture-config' / 'browser-rule-effects.json'
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'output': str(output), 'activeFlags': active, 'evidenceExcerpts': len(evidence)}))


if __name__ == '__main__':
    main()
