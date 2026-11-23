from pathlib import Path
import json,hashlib
base=Path('<development>');root=Path('<repo>')
s=(base/'build-advisory-intent-9.74.py').read_text(encoding='utf-8-sig')
s=s.replace('owned-advisory-intent-build-9.74','owned-verified-handoff-build-9.75').replace('owned-advisory-intent-native-temp-9.74','owned-verified-handoff-native-temp-9.75').replace('owned-advisory-intent-9.74-D-','owned-verified-handoff-9.75-D-')
s=s.replace("owned-advisory-intent-build-9.73/build-result.json","owned-advisory-intent-build-9.74/build-result.json")
s=s.replace("'9.73 captured source inventory'","'9.74 captured source inventory'").replace("version='9.74'","version='9.75'").replace("extensionVersion='0.73.38'","extensionVersion='0.73.39'").replace('UDM 9.74 captured','UDM 9.75 captured').replace('Five native 9.74','Five native 9.75')
s=s.replace("paths={r['path'] for r in previous['sources']}|", "paths={r['path'] for r in previous['sources']}|{'native/VerifiedMediaHandoffTests.cpp'}|")
s=s.replace("tests=['AdvisoryPublicationTests'", "tests=['VerifiedMediaHandoffTests','AdvisoryPublicationTests'")
path=base/'build-verified-handoff-9.75.py';assert not path.exists();path.write_text(s,encoding='utf-8')
print('Prepared source-captured dependency rebuild and 22 affected suites')
