"""Owned-tree cleanup and existing wrapper abort gate; no MFC entry or transfer."""
import json
import os
from pathlib import Path
import subprocess
import uuid
import capture_app_finishing as runner


def main():
    root=runner.BASE/'app-runner-controls-8.65'
    root.mkdir(exist_ok=False)
    (root/'.udm-app-runner-controls').write_text('owned runner controls')
    parent=root/'payloads';parent.mkdir()
    checks=0
    def owned():
        work=parent/('owned-realtime-8.58-'+uuid.uuid4().hex);work.mkdir()
        (work/'.udm-lifetime-data').write_text('owned app capture')
        meta=root/('meta-'+uuid.uuid4().hex);(meta/'bin').mkdir(parents=True)
        image=meta/'bin/control.exe';image.write_bytes(b'not an executable; cleanup-only marker')
        return work,meta,image
    work,meta,image=owned();(work/'catalog').mkdir();state=work/'catalog/state.json'
    state.write_text('{"fixture":true}');body=work/'body.data';body.write_bytes(bytes(64))
    os.link(body,work/'alias.bin')
    result=runner.cleanup_work(work,parent,meta,image)
    assert result['passed'] and not work.exists() and not image.exists()
    assert json.loads((meta/'work-metadata/catalog/state.json').read_text())=={'fixture':True}
    checks+=1
    work,meta,image=owned();(work/'.udm-lifetime-data').write_text('wrong marker')
    try:runner.cleanup_work(work,parent,meta,image);raise AssertionError('Unowned tree accepted')
    except ValueError:assert work.exists() and image.exists();checks+=1
    (work/'.udm-lifetime-data').write_text('owned app capture');runner.cleanup_work(work,parent,meta,image)
    work,meta,image=owned();external=root/'external-control.bin';external.write_bytes(b'owned external-link control')
    alias=work/'external-alias.bin';os.link(external,alias)
    try:runner.cleanup_work(work,parent,meta,image);raise AssertionError('External link accepted')
    except ValueError:assert external.read_bytes()==b'owned external-link control' and image.exists();checks+=1
    alias.unlink();runner.cleanup_work(work,parent,meta,image)
    work,meta,image=owned()
    try:runner.cleanup_work(work,parent,meta,external);raise AssertionError('Foreign image accepted')
    except ValueError:assert work.exists() and external.exists() and image.exists();checks+=1
    runner.cleanup_work(work,parent,meta,image);external.unlink()
    # Exercise the existing wrapper's abort byte without ever releasing MFC entry.
    app=runner.load(runner.BASE/'app-io-observer-8.65/build-result.json')
    executable=Path(app['binaryPath']);assert runner.sha(executable)==app['binarySha256']
    work,meta,image=owned();app_root=work/'app';app_root.mkdir()
    (app_root/'.udm-app-observer').write_text('owned abort control');data=app_root/'catalog';data.mkdir()
    runner.save(data/'state.json',{'Schema':1,'Settings':{'ClipboardMonitor':False,
                'ShowTipsOnStartup':False,'CloseToTray':False},'Downloads':[],'Queues':[]})
    report=app_root/'app-return.json';env=os.environ.copy();env['UDM_IO_OBSERVER_REPORT']=str(report)
    process=subprocess.Popen([str(executable),'--background','--instance-tag','abort-'+uuid.uuid4().hex,
             '--data-dir',str(data)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
             text=True,env=env,cwd=executable.parent,creationflags=subprocess.CREATE_NO_WINDOW)
    try:stdout,stderr=process.communicate(input='x\n',timeout=25)
    except subprocess.TimeoutExpired:
        while True:
            print('Observing original abort-control PID '+str(process.pid),flush=True)
            try:stdout,stderr=process.communicate(timeout=25);break
            except subprocess.TimeoutExpired:pass
    assert process.returncode==5 and 'READY' in stdout and 'DONE' not in stdout and not report.exists()
    assert runner.sha(executable)==app['binarySha256'];checks+=1
    runner.cleanup_work(work,parent,meta,image)
    value=dict(passed=True,controls=checks,wrapperProcessesStarted=1,mfcEntryReleased=False,
               abortExitCode=5,newOriginStarted=False,newDownloadBytes=0,newElevationRequested=False,
               sourceWrapperPreserved=True)
    runner.save(root/'result.json',value);print(json.dumps(value))


if __name__=='__main__':main()
