'use strict';
// Real loopback HTTP -> native Grabber -> saved files; no browser automation.
const fs = require('fs');
const path = require('path');
const http = require('http');
const crypto = require('crypto');
const { spawn } = require('child_process');
const exe = path.resolve(process.argv[2]);
const output = path.resolve(process.argv[3]);
if (fs.existsSync(output)) throw Error('Use a new output directory.');
fs.mkdirSync(output, { recursive: true });
const types = ['application/ecmascript', 'application/javascript', 'application/x-ecmascript', 'application/x-javascript', 'text/ecmascript', 'text/javascript', 'text/javascript1.0', 'text/javascript1.1', 'text/javascript1.2', 'text/javascript1.3', 'text/javascript1.4', 'text/javascript1.5', 'text/jscript', 'text/livescript', 'text/x-ecmascript', 'text/x-javascript'];
const rejected = ['application/json', 'application/ld+json', 'text/plain', 'text/javascript-invalid'];
const attributes = [
  ["type=' \tTEXT/JAVASCRIPT\r\n\f'", true],
  ["language='JavaScript1.2'", true], ["language='VBScript'", false],
  ["language='VBScript' type=''", true], ["language='VBScript' type", true],
  ["type='text/javascript' type='application/json'", true],
  ["type='application/json' type='text/javascript'", false], ["type=' module '", false],
];
const payload = Buffer.from('Exact MIME discovery fixture bytes.');
const requests = [], checks = [];
const routes = new Map();
let html = '<!doctype html><title>MIME discovery</title>';
types.forEach((type, i) => {
  html += `<script type="${type.toUpperCase()}" src="/script-${i}.js"></script><script type="${type}">image.src='/inline-${i}.bin';</script>`;
  routes.set(`/script-${i}.js`, [type.toUpperCase() + '; charset=utf-8', `image.src='/external-${i}.bin';`]);
});
rejected.forEach((type, i) => {
  html += `<script src="/rejected-${i}.js"></script><script type="${type}">image.src='/excluded-inline-${i}.bin';</script>`;
  routes.set(`/rejected-${i}.js`, [type, `image.src='/excluded-external-${i}.bin';`]);
});
attributes.forEach(([attrs, accepted], i) => {
  const asset = `/${accepted ? 'classified' : 'excluded-attribute'}-${i}.bin`;
  html += `<script ${attrs} src="/classified-${i}.js"></script>`;
  routes.set(`/classified-${i}.js`, ['application/javascript', `image.src='${asset}';`]);
});
for (const attrs of ["src='/script-0.js'", "src=''", "src", "type='module' src='/script-0.js'"]) html += `<script ${attrs}>image.src='/excluded-fallback.bin';</script>`;
for (const tag of ['textarea','title']) html += `<${tag}><img src='/excluded-text.bin'></${tag}-suffix><script>image.src='/excluded-text-script.bin';</script></${tag}>`;
html += `<script>const text="</script-suffix><img src='/excluded-boundary.bin'>";image.src='/boundary-script.bin';</script><style>.a{content:"</style-suffix><img src='/excluded-style.bin'>"}.b{background:url('/boundary-style.bin')}</style>`;
routes.set('/index.html', ['text/html', html]);
const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost').pathname;
  requests.push({ path: url, method: req.method });
  const entry = routes.get(url) || (url.endsWith('.bin') ? ['application/octet-stream', payload] : null);
  if (!entry) { res.writeHead(404, { 'Content-Length': 0 }); res.end(); return; }
  const body = Buffer.from(entry[1]);
  res.writeHead(200, { 'Content-Type': entry[0], 'Content-Length': body.length, Connection: 'close' });
  res.end(req.method === 'HEAD' ? undefined : body);
});
function check(name, passed) {
  checks.push({ name, passed: !!passed });
  if (!passed) throw Error(name);
}
(async () => {
  try {
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const origin = `http://127.0.0.1:${server.address().port}`;
    const input = path.join(output, 'input.json');
    fs.writeFileSync(input, JSON.stringify({ url: origin + '/index.html' }));
    await new Promise((resolve, reject) => {
      const child = spawn(exe, ['--grabber-link-spec', input], { windowsHide: true });
      const log = fs.createWriteStream(path.join(output, 'native.log'));
      child.stdout.pipe(log); child.stderr.pipe(log);
      child.once('error', reject);
      child.once('exit', code => code === 0 ? resolve() : reject(Error(`Native exit ${code}`)));
    });
    const result = JSON.parse(fs.readFileSync(path.join(output, 'result.json'), 'utf8'));
    check('Native exploration and publication complete', result.project.LinkConversionState === 'Complete' && result.files.every(f => f.status === 'Complete'));
    types.forEach((type, i) => {
      for (const kind of ['inline', 'external']) {
        const file = result.files.find(f => f.url === `${origin}/${kind}-${i}.bin`);
        check(`${type}: ${kind} discovery publishes exact bytes`, file && fs.readFileSync(file.path).equals(payload));
      }
    });
    attributes.forEach(([attrs, accepted], i) => {
      if (!accepted) return;
      const file = result.files.find(f => f.url === `${origin}/classified-${i}.bin`);
      check(`${attrs}: classified script publishes exact asset bytes`, file && fs.readFileSync(file.path).equals(payload));
    });
    for (const kind of ['script','style']) check(`Closing boundary preserves ${kind} asset`, result.files.some(f => f.url === `${origin}/boundary-${kind}.bin` && fs.readFileSync(f.path).equals(payload)));
    check('Non-JavaScript payload URLs are never requested', requests.every(r => !r.path.startsWith('/excluded-')));
    check('Non-JavaScript payload URLs are never queued', result.files.every(f => !f.url.includes('/excluded-')));
    check('All published hashes match saved files', result.files.every(f => crypto.createHash('sha256').update(fs.readFileSync(f.path)).digest('hex') === f.sha256));
    console.log(`${checks.length} HTTP MIME checks passed.`);
  } catch (error) {
    checks.push({ name: 'Fixture completion', passed: false, error: error.message });
    process.exitCode = 1;
    console.error(error.message);
  } finally {
    server.close();
    fs.writeFileSync(path.join(output, 'acceptance.json'), JSON.stringify({ checks, requests, scope: 'Local HTTP native Grabber discovery and byte-exact publication; not browser UI or module import traversal.' }, null, 2));
  }
})();
