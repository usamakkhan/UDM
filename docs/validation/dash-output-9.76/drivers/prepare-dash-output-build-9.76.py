from pathlib import Path
p=Path('<development>');s=(p/'build-verified-handoff-9.75.py').read_text(encoding='utf-8-sig')
for before,after in [('owned-verified-handoff-build-9.75','owned-dash-output-build-9.76'),('owned-verified-handoff-native-temp-9.75','owned-dash-output-native-temp-9.76'),('owned-verified-handoff-9.75-D-','owned-dash-output-9.76-D-'),('owned-advisory-intent-build-9.74/build-result.json','owned-verified-handoff-build-9.75/build-result.json'),("version='9.75'","version='9.76'"),("extensionVersion='0.73.39'","extensionVersion='0.73.40'"),('UDM 9.75 captured','UDM 9.76 captured'),('Five native 9.75','Five native 9.76'),("'native/VerifiedMediaHandoffTests.cpp'}|","'native/VerifiedMediaHandoffTests.cpp','benchmarks/speed/dash_output_ownership.cpp'}|")]:
 assert before in s,before;s=s.replace(before,after)
q=p/'build-dash-output-9.76.py';assert not q.exists();q.write_text(s,encoding='utf-8')
