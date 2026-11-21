"""Headless owned MFC-app trace. Defaults to read-only preflight; never requests UAC."""
import argparse
import ctypes
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import queue
import re
import shutil
import subprocess
import sys
import threading
import time
import uuid

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
BASE = Path(os.environ['LOCALAPPDATA'])/'UDM-Development'
from origin import Origin, Options, expected_sha256


def load(path):
    with path.open('rb') as stream:
        raw = stream.read(32*1024*1024+1)
    if len(raw) > 32*1024*1024:
        raise ValueError('Oversized diagnostic record')
    return json.loads(raw.decode('utf-8-sig'))


def save(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest().upper()


def require(value, message):
    if not value:
        raise ValueError(message)


def import_file(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def qualification(report, native, prefix, collector_reply, expected_hash):
    """A staged/offline pass cannot satisfy the live prerequisite."""
    require(prefix.get('passed') is True and prefix.get('targetThreadFilter') is True,
            'Live filtered-fixture prefix qualification is missing')
    require(report.get('traceProfile') == 'disk-scheduling-target-threads-v3', 'Fixture profile')
    require(report.get('started') is True and report.get('eventsLost') == 0
            and report.get('buffersLost') == 0 and report.get('limit') is False
            and report.get('callbackFailed') is False, 'Fixture capture incomplete')
    require(collector_reply.get('accepted') is True
            and collector_reply.get('collectorSha256') == expected_hash, 'Fixture collector differs')
    require(native.get('passed') is True and native.get('elevated') is False
            and native.get('bytes') == 8192, 'Normal-token fixture prerequisite')
    require(report.get('sessionAbsent') is True and report.get('stopStatus') == 0,
            'Fixture logger shutdown unproven')
    targets = report.get('targets', [])
    require(len(targets) == 1 and targets[0].get('pid') == native.get('pid'), 'Fixture PID')
    target = targets[0]
    require(target.get('pendingDiskOperations') == 0 and target.get('ambiguous') is False,
            'Fixture disk correlation incomplete')
    for call in native.get('flushCalls', []):
        completions = [row for row in target.get('diskEvents', [])
                       if row.get('opcode') == 'flushbuffers'
                       and row.get('linkedToTargetInit') is True
                       and row.get('targetInitOpcode') == 'flushinit'
                       and row.get('targetInitThread') == call.get('thread')
                       and call['beginCounter'] <= row['targetInitCounter']
                       <= row['counter'] <= call['endCounter']]
        require(completions, 'Fixture lacks a linked physical flush within a native call')
    require(len(native.get('flushCalls', [])) == 2, 'Fixture native calls')


def preflight(area, app_build):
    require(not ctypes.windll.shell32.IsUserAnAdmin(), 'Controller must use its normal token')
    build = load(area/'build-result.json')
    collector = next(row for row in build['outputs'] if row['name'] == 'finish_thread_filtered_lifetime')
    require(sha(Path(collector['binary'])) == collector['binarySha256'], 'Collector changed')
    app = load(app_build)
    require(app.get('orderedWritesDefaultEnabled') is False, 'Requires the normal App build')
    require(sha(Path(app['binaryPath'])) == app['binarySha256'], 'App observer changed')
    require(sha(ROOT/'benchmarks/speed/finish_user_io_agent.js') == app['agentSha256'], 'Agent changed')
    addon_hash = sha(ROOT/'benchmarks/speed/finish_app_io_read_addon.js')
    known_observer = load(ROOT/'docs/validation/app-io-observation-8.65/results.json')
    require(any(row.get('mode') == 'observe-read' and row.get('normalProcessExit') is True
                and row.get('hashMatches') is True and row.get('sourceBinarySha256') == app['binarySha256']
                and row.get('agentSha256') == app['agentSha256']
                and row.get('readAddonSha256') == addon_hash for row in known_observer['runs']),
            'Native observer lacks a matching qualified control')
    for row in app['coreObjects']:
        require(sha(Path(row['path'])) == row['sha256'], 'Normal core changed')
    require(sha(Path(app['appEntry'])) == app['appEntrySha256'], 'App entry changed')
    proof = load(area/'last-fixture-private.json')
    run = Path(proof['run']).resolve()
    require(run.parent == area.resolve() and (run/'.udm-lifetime-capture').is_file(), 'Fixture scope')
    report, native = load(run/'results.json'), load(run/'native-fixture.json')
    controller = load(run/'controller-private.json')
    request_id = load(run/'request-private.json')['requestId']
    request = load(Path(controller['queue'])/('request-'+request_id+'.json'))
    expected_pid = load(run/'fixture-process.json')['pid']
    fixture_build = load(area/'fixture-build-result.json')
    require(load(run/'fixture-process.json').get('sourceBinarySha256') == fixture_build['binarySha256']
            and sha(Path(fixture_build['binary'])) == fixture_build['binarySha256'], 'Fixture binary differs')
    model = import_file('app_finish_prefix', ROOT/'benchmarks/speed/analyze_cswitch_prefix.py')
    recomputed = model.analyze(report, native, load(BASE/'cswitch-prefix-8.65/result.json'),
                               expected_pid, request['prefix']+'f.exe')
    require(recomputed.get('passed') is True, 'Current prefix analysis rejects fixture: '+str(recomputed.get('failures')))
    integrity = load(run/'fixture-integrity.json')
    require(integrity.get('passed') is True and integrity.get('normalExit') is True
            and integrity.get('bytes') == 8192
            and integrity.get('sha256') == hashlib.sha256(bytes(8192)).hexdigest().upper(),
            'Fixture integrity prerequisite')
    qualification(report, native, recomputed, load(run/'collector-pid.json'), collector['binarySha256'])
    config = load(area/'worker-config.json')
    worker_queue = Path(config['queue'])
    ready = load(worker_queue/'ready.json')
    require(not (worker_queue/'stopped.json').exists(), 'Worker stopped')
    require(datetime.datetime.now(datetime.timezone.utc)
            < datetime.datetime.fromisoformat(ready['expiresUtc']), 'Worker expired')
    require(config['collectorSha256'] == collector['binarySha256'], 'Worker trusts another collector')
    utils = import_file('app_finish_utils', ROOT/'benchmarks/speed/compare-completion.py')
    handle = utils.k.OpenProcess(0x100000|0x1000, False, int(ready['pid']))
    require(handle, 'Cannot inspect approved worker')
    try:
        require(utils.k.WaitForSingleObject(handle, 0) == 258, 'Worker no longer running')
        require(utils.exit_ticks(handle) is None, 'Worker has exited')
        stamps = [utils.W.FILETIME() for _ in range(4)]
        require(utils.k.GetProcessTimes(handle,*[ctypes.byref(t) for t in stamps]), 'Cannot read worker creation time')
        created = utils.ticks(stamps[0])/1e7-11644473600
        expected = datetime.datetime.fromisoformat(ready['expiresUtc']).timestamp()-60*config['sessionMinutes']
        require(abs(created-expected) < 10, 'Worker PID no longer matches readiness record')
    finally:
        utils.k.CloseHandle(handle)
    return app, collector, config, utils


def wait_record(path, *, utils, process_id, terminal_paths=()):
    last_notice = time.monotonic()
    handle = None
    try:
        while True:
            try:
                if path.exists(): return load(path)
            except (json.JSONDecodeError,PermissionError):
                pass  # A producer may still be writing the same record.
            for terminal in terminal_paths:
                try:
                    if terminal.exists():
                        load(terminal)
                        raise RuntimeError('Job reported a terminal outcome before '+path.name)
                except (json.JSONDecodeError,PermissionError):
                    pass
            if handle is None:
                handle = utils.k.OpenProcess(0x100000|0x1000,False,process_id)
                if not handle and ctypes.get_last_error() == 87:
                    raise RuntimeError('Original process no longer exists; missing '+path.name)
            if handle and utils.k.WaitForSingleObject(handle,0) == 0:
                # Recheck after confirmed exit: the final file may have appeared.
                if path.exists(): return load(path)
                raise RuntimeError('Original process exited before '+path.name)
            if time.monotonic()-last_notice > 25:
                print('Still observing original PID '+str(process_id)+': '+path.name,flush=True)
                last_notice = time.monotonic()
            time.sleep(.05)
    finally:
        if handle: utils.k.CloseHandle(handle)


def cleanup_work(work, work_parent, meta, image):
    require(not work.lstat().st_file_attributes & 0x400, 'Reparse cleanup root')
    work = work.resolve()
    require(work.parent == work_parent.resolve()
            and re.fullmatch(r'owned-realtime-8\.58-[0-9a-f]{32}',work.name), 'Cleanup root scope')
    marker = work/'.udm-lifetime-data'
    require(not marker.lstat().st_file_attributes & 0x400
            and marker.read_text() == 'owned app capture', 'Cleanup ownership marker')
    require(image.resolve().parent == (meta/'bin').resolve()
            and image.stat().st_nlink == 1
            and not image.lstat().st_file_attributes & 0x400, 'App-copy cleanup scope')
    for path in [work,*work.parents]:
        require(not path.lstat().st_file_attributes & 0x400, 'Reparse cleanup ancestor')
        if path == work_parent: break
    files, groups = [], {}
    for directory, children, names in os.walk(work,topdown=True,followlinks=False):
        for name in children+names:
            path = Path(directory)/name
            stat = path.lstat()
            require(not stat.st_file_attributes & 0x400, 'Reparse cleanup entry')
            if path.is_file():
                files.append((path,stat))
                groups.setdefault((stat.st_dev,stat.st_ino),[]).append(stat)
    require(all(len(group) == group[0].st_nlink for group in groups.values()),
            'Cleanup has hard links outside its owned tree')
    kept = 0
    for path, stat in files:
        if (path.name.endswith(('.json','.json.tmp')) and stat.st_size <= 1024*1024
                and kept+stat.st_size <= 8*1024*1024):
            target = meta/'work-metadata'/path.relative_to(work)
            target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(path.read_bytes()); kept += stat.st_size
    shutil.rmtree(work)
    require(not work.exists(), 'Work cleanup incomplete')
    image.unlink()
    value = dict(passed=True,ownedWorkTreeRemoved=True,workFilesRemoved=len(files),
                 logicalWorkBytesRemoved=sum(s.st_size for _,s in files),
                 uniqueFileBytesRemoved=sum(group[0].st_size for group in groups.values()),
                 compactMetadataBytesPreserved=kept,generatedAppCopyRemoved=True)
    save(meta/'payload-cleanup.json',value)
    return value


def capture(args, app, collector, config, utils):
    area = args.area.resolve()
    work_parent = Path(config['args'][config['args'].index('-DataParent')+1]).resolve()
    require(work_parent == (ROOT/'.media-cache').resolve(), 'This controller requires the declared D: work parent')
    require(shutil.disk_usage(work_parent).free >= 2*args.bytes+16*1024**2, 'Insufficient payload-space margin')
    require(shutil.disk_usage(area).free >= Path(app['binaryPath']).stat().st_size+64*1024**2,
            'Insufficient capture-space margin')
    nonce = uuid.uuid4().hex
    meta = area/('live-'+str(time.time_ns()))
    meta.mkdir()
    (meta/'.udm-lifetime-capture').write_text('owned app capture')
    work = work_parent/('owned-realtime-8.58-'+nonce)
    work.mkdir()
    (work/'.udm-lifetime-data').write_text('owned app capture')
    app_root = work/'app'
    app_root.mkdir()
    (app_root/'.udm-app-observer').write_text('owned app observer')
    destination, data = app_root/'files', app_root/'catalog'
    destination.mkdir(); data.mkdir()
    output, report_path = destination/'sample.bin', app_root/'app-return.json'
    save(data/'state.json', {'Schema':1, 'Settings':dict(Connections=args.connections,
         Parallel=1, DownloadFolder=str(destination), CategoryFolders=False,
         SuppressProgressDialog=True, SuppressCompletionDialog=True, Sound=False,
         ShowTipsOnStartup=False, ClipboardMonitor=False, PrefetchFileInfo=False,
         StopOnWebPage=False, DuplicatePolicy='Numbered', CloseToTray=False,
         UseHttp3=False, ProxyMode='Connect directly'), 'Downloads':[], 'Queues':[]})
    images = meta/'bin'; images.mkdir()
    prefix = 'u'+nonce[:8]
    image = images/(prefix+'a.exe')
    image.write_bytes(Path(app['binaryPath']).read_bytes())
    require(sha(image) == app['binarySha256'], 'App copy differs')
    worker_queue = Path(config['queue'])
    request_id = uuid.uuid4().hex
    save(worker_queue/('request-'+request_id+'.json'), dict(action='trace',
         captureRoot=str(meta), dataRoot=str(work), controllerPid=os.getpid(), prefix=prefix))
    save(meta/'controller-private.json', dict(pid=os.getpid(), work=str(work),
         requestId=request_id, collectorSha256=collector['binarySha256'],
         sourceAppSha256=app['binarySha256'], controllerSourceSha256=sha(Path(__file__))))
    save(area/'current-app-private.json', dict(meta=str(meta), work=str(work), controllerPid=os.getpid()))
    worker_pid = load(worker_queue/'ready.json')['pid']
    try:
        reply = wait_record(worker_queue/('response-'+request_id+'.json'),utils=utils,
                            process_id=worker_pid,terminal_paths=(worker_queue/'stopped.json',))
        require(reply.get('accepted') is True and reply.get('collectorSha256') == collector['binarySha256'],
                'Collector refused or identity differs: '+str(reply))
        save(meta/'collector-pid.json',reply)
        collector_ready = wait_record(meta/'ready.json',utils=utils,process_id=reply['pid'],
                    terminal_paths=(meta/'results.json',worker_queue/('exit-'+request_id+'.json')))
        require(collector_ready.get('started') is True and collector_ready.get('collectorPid') == reply['pid']
                and collector_ready.get('expectedParent') == os.getpid(),'Collector readiness identity differs')
    except BaseException as error:
        save(meta/'failure-private.json',dict(type=type(error).__name__,error=str(error),appNeverStarted=True))
        save(meta/'done.json',dict(controllerFailedBeforeApp=True))
        cleanup_work(work,work_parent,meta,image)
        raise
    # No origin, child App or desktop operation starts before collector readiness.
    try:
        origin = Origin(Options(size=args.bytes,rate=8_000_000,latency_ms=20,
                        no_range=args.connections == 1,
                        accept_ranges='none' if args.connections == 1 else None,seed=20261113))
    except BaseException as error:
        save(meta/'failure-private.json',dict(type=type(error).__name__,error=str(error),appNeverStarted=True))
        save(meta/'done.json',dict(controllerFailedBeforeApp=True))
        cleanup_work(work,work_parent,meta,image)
        raise
    env = os.environ.copy(); env['UDM_IO_OBSERVER_REPORT'] = str(report_path)
    command = [str(image),'--instance-tag',nonce,'--data-dir',str(data),'--background',
               '/d',origin.url,'/p',str(destination),'/f',output.name,'/n','/q']
    process = None; session = None; script = None; messages = []; stdout = []; stderr = []
    entry_started = False
    failure = None
    signals = queue.Queue()
    def pump(stream, values, notify=False):
        for line in stream:
            values.append(line.rstrip())
            if notify: signals.put(line.strip())
        if notify: signals.put('<EOF>')
    def status(expected):
        while True:
            try: value = signals.get(timeout=25)
            except queue.Empty:
                require(process.poll() is None, 'Child ended without '+expected)
                print('App still running; observing original PID '+str(process.pid), flush=True)
                continue
            require(value != '<EOF>', 'Child ended before '+expected)
            if value == expected: return
    try:
        process = subprocess.Popen(command, cwd=images, env=env, stdin=subprocess.PIPE,
                  stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding='utf-8',
                  errors='replace', creationflags=subprocess.CREATE_NO_WINDOW)
        save(meta/'app-process-private.json', dict(pid=process.pid, command=command))
        threading.Thread(target=pump,args=(process.stdout,stdout,True),daemon=True).start()
        threading.Thread(target=pump,args=(process.stderr,stderr),daemon=True).start()
        status('READY')
        sys.path.insert(0,str(BASE/'finish-frida-runtime'))
        import frida
        session = frida.attach(process.pid)
        code = (ROOT/'benchmarks/speed/finish_user_io_agent.js').read_text()+'\n'+(ROOT/'benchmarks/speed/finish_app_io_read_addon.js').read_text()
        script = session.create_script(code)
        script.on('message',lambda message, payload:messages.append(message))
        script.load()
        identity = script.exports_sync.identity()
        require(identity['pid'] == process.pid and Path(identity['image']).resolve() == image.resolve(), 'Hook target identity')
        save(meta/'native-identity-private.json',identity)
        script.exports_sync.start(nonce)
        entry_started = True  # Delivery is uncertain if the release write fails.
        process.stdin.write('\n'); process.stdin.flush()
        status('DONE')
        app_return = load(report_path)
        require(app_return.get('passed') is True and app_return['compiledVersion'] == app['compiledVersion'], 'MFC entry result')
        snapshot = script.exports_sync.snapshot()
        save(meta/'native-io-private.json', snapshot)
        require(snapshot['dropped'] == snapshot['openFiles'] == snapshot['openMappings'] == 0 and not messages, 'Native capture incomplete')
        process.stdin.write('\n'); process.stdin.flush()
        while process.poll() is None:
            try: process.wait(timeout=25)
            except subprocess.TimeoutExpired:
                print('Waiting for original App exit PID '+str(process.pid), flush=True)
        require(process.returncode == 0, 'App abnormal exit')
        expected = expected_sha256(args.bytes, 20261113).upper()
        require(output.stat().st_size == args.bytes and sha(output) == expected, 'Output integrity')
        job = load(data/'state.json')['Downloads'][0]
        require(job['Status'] == 'Complete' and job['Sha256'].upper() == expected, 'Catalog result')
        durable = utils.durable_output(output)
        save(meta/'app-result-private.json', dict(passed=True, normalProcessExit=True,
             bytes=args.bytes, connections=args.connections, sha256=expected,
             job=job, appReturn=app_return, origin=origin.snapshot(), postExitFlush=durable[1],
             stdout=stdout, stderr=stderr, messages=messages,
             sourceAppSha256=app['binarySha256'],agentSha256=app['agentSha256'],
             readAddonSha256=sha(ROOT/'benchmarks/speed/finish_app_io_read_addon.js'),
             performanceComparison=False, kernelCauseVerified=False,
             controllerGateAndCrtExcludedFromMfcReturn=True))
    except BaseException as error:
        save(meta/'failure-private.json', dict(type=type(error).__name__, error=str(error),
             pid=process.pid if process else None, exitCode=process.poll() if process else None))
        # Retain the observer through actual exit, including failure cleanup.
        if process:
            if process.poll() is None:
                try:
                    # A non-newline aborts the existing wrapper before MFC entry.
                    # Failed hook setup must not accidentally start a download.
                    process.stdin.write('\n' if entry_started else 'x\n'); process.stdin.flush()
                except OSError:
                    pass
            while process.poll() is None:
                try: process.wait(timeout=25)
                except subprocess.TimeoutExpired:
                    print('Failed owned run remains live; observing PID '+str(process.pid), flush=True)
            save(meta/'failure-private.json',dict(type=type(error).__name__,error=str(error),
                 pid=process.pid,finalExitCode=process.returncode,ownedProcessConfirmedExited=True,
                 entryReleaseAttempted=entry_started,gateAbortedBeforeMfc=not entry_started and process.returncode==5))
        failure = error
    finally:
        if session and process and process.poll() is not None:
            try: session.detach()
            except Exception as error:
                save(meta/'detach-warning.json',dict(error=str(error)))
        origin.close()
        save(meta/'done.json', dict(appProcessEnded=process is not None and process.poll() is not None))
    try:
        kernel = wait_record(meta/'results.json',utils=utils,process_id=reply['pid'])
        ended = wait_record(worker_queue/('exit-'+request_id+'.json'),utils=utils,process_id=worker_pid)
        require(ended['exitCode'] == 0 and kernel['sessionAbsent'] and kernel['stopStatus'] == 0, 'Kernel collector shutdown')
    except BaseException as error:
        save(meta/'collector-failure-private.json',dict(type=type(error).__name__,error=str(error)))
        if failure is None: failure = error
    # Caller analyses retained native/kernel records separately. Never label a
    # transport/integrity pass a schema, performance or kernel-cause pass.
    save(area/'last-app-private.json', dict(meta=str(meta), work=str(work),
         transportAndIntegrityPassed=failure is None, schemaQualificationPassed=False))
    require(process is None or process.poll() is not None, 'Owned App still running; cleanup must wait')
    cleanup_work(work,work_parent,meta,image)
    if failure is not None:
        raise RuntimeError('Owned capture failed; temporary work removed, evidence retained') from failure
    print(json.dumps(dict(transportAndIntegrityPassed=True,schemaQualificationPassed=False,
         payloadRemoved=True,meta=str(meta))),flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--area',type=Path,default=BASE/'scheduling-filter-8.65')
    parser.add_argument('--app-build',type=Path,default=BASE/'app-io-observer-8.65/build-result.json')
    parser.add_argument('--connections',type=int,choices=(1,8),default=8)
    parser.add_argument('--bytes',type=int,choices=(4*1024**2,64*1024**2),default=64*1024**2)
    parser.add_argument('--run',action='store_true')
    args = parser.parse_args()
    try:
        settings = preflight(args.area,args.app_build)
    except (ValueError,KeyError,FileNotFoundError,StopIteration) as error:
        print(json.dumps(dict(ready=False, processStarted=False, error=str(error))))
        raise SystemExit(75)
    if args.run:
        capture(args,*settings)
    else:
        print(json.dumps(dict(ready=True, processStarted=False)))
