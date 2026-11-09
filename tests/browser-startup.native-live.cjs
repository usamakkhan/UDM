'use strict';
const assert = require('node:assert/strict');
const {execFileSync, spawn} = require('node:child_process');
const {randomUUID} = require('node:crypto');
const fs = require('node:fs');
const path = require('node:path');

const [desktop, output, repeat = '1', mode = 'concurrent'] = process.argv.slice(2);
if (!desktop || !output || !/^[1-9]\d*$/.test(repeat) || !['single', 'concurrent', 'relaunch'].includes(mode)) {
  throw new Error('Pass a staged UDM.exe, a fresh output folder, an optional run count, and single, concurrent, or relaunch.');
}
const source = path.dirname(path.resolve(desktop));
const root = path.resolve(output);
fs.mkdirSync(root);

function nativeMessage(host, tag, message) {
  return new Promise((resolve, reject) => {
    const body = Buffer.from(JSON.stringify(message));
    const length = Buffer.alloc(4);
    length.writeUInt32LE(body.length);
    const started = performance.now();
    const child = spawn(host, [], {
      env: {...process.env, UDM_INSTANCE_TAG: tag},
      windowsHide: true,
      stdio: ['pipe', 'pipe', 'ignore'],
    });
    const chunks = [];
    const timeout = setTimeout(() => {
      child.kill();
      reject(new Error('Native startup reply timed out'));
    }, 28000);
    child.stdout.on('data', chunk => chunks.push(chunk));
    child.stdin.on('error', reject);
    child.on('error', error => {
      clearTimeout(timeout);
      reject(error);
    });
    child.on('close', code => {
      clearTimeout(timeout);
      try {
        assert.equal(code, 0);
        const reply = Buffer.concat(chunks);
        assert.equal(reply.readUInt32LE(0), reply.length - 4);
        resolve({reply: JSON.parse(reply.subarray(4)), ms: Math.round(performance.now() - started)});
      } catch (error) {
        reject(error);
      }
    });
    child.stdin.end(Buffer.concat([length, body]));
  });
}

function ownedDesktops(exe) {
  const command = [
    'Get-Process',
    'Where-Object {$_.ProcessName -eq "UDM" -and $_.Path -eq $env:UDM_STARTUP_FIXTURE_EXE}',
    'ForEach-Object { "$($_.Id) $($_.MainWindowHandle)" }',
  ].join(' | ');
  const lines = execFileSync('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', command], {
    env: {...process.env, UDM_STARTUP_FIXTURE_EXE: exe},
    encoding: 'utf8',
    windowsHide: true,
  }).trim();
  return lines ? lines.split(/\r?\n/).map(line => line.split(' ').map(Number)) : [];
}

function relaunchDesktop(exe, tag, args) {
  execFileSync(exe, args, {
    env: {...process.env, UDM_INSTANCE_TAG: tag},
    stdio: 'ignore',
    timeout: 15000,
    windowsHide: true,
  });
}

async function trial(index) {
  const folder = path.join(root, String(index));
  const data = path.join(folder, 'data');
  fs.mkdirSync(data, {recursive: true});
  for (const file of ['UDM.exe', 'Udm.NativeHost.exe']) {
    fs.linkSync(path.join(source, file), path.join(folder, file));
  }
  const assets = path.join(source, 'assets');
  if (fs.existsSync(assets)) fs.cpSync(assets, path.join(folder, 'assets'), {recursive: true});
  fs.writeFileSync(path.join(folder, 'udm-data.json'), JSON.stringify({dataDirectory: data}));
  fs.writeFileSync(path.join(data, 'state.json'), JSON.stringify({
    Schema: 1,
    Settings: {DownloadFolder: path.join(folder, 'files'), CategoryFolders: false, PrefetchFileInfo: false},
    Queues: [{Name: 'Main queue', Enabled: true, Parallel: 1}],
    Downloads: [],
    Projects: [],
  }));
  const exe = path.join(folder, 'UDM.exe');
  const host = path.join(folder, 'Udm.NativeHost.exe');
  const tag = 'startup-' + randomUUID().slice(0, 12);
  try {
    const absent = await nativeMessage(host, tag, {action: 'preferences-if-running'});
    assert.equal(absent.reply.ok, false);
    assert.deepEqual(ownedDesktops(exe), [], 'A read-only preference request must leave UDM closed');
    const ids = mode === 'concurrent' ? [1, 2] : [1];
    const messages = ids.map(id => ({
      action: 'add',
      url: 'http://127.0.0.1:1/startup-' + id + '.bin',
      filename: 'startup-' + id + '.bin',
      downloadLater: true,
    }));
    const results = await Promise.all(messages.map(message => nativeMessage(host, tag, message)));
    assert(results.every(({reply}) => reply.ok && reply.id), JSON.stringify(results));
    assert.equal(new Set(results.map(result => result.reply.id)).size, ids.length);
    const warm = await nativeMessage(host, tag, {action: 'diagnostics'});
    assert.equal(warm.reply.ok, true);
    assert.equal(warm.reply.downloads, ids.length);
    assert.equal(path.resolve(warm.reply.dataDirectory).toLowerCase(), data.toLowerCase());
    const records = JSON.parse(fs.readFileSync(path.join(data, 'state.json'))).Downloads;
    assert.equal(records.length, ids.length);
    assert(records.every(record => record.Status === 'Paused' && record.Received === 0));
    const processes = ownedDesktops(exe);
    assert.equal(processes.length, 1);
    assert.equal(processes[0][1], 0, 'The background desktop must remain hidden');
    if (mode === 'relaunch') {
      relaunchDesktop(exe, tag, ['--background']);
      assert.deepEqual(ownedDesktops(exe), processes, 'A repeated background launch must keep the original desktop hidden');
      relaunchDesktop(exe, tag, []);
      const shown = ownedDesktops(exe);
      assert.equal(shown.length, 1);
      assert.equal(shown[0][0], processes[0][0]);
      assert.notEqual(shown[0][1], 0, 'A foreground launch still shows the original desktop');
    }
    return {version: warm.reply.version, mode, absentMs: absent.ms, coldMs: results.map(result => result.ms), warmMs: warm.ms};
  } finally {
    for (const [pid] of ownedDesktops(exe)) {
      try {
        process.kill(pid);
      } catch (error) {
        if (error.code !== 'ESRCH') throw error;
      }
    }
  }
}

(async () => {
  const results = [];
  for (let index = 1; index <= Number(repeat); index++) {
    const result = await trial(index);
    results.push(result);
    console.log(JSON.stringify(result));
  }
  fs.writeFileSync(path.join(root, 'results.json'), JSON.stringify(results, null, 2));
})().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
