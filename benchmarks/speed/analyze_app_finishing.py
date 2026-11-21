"""Read-only finishing phase analysis; requires same-process/same-clock evidence to join."""
import argparse
import hashlib
import json
from pathlib import Path
from analyze_cswitch_prefix import KEY, decode, integer, load, require, verify_controls


def normalized(path):
    return path.removeprefix('\\\\?\\').replace('\\','/').lower()


def native_phase(snapshot, size, app_return):
    require(integer(size) and size > 0, 'Body size')
    hz = snapshot.get('frequency')
    require(integer(hz) and hz > 0, 'Native frequency')
    require(all(integer(snapshot.get(k)) and snapshot[k]==0 for k in ('dropped','openFiles','openMappings')),
            'Native capture incomplete')
    require(app_return.get('passed') is True and integer(app_return.get('exitCode'))
            and app_return['exitCode']==0, 'MFC result')
    entry, returned = app_return.get('appEntryCounter'), app_return.get('appReturnedCounter')
    require(integer(entry) and integer(returned) and 0 < entry < returned, 'MFC interval')
    events = snapshot.get('records', [])
    require(events and all(integer(e.get('begin')) and integer(e.get('end'))
                           and e['begin'] <= e['end'] for e in events), 'Native event interval')
    body = [e for e in events if e.get('api') == 'NtWriteFile'
            and normalized(e.get('path','')).endswith('.data')]
    require(body and len({normalized(e['path']) for e in body}) == 1, 'Ambiguous body file')
    body_path = normalized(body[0]['path'])
    cursor = 0
    for event in sorted(body,key=lambda e:int(e.get('byteOffset',-1))):
        require(event.get('ok') is True and event.get('pending') is False
                and event.get('status') == 0 and int(event.get('byteOffset',-1)) == cursor
                and integer(event.get('completed')) and event['completed'] == event.get('requested')
                and event['completed'] > 0 and entry <= event['begin'] <= event['end'] <= returned,
                'Body write coverage/status')
        cursor += event['completed']
    require(cursor == size, 'Body byte coverage')
    last_write = max(e['end'] for e in body)
    flushes = [e for e in events if e.get('api') in ('NtFlushBuffersFile','NtFlushBuffersFileEx')
               and normalized(e.get('path','')) == body_path
               and e['begin'] >= last_write and e['end'] <= returned]
    require(len(flushes) == 1, 'Final body flush selection is not unique')
    flush = flushes[0]
    require(flush.get('ok') is True and flush.get('pending') is False and flush.get('status') == 0
            and integer(flush.get('tid')) and flush['tid'] > 0, 'Final native flush status')
    for event in body+[flush]:
        api = 'WriteFile' if event['api']=='NtWriteFile' else 'FlushFileBuffers'
        parents = [p for p in events if p.get('api') == api and p.get('tid') == event.get('tid')
                   and p.get('handle') == event.get('handle') and normalized(p.get('path','')) == body_path
                   and p['begin'] <= event['begin'] <= event['end'] <= p['end']
                   and p.get('ok') is True]
        require(len(parents) == 1, 'Native call lacks a unique successful Win32 envelope')
    return dict(frequency=hz,bodyBytes=cursor,bodyWrites=len(body),lastWriteCounter=last_write,
                mfcReturnCounter=returned,flush=flush,bodyPath=body_path)


def public_native(phase):
    hz=phase['frequency'];flush=phase['flush']
    return dict(bodyBytes=phase['bodyBytes'],bodyWrites=phase['bodyWrites'],
                finalNativeFlushMs=(flush['end']-flush['begin'])*1000/hz,
                lastBodyWriteToMfcReturnMs=(phase['mfcReturnCounter']-phase['lastWriteCounter'])*1000/hz)


def thread_lifetimes(target, begin, end):
    active, spans = {}, {}
    previous=begin
    require(target.get('threadLifecycle'), 'Missing thread lifecycle')
    for row in target['threadLifecycle']:
        time,tid,op=row.get('counter'),row.get('thread'),row.get('opcode')
        require(integer(time) and previous <= time <= end and integer(tid) and tid > 0,
                'Thread lifecycle interval')
        previous=time
        if op in ('start','dcstart'):
            require(tid not in active,'Duplicate thread start');active[tid]=time
        elif op=='end':
            require(tid in active,'Unmatched thread end')
            spans.setdefault(tid,[]).append((active.pop(tid),time))
        else:raise ValueError('Unknown thread lifecycle operation')
    for tid,start in active.items():spans.setdefault(tid,[]).append((start,end))
    return spans


def union_ticks(intervals):
    result=0;last=None
    for lo,hi in sorted(intervals):
        require(lo<=hi,'Inverted interval')
        if last is None:result+=hi-lo;last=hi
        elif hi>last:result+=hi-max(lo,last);last=hi
    return result


def request_key(row):
    if 'irpToken' in row:
        require(isinstance(row['irpToken'],str) and row['irpToken'], 'Invalid IRP token')
        return ('token',row['irpToken'])
    values=[row[k] for k in ('Irp','IrpPtr') if k in row]
    require(values and all(integer(v) and v>0 for v in values) and len(set(values))==1,
            'Missing/ambiguous request identity')
    return ('pointer',values[0])


def linked_disk_intervals(target, begin, end, a, b, tid, hz):
    starts={};disk=[];previous=begin
    for row in target.get('diskEvents',[]):
        require(integer(row.get('counter')) and previous<=row['counter']<=end,'Disk record interval/order')
        previous=row['counter']
        if row.get('opcode') in ('readinit','writeinit','flushinit'):
            key=(request_key(row),row['counter']);require(key not in starts,'Duplicate disk initiation')
            starts[key]=row
        elif row.get('linkedToTargetInit') is True:
            key=(request_key(row),row.get('targetInitCounter'))
            require(key in starts,'Disk completion lacks its initiation')
            initial=starts.pop(key)
            expected={'readinit':'read','writeinit':'write','flushinit':'flushbuffers'}[initial['opcode']]
            require(row.get('opcode')==expected and row['counter']>=initial['counter']
                    and row.get('targetInitThread')==initial.get('targetInitThread')
                    and row.get('targetInitOpcode')==initial['opcode'],'Disk correlation type/time/thread')
            lo,hi=initial['counter'],row['counter']
            if row['targetInitThread']==tid and a<=lo<=hi<=b:
                size=0 if expected=='flushbuffers' else row.get('TransferSize')
                require(integer(size) and size>=0,'Disk transfer size')
                disk.append(dict(opcode=expected,beginAfterNativeStartMs=(lo-a)*1000/hz,
                    durationMs=(hi-lo)*1000/hz,transferBytes=size,diskNumber=row.get('DiskNumber'),interval=(lo,hi)))
    require(target.get('pendingDiskOperations')==len(starts),'Pending disk accounting')
    flushes=[d for d in disk if d['opcode']=='flushbuffers']
    groups={kind:dict(requests=len(rows),transferBytes=sum(d['transferBytes'] for d in rows),
             intervalUnionMs=union_ticks([d['interval'] for d in rows])*1000/hz)
            for kind in ('read','write','flushbuffers')
            for rows in [[d for d in disk if d['opcode']==kind]]}
    return dict(diskCorrelationsValidated=True,pendingTargetDiskOperations=len(starts),
        unmatchedOwnedDiskCompletions=target.get('unmatchedOwnedDiskCompletions'),
        linkedDiskFlushIntervalUnionMs=groups['flushbuffers']['intervalUnionMs'],
        linkedDiskRequestIntervalUnionMs=union_ticks([d['interval'] for d in disk])*1000/hz,
        linkedDiskRequestKinds=groups,
        linkedDiskRequests=[{k:v for k,v in d.items() if k!='interval'} for d in disk],
        linkedDiskFlushes=[{k:v for k,v in d.items() if k not in ('interval','opcode','transferBytes')} for d in flushes])


def join_records(kernel, snapshot, receipt, identity, expected_pid, expected_image, expected_hash, controls):
    result=dict(passed=False,kernelJoined=False,fullSchemaQualificationPassed=False,
                kernelCauseVerified=False,performanceComparison=False,payloadIntegrityReverified=False,
                fileAttributionFromKernelVerified=False)
    try:
        verify_controls(controls)
        require(integer(expected_pid) and expected_pid>0,'Expected PID')
        require(receipt.get('passed') is True and receipt.get('normalProcessExit') is True
                and receipt.get('sourceAppSha256')==expected_hash,'App receipt/provenance')
        digest=receipt.get('sha256','')
        require(isinstance(digest,str) and len(digest)==64
                and all(c in '0123456789abcdefABCDEF' for c in digest)
                and receipt.get('job',{}).get('Status')=='Complete'
                and receipt['job'].get('Sha256','').upper()==digest.upper(),'App integrity/catalog receipt')
        require(identity.get('pid')==expected_pid
                and Path(identity.get('image','')).name.lower()==expected_image.lower(),'Native identity')
        phase=native_phase(snapshot,receipt['bytes'],receipt['appReturn'])
        require(identity.get('frequency')==phase['frequency'],'Native identity clock')
        require(kernel.get('traceProfile')=='disk-scheduling-target-threads-v3'
                and kernel.get('enabledFlags')==0x10000D13
                and kernel.get('fileOperationAttributionSupported') is False,'Kernel profile')
        require(kernel.get('started') is True and kernel.get('reason')=='owner-complete'
                and kernel.get('sessionAbsent') is True and kernel.get('postStopQueryStatus')==4201,
                'Kernel lifetime/shutdown')
        for key in ('eventsLost','buffersLost','stopStatus','processTraceStatus'):
            require(integer(kernel.get(key)) and kernel[key]==0,'Kernel '+key)
        require(all(kernel.get(k) is False for k in ('callbackFailed','ambiguous','limit','normalWorkloadLaunchedByCollector')),
                'Kernel capture status')
        require(kernel.get('counterFrequency')==phase['frequency'],'Clock frequency mismatch')
        require(kernel.get('closeTraceStatus') in (0,7007),'Kernel consumer close')
        require(integer(kernel.get('actualBuffers')) and integer(kernel.get('bufferSizeKiB'))
                and 0<kernel['actualBuffers']*kernel['bufferSizeKiB']<=16384,'Kernel buffers')
        targets=kernel.get('targets',[])
        require(len(targets)==1,'Target count');target=targets[0]
        require(target.get('pid')==expected_pid and target.get('image')==expected_image.lower()
                and target.get('imageMatches') is True and target.get('starts')==target.get('ends')==1,
                'Kernel target identity')
        require(all(target.get(k) is False for k in ('callbackFailed','ambiguous','limit'))
                and target.get('targetSchemaErrors')==0 and target.get('targetSchemaFailureInventory')=={},
                'Kernel target status')
        require(target.get('pendingFlushes')==0 and target.get('flushPairs')==[],'Unexpected FileIo capture')
        begin,end=target.get('beginCounter'),target.get('endCounter')
        require(integer(begin) and integer(end) and 0<begin<=receipt['appReturn']['appEntryCounter']
                <phase['mfcReturnCounter']<=snapshot['now']<=end,'Cross-source process/clock boundaries')
        inventory=kernel['unknownSchemaInventory']
        require(inventory.get('capped') is False and inventory.get('targetThreadFilter') is True
                and inventory.get('selectedIdPrefixReadForFiltering') is True
                and inventory.get('payloadsFullyDecoded') is False,'Prefix capture scope')
        require(set(inventory['schemas'])=={KEY}
                and inventory['schemas'][KEY]['count']==kernel.get('unknownMetadataEvents'),
                'Unknown schema accounting')
        raw=inventory['threadWindowRecords']
        require(0<len(raw)<=16384 and inventory.get('retainedTargetThreadEvents')==len(raw)
                and inventory.get('unsupportedTargetThreadForms')==0
                and inventory.get('threadWindowEvents')==len(raw)+inventory.get('unmatchedTargetThreadEvents',-1),
                'Filtered record accounting')
        rows=[decode(inventory['schemas'][KEY],row) for row in raw]
        require(all(integer(r['counter']) and begin<=r['counter']<=end for r in rows)
                and all(a['counter']<=b['counter'] for a,b in zip(rows,rows[1:])),'Prefix timestamp order')
        lives=thread_lifetimes(target,begin,end)
        def belongs(tid,a,b):return any(lo<=a<=b<=hi for lo,hi in lives.get(tid,[]))
        require(all(belongs(r['newThread'],r['counter'],r['counter'])
                    or belongs(r['oldThread'],r['counter'],r['counter']) for r in rows),
                'Record outside known target thread lifetime')
        flush=phase['flush'];a,b,tid=flush['begin'],flush['end'],flush['tid'];hz=phase['frequency']
        require(belongs(tid,a,b),'Native flush outside known thread lifetime')
        result.update(nativePhaseValidated=True,**public_native(phase),
                      **linked_disk_intervals(target,begin,end,a,b,tid,hz))
        relevant=[r for r in rows if tid in (r['newThread'],r['oldThread'])
                  and belongs(tid,r['counter'],r['counter'])]
        before=[r for r in relevant if r['counter']<=a]
        require(before and (before[-1]['newThread']==tid or before[-1]['counter']==a),
                'No running-thread witness at native entry')
        running=None;pending=None;off=[];state_rows=[]
        for row in relevant:
            require(row['newThread']!=row['oldThread'],'Self context switch')
            if row['newThread']==tid:
                require(running is not True,'Duplicate switch-in');running=True
                if pending is not None:
                    lo,hi=max(a,pending['counter']),min(b,row['counter'])
                    if hi>lo:
                        require(pending['counter']>=a and row['counter']<=b,
                                'Native boundary intersects an unresolved off-CPU interval')
                        off.append((lo,hi));state_rows.append(dict(beginAfterNativeStartMs=(lo-a)*1000/hz,
                            offCpuMs=(hi-lo)*1000/hz,oldStateByte=pending['oldState'],
                            oldWaitReasonByte=pending['oldWaitReason']))
                    pending=None
            else:
                require(running is not False,'Duplicate switch-out');running=False;pending=row
        require(pending is None or pending['counter']>=b,'Unfinished native off-CPU interval')
        after=[r for r in relevant if r['counter']<=b]
        require(after and (after[-1]['newThread']==tid or after[-1]['counter']==b),
                'No running-thread witness at native return')
        off_union=union_ticks(off)
        result.update(passed=True,kernelJoined=True,failures=[],**public_native(phase),
                      observedOffCpuMs=off_union*1000/hz,
                      notClassifiedOffCpuMs=(b-a-off_union)*1000/hz,
                      offCpuIntervals=state_rows,
                      limitations='Instrumented selected-prefix model. Disk intervals can overlap and are unioned, not summed. They identify linked request intervals, not exclusive hardware/filter time or a kernel file mapping. Unobserved/system-worker IO and the full version-5 schema remain outside this model.')
    except (ValueError,KeyError,TypeError) as error:
        result['failures']=[str(error)]
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--legacy-app-result',type=Path)
    mode.add_argument('--run-dir',type=Path)
    parser.add_argument('--controls',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.legacy_app_result:
        r=load(args.legacy_app_result)
        require(r.get('passed') is True and r.get('normalProcessExit') is True
                and r.get('hashMatches') is True,'Prior App integrity receipt')
        result=dict(passed=True,nativeOnly=True,kernelJoined=False,kernelCauseVerified=False,
                    fullSchemaQualificationPassed=False,
                    payloadIntegrityReverified=False,performanceComparison=False,
                    **public_native(native_phase(r['snapshot'],r['size'],r['appReturn'])))
        result['inputSha256']=hashlib.sha256(args.legacy_app_result.read_bytes()).hexdigest().upper()
    else:
        require(args.controls is not None,'TDH controls required')
        process=load(args.run_dir/'app-process-private.json')
        controller=load(args.run_dir/'controller-private.json')
        files={'kernel':args.run_dir/'results.json','native':args.run_dir/'native-io-private.json',
               'app':args.run_dir/'app-result-private.json','identity':args.run_dir/'native-identity-private.json'}
        result=join_records(load(files['kernel']),load(files['native']),load(files['app']),
                           load(files['identity']),process['pid'],Path(process['command'][0]).name,
                           controller['sourceAppSha256'],load(args.controls))
        result['inputSha256']={name:hashlib.sha256(path.read_bytes()).hexdigest().upper()
                              for name,path in files.items()}
    args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k not in ('offCpuIntervals','linkedDiskRequests','linkedDiskFlushes','inputSha256','limitations')}))
    raise SystemExit(0 if result['passed'] else 1)
