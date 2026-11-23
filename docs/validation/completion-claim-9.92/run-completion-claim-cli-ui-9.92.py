from pathlib import Path
import os,json,subprocess
area=Path(os.environ['LOCALAPPDATA'])/'UDM-Development/owned-completion-claim-build-9.92';rows=[]
for name,mode in [('claim-frozen','baseline'),('claim-current','candidate')]:
 folder=area/'ui-runs-cli-final'/name;folder.parent.mkdir(exist_ok=True);assert not folder.exists()
 with (area/'ui-fixtures'/(name+'-cli-run.log')).open('w',encoding='utf-8') as log:
  p=subprocess.Popen([str(area/'ui-fixtures'/(name+'.exe')),mode,str(folder)],stdout=log,stderr=subprocess.STDOUT);(folder.parent/(name+'-process.json')).write_text(json.dumps({'pid':p.pid}));code=p.wait(timeout=90)
 d=json.loads((folder/'results.json').read_text(encoding='utf-8-sig'));row={'suite':name,'passed':code==0 and d['passed'],'checks':len(d['checks']),'error':d.get('error'),'resultRoot':'ui-runs-cli-final'};rows.append(row);print(json.dumps(row),flush=True)
 if not row['passed']:break
old=json.loads((area/'before-cli-ui-final-results.json').read_text());rows += [{**r,'resultRoot':'ui-runs-final'} for r in old['suites'] if not r['suite'].startswith('claim-')]
(area/'ui-final-results.json').write_text(json.dumps({'passed':all(x['passed'] for x in rows),'suites':rows},indent=2));assert all(x['passed'] for x in rows)
