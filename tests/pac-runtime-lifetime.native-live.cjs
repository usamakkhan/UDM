'use strict';
// Isolated engine helper only. Never invokes the Windows PAC service.
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const {spawn} = require('node:child_process');
const root = path.resolve(process.argv[2]);
const exe = path.resolve(process.argv[3]);
assert(!fs.existsSync(root));
assert(fs.existsSync(exe));
fs.mkdirSync(root, {recursive:true});
const results = [];
async function check(name, source, crashParent) {
  const file = path.join(root, name + '.pac');
  fs.writeFileSync(file, source);
  const parent = spawn(exe, ['--supervise', file, crashParent ? '5000' : name === 'normal-exit' ? '1000' : '100'],
    {windowsHide:true, stdio:['ignore','pipe','pipe']});
  let output = '', workerPid, killedAt, stderr = '';
  const guard = setTimeout(() => parent.kill(), 8000);
  const closed = new Promise((resolve,reject) => {
    parent.once('error', reject);
    parent.once('close', (code,signal) => resolve({code,signal}));
  });
  parent.stderr.on('data', data => stderr += data);
  parent.stdout.on('data', data => {
    output += data;
    if (!workerPid && output.includes('\n')) {
      const start = JSON.parse(output.split('\n')[0]);
      assert(start.assignedBeforeExecution);
      workerPid = start.workerPid;
      if (crashParent) { killedAt = Date.now(); parent.kill(); }
    }
  });
  try {
    const status = await closed;
    assert(workerPid, 'Supervisor never reported an assigned worker: ' + stderr);
    if (crashParent) {
      let alive = true;
      while (alive && Date.now() - killedAt < 1000) {
        try { process.kill(workerPid, 0); }
        catch (error) { if (error.code !== 'ESRCH') throw error; alive = false; }
        if (alive) await new Promise(resolve => setTimeout(resolve, 10));
      }
      assert(!alive, 'Worker survived parent termination');
      results.push({name, ok:true, workerExited:true, afterParentExitMs:Date.now()-killedAt});
    } else {
      assert.equal(status.code, 0, stderr);
      const final = JSON.parse(output.trim().split('\n').at(-1));
      assert(final.workerExited);
      assert.equal(final.deadlineReached, name !== 'normal-exit');
      assert(final.wallMs < 1000, 'Job closure did not stop the worker promptly');
      results.push({name, ok:true, ...final});
    }
    console.log('PASS ' + name);
  } finally { clearTimeout(guard); }
}
(async () => {
  await check('normal-exit', 'function FindProxyForURL(){return "DIRECT"}', false);
  await check('parent-deadline', 'while(true){}', false);
  await check('parent-crash', 'while(true){}', true);
})().catch(error => {
  results.push({failed:true,error:error.message});
  console.error(error);process.exitCode=1;
}).finally(() => fs.writeFileSync(path.join(root,'results.json'),JSON.stringify({prototypeOnly:true,results},null,2)));
