'use strict';
// Downloads through the native Grabber, closes the fixture server, then opens
// the actual saved files in installed Chrome and Edge using isolated profiles.
const fs = require('fs');
const path = require('path');
const http = require('http');
const crypto = require('crypto');
const { spawn } = require('child_process');
const { pathToFileURL, fileURLToPath } = require('url');
const { chromium } = require('playwright');
const exe = path.resolve(process.argv[2]);
const output = path.resolve(process.argv[3]);
if (fs.existsSync(output)) throw new Error('Use a new fixture output directory.');
fs.mkdirSync(output, { recursive: true });
const checks = [];
function check(name, ok, extra = {}) {
  checks.push({ name, passed: !!ok, ...extra });
  console.log(`${ok ? 'PASS' : 'FAIL'} ${name}`);
  if (!ok) throw new Error(name);
}
const resources = new Map([
  ['/docs/start.php', ['text/html; charset=utf-8', `<!doctype html><html><head><meta charset="utf-8"><base href="/docs/"><title>UDM saved website</title><link rel="stylesheet" href="../styles/main.css"><script defer src="../scripts/app.js"></script></head><body><h1 id="title">UDM offline site</h1><p id="script">Waiting for script</p><img id="picture" src="../media/pixel.png"><a id="next" href="next.php#chapter">Next page</a><a id="remote" href="https://outside.invalid/not-saved">Online page</a><form id="form" action="submit"><button>Send</button></form></body></html>`]],
  ['/docs/next.php', ['text/html', `<!doctype html><html><head><title>Second saved page</title><link rel="stylesheet" href="../styles/main.css"></head><body><h1 id="chapter">Second saved page</h1><a id="back" href="start.php">Back</a><img id="picture" src="../media/pixel.png"></body></html>`]],
  ['/styles/main.css', ['text/css', `@import "nested/theme.css";body{font:18px sans-serif;background:#f0f5ff}h1{color:rgb(12,34,56)}#picture{width:16px;height:16px}#next{background-image:url('../media/pixel.png')}`]],
  ['/styles/nested/theme.css', ['text/css', '#script{color:rgb(23,45,67)}']],
  ['/scripts/app.js', ['application/javascript', 'document.getElementById("script").textContent="Local script loaded";']],
  ['/images/pixel.png', ['image/png', Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jRZkAAAAASUVORK5CYII=', 'base64')]],
]);
const requests = [];
const server = http.createServer((req, res) => {
  const route = new URL(req.url, 'http://localhost').pathname;
  requests.push({ path: route, method: req.method });
  if (route === '/media/pixel.png') { res.writeHead(302, { Location: '/images/pixel.png', 'Content-Length': 0 }); res.end(); return; }
  const entry = resources.get(route);
  if (!entry) { res.writeHead(404, { 'Content-Length': 0 }); res.end(); return; }
  const body = Buffer.isBuffer(entry[1]) ? entry[1] : Buffer.from(entry[1]);
  const headers = { 'Content-Type': entry[0], 'Content-Length': body.length, 'Accept-Ranges': 'bytes', ETag: '"site-fixture-v1"', Connection: 'close' };
  let data = body, status = 200;
  const range = /^bytes=(\d+)-(\d*)$/.exec(req.headers.range || '');
  if (range && req.method !== 'HEAD') {
    const start = Number(range[1]), end = range[2] ? Number(range[2]) : body.length - 1;
    if (start >= body.length || end >= body.length || end < start) { res.writeHead(416, { 'Content-Range': `bytes */${body.length}`, 'Content-Length': 0 }); res.end(); return; }
    status = 206; data = body.subarray(start, end + 1); headers['Content-Range'] = `bytes ${start}-${end}/${body.length}`; headers['Content-Length'] = data.length;
  }
  res.writeHead(status, headers); res.end(req.method === 'HEAD' ? undefined : data);
});
function runNative(spec) {
  return new Promise((resolve, reject) => {
    const child = spawn(exe, ['--grabber-link-spec', spec], { windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
    const log = fs.createWriteStream(path.join(output, 'native.log'));
    child.stdout.pipe(log); child.stderr.pipe(log);
    child.once('error', reject); child.once('exit', code => code === 0 ? resolve() : reject(new Error(`Native fixture failed (${code}). See native.log.`)));
  });
}
(async () => {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  const origin = `http://127.0.0.1:${server.address().port}`;
  const startUrl = origin + '/docs/start.php';
  const collision = path.join(output, 'downloads/docs/start.php.html');
  fs.mkdirSync(path.dirname(collision), { recursive: true }); fs.writeFileSync(collision, 'Existing user file');
  const spec = path.join(output, 'input.json'); fs.writeFileSync(spec, JSON.stringify({ url: startUrl }));
  try { await runNative(spec); } finally { await new Promise(resolve => server.close(resolve)); }
  const result = JSON.parse(fs.readFileSync(path.join(output, 'result.json'), 'utf8'));
  check('Native Grabber finishes all selected files and link conversion', result.project.LinkConversionState === 'Complete' && result.files.length >= 6 && result.files.every(f => f.status === 'Complete'));
  check('Native Grabber leaves the disabled queue disabled', result.queueEnabled === false);
  check('Existing destination content is preserved', fs.readFileSync(collision, 'utf8') === 'Existing user file');
  const main = result.files.find(f => f.url === startUrl);
  check('Main page uses the actual numbered HTML destination', main && main.path !== collision && main.path.endsWith('.html'));
  const mainSource = fs.readFileSync(main.path, 'utf8');
  check('Original website folders are retained', result.files.some(f => /styles[\\/]nested[\\/]theme\.css$/.test(f.path)));
  check('All published file hashes match their recorded values', result.files.every(f => crypto.createHash('sha256').update(fs.readFileSync(f.path)).digest('hex') === f.sha256));
  check('Form destinations are not requested during exploration', requests.every(r => r.path !== '/docs/submit'));
  check('Base element is removed after its references are resolved', !/<base\b/i.test(mainSource));
  const bytesAfterNative = JSON.stringify(result.files.map(f => [f.path, f.sha256]));
  for (const channel of ['chrome', 'msedge']) {
    const browser = await chromium.launch({ channel, headless: true });
    try {
      const context = await browser.newContext({ viewport: { width: 1000, height: 660 } });
      const page = await context.newPage(), webRequests = [];
      page.on('request', req => { if (/^https?:/.test(req.url())) webRequests.push(req.url()); });
      await page.goto(pathToFileURL(main.path).href, { waitUntil: 'load' });
      check(`${channel}: saved HTML opens with the server offline`, await page.title() === 'UDM saved website');
      check(`${channel}: external script loads from disk`, await page.locator('#script').textContent() === 'Local script loaded');
      check(`${channel}: linked stylesheet loads from disk`, await page.locator('#title').evaluate(e => getComputedStyle(e).color) === 'rgb(12, 34, 56)');
      check(`${channel}: nested CSS import loads from disk`, await page.locator('#script').evaluate(e => getComputedStyle(e).color) === 'rgb(23, 45, 67)');
      check(`${channel}: redirected image loads from disk`, await page.locator('#picture').evaluate(e => e.complete && e.naturalWidth === 1));
      check(`${channel}: CSS asset reference points to a local file`, (await page.locator('#next').evaluate(e => getComputedStyle(e).backgroundImage)).includes('file:///'));
      check(`${channel}: omitted link retains its original website address`, await page.locator('#remote').getAttribute('href') === 'https://outside.invalid/not-saved');
      check(`${channel}: form keeps its original server destination`, await page.locator('#form').getAttribute('action') === origin + '/docs/submit');
      await page.screenshot({ path: path.join(output, channel + '-offline.png') });
      await page.locator('#next').click();
      check(`${channel}: local next-page navigation preserves the anchor`, await page.title() === 'Second saved page' && page.url().endsWith('#chapter'));
      check(`${channel}: second page image also loads locally`, await page.locator('#picture').evaluate(e => e.complete && e.naturalWidth === 1));
      await page.locator('#back').click();
      check(`${channel}: return navigation uses the numbered starting filename`, path.resolve(fileURLToPath(page.url())) === path.resolve(main.path) && await page.title() === 'UDM saved website');
      check(`${channel}: browsing the saved site makes no HTTP requests`, webRequests.length === 0, { requests: webRequests });
    } finally { await browser.close(); }
  }
  check('Offline browsing leaves downloaded files unchanged', bytesAfterNative === JSON.stringify(result.files.map(f => [f.path, crypto.createHash('sha256').update(fs.readFileSync(f.path)).digest('hex')])));
  fs.writeFileSync(path.join(output, 'acceptance.json'), JSON.stringify({ executableSha256: crypto.createHash('sha256').update(fs.readFileSync(exe)).digest('hex'), checks, requests, scope: 'Isolated local HTTP site, native Grabber and real Chrome/Edge file browsing; not a public-site or native-dialog test.' }, null, 2));
  console.log(`${checks.length} offline website checks passed.`);
})().catch(error => {
  server.close();
  fs.writeFileSync(path.join(output, 'acceptance.json'), JSON.stringify({ checks, error: error.stack, requests }, null, 2));
  console.error(error.stack); process.exitCode = 1;
});
