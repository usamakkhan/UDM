'use strict';
// Owned, isolated browser profile and loopback proxies; never changes the user's profile.
// Usage: node tests/pac-route.edge-native-live.cjs <fresh-output-dir> <Udm.TransportPoolProbe.exe>
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const assert = require('node:assert/strict');
const { promisify } = require('node:util');
const execFile = promisify(require('node:child_process').execFile);
const { chromium } = require('playwright');
assert(process.argv[2] && process.argv[3], 'Supply output directory and native probe');
const root = path.resolve(process.argv[2]);
const probe = path.resolve(process.argv[3]);
assert(!fs.existsSync(root), 'Output directory must be fresh');
assert(fs.existsSync(probe), 'Native probe must exist');
fs.mkdirSync(root, { recursive: true });
const servers = [], observations = [];
let context;
async function listen(handler, port = 0) {
  const server = http.createServer(handler);
  await new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(port, '127.0.0.1', resolve);
  });
  servers.push(server);
  return server;
}
async function close(server) {
  server.closeAllConnections();
  await new Promise(resolve => server.close(resolve));
}
function label(name) {
  return (request, response) => {
    response.writeHead(200, { 'Content-Type': 'text/plain', 'Cache-Control': 'no-store', 'Connection': 'close' });
    response.end(name);
  };
}
(async () => {
  // Reserve then release A: the first request must fail over to B.
  const reservation = await listen(label('A'));
  const portA = reservation.address().port;
  await close(reservation);
  const proxyB = await listen(label('B'));
  const script = `function FindProxyForURL(url, host) { return "PROXY 127.0.0.1:${portA}; PROXY 127.0.0.1:${proxyB.address().port}"; }`;
  const pac = await listen((request, response) => {
    response.writeHead(200, { 'Content-Type': 'application/x-ns-proxy-autoconfig' });
    response.end(script);
  });
  const pacUrl = `http://127.0.0.1:${pac.address().port}/proxy.pac`;
  const extension = path.join(root, 'extension');
  fs.mkdirSync(extension);
  fs.writeFileSync(path.join(extension, 'manifest.json'), JSON.stringify({
    manifest_version: 3, name: 'UDM isolated PAC comparison', version: '1.0',
    permissions: ['proxy'], background: { service_worker: 'background.js' }
  }));
  fs.writeFileSync(path.join(extension, 'background.js'), '// Isolated PAC fixture\n');
  context = await chromium.launchPersistentContext(path.join(root, 'profile'), {
    channel: process.env.UDM_TEST_BROWSER || 'msedge', headless: true,
    ignoreDefaultArgs: ['--disable-extensions'], args: ['--enable-unsafe-extension-debugging']
  });
  const cdp = await context.browser().newBrowserCDPSession();
  await cdp.send('Extensions.loadUnpacked', { path: extension });
  const worker = context.serviceWorkers()[0] || await context.waitForEvent('serviceworker');
  await worker.evaluate(url => new Promise((resolve, reject) => {
    chrome.proxy.settings.set({ value: { mode: 'pac_script', pacScript: { url, mandatory: true } }, scope: 'regular' }, () => {
      if (chrome.runtime.lastError) reject(Error(chrome.runtime.lastError.message)); else resolve();
    });
  }), pacUrl);
  const page = await context.newPage();
  // .invalid cannot escape to a public origin; both proxies return their own label.
  async function browserRoute(id) {
    await page.goto('http://udm-pac-fixture.invalid/' + id, { timeout: 30000 });
    return (await page.locator('body').innerText()).trim();
  }
  assert.equal(await browserRoute('first'), 'B', 'Chrome must fall back while A is offline');
  observations.push({ check: 'browser-fallback-while-a-offline', actual: 'B', passed: true });
  await listen(label('A'), portA);
  const browserAfterRecovery = await browserRoute('recovered');
  const config = path.join(root, 'native-config.json'), result = path.join(root, 'native-result.json');
  fs.writeFileSync(config, JSON.stringify({ cases: [{ id: 'native-recovered', url: 'http://udm-pac-fixture.invalid/recovered',
    settings: { ProxyMode: 'Use automatic configuration script', ProxyAutoConfigUrl: pacUrl, ProxyBypass: '' }, cancelAfterMs: 30000 }] }));
  await execFile(probe, [config, result], { timeout: 45000, windowsHide: true });
  const native = JSON.parse(fs.readFileSync(result, 'utf8')).cases[0];
  assert.equal(native.ok, true, native.error);
  const nativeAfterRecovery = fs.readFileSync(path.join(root, 'native-recovered.bin'), 'utf8');
  observations.push({ check: 'same-pac-after-a-recovers', browser: browserAfterRecovery, native: nativeAfterRecovery,
    routesMatch: browserAfterRecovery === nativeAfterRecovery });
  assert.equal(browserAfterRecovery, 'B', 'Chrome retains fallback ordering after A recovers');
  assert.equal(nativeAfterRecovery, 'A', 'Fresh native resolver uses the original PAC ordering');
  observations.push({ check: 'configuration-alone-does-not-preserve-browser-route', passed: true });
  console.log('PASS: Chrome stayed on B; UDM selected recovered A from the same PAC script.');
})().catch(error => {
  observations.push({ passed: false, error: error.message });
  console.error(error); process.exitCode = 1;
}).finally(async () => {
  await context?.close();
  for (const server of servers) if (server.listening) await close(server);
  fs.writeFileSync(path.join(root, 'results.json'), JSON.stringify({ observations }, null, 2));
});
