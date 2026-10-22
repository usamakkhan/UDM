'use strict';
const fs = require('fs'), path = require('path'), http = require('http');
const { spawn } = require('child_process');
const crypto = require('crypto');
const {pathToFileURL,fileURLToPath}=require('url');
const exe = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
if (fs.existsSync(output)) throw Error('Use a new output directory.');
fs.mkdirSync(output, { recursive: true });
const requests = [], checks = [];
const routes = new Map([
  ['/docs/index.html', ['text/html', `<base href='/docs/'><script type='module' src='/redirect.js'></script><script type='module'>import './inline.js';</script><script src='/classic.js'></script><script>import('/classic-inline-lazy.js');</script>`]],
  ['/modules/main.js', ['application/javascript', `import {value} from './nested/dep.js'; export * from '../shared.js'; import 'bare-package'; import('./dynamic'+suffix); function load(){return import('./lazy.js');}`]],
  ['/modules/nested/dep.js', ['text/javascript', `import '../main.js'; export const value=1; image.src='module-asset.bin';`]],
  ['/modules/template.js', ['text/javascript', `export const value=67;`]],
  ['/modules/inside.js', ['text/javascript', `export const value=56;`]],
  ['/modules/lazy.js', ['text/javascript', `export const lazy=1;`]],
  ['/classic-lazy.js', ['text/javascript', `export const lazy=2;`]],
  ['/classic-inline-lazy.js', ['text/javascript', `export const lazy=3;`]],
  ['/shared.js', ['text/javascript', `export const shared=2;`]],
  ['/docs/inline.js', ['text/javascript', `export * as leaf from '../leaf.js';`]],
  ['/leaf.js', ['text/javascript', `export const leaf=3;`]],
  ['/classic.js', ['text/javascript', `function load(){return import('./classic-lazy.js');} import './classic-only.js';`]],
  ['/docs/module-asset.bin', ['application/octet-stream', 'Module-discovered asset bytes']],
  ['/limit.html', ['text/html', `<script type='module'>import '/chain/0.js';</script>`]],
]);
const templateEntry=routes.get('/modules/main.js');
// Valid dense JavaScript puts real dependencies beyond the former token cap.
templateEntry[1]='void 0;'.repeat(75000)+String.raw`consume('ordinary \u0061 text');`.repeat(16000)+templateEntry[1];
templateEntry[1]="const template = `import './template-fake.js' ${(()=>{const marker='}'; return `nested ${1}`;})()}`;\n"+templateEntry[1];
templateEntry[1]+="\nconst inside = `${import('./inside.js')}`; import(`./template.js`);";
const server = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://localhost').pathname;
  requests.push({ path: url, method: req.method });
  if (url === '/redirect.js') { res.writeHead(302, { Location: '/modules/main.js', 'Content-Length': 0 }); res.end(); return; }
  const chain = /^\/chain\/(\d+)\.js$/.exec(url);
  const entry = routes.get(url) || (chain ? ['text/javascript', `import './${Number(chain[1]) + 1}.js';`] : null);
  if (!entry) { res.writeHead(404, { 'Content-Length': 0 }); res.end(); return; }
  const body = Buffer.from(entry[1]);
  res.writeHead(200, { 'Content-Type': entry[0], 'Content-Length': body.length, Connection: 'close' });
  res.end(req.method === 'HEAD' ? undefined : body);
});
function check(name, passed) { checks.push({ name, passed: !!passed }); if (!passed) throw Error(name); }
async function run(origin, url, name) {
  const dir = path.join(output, name); fs.mkdirSync(dir);
  if(name === 'graph')for(const file of ['modules/nested/dep.js','docs/inline.js','modules/inside.js','modules/template.js']){
    const collision=path.join(dir,'downloads',file);fs.mkdirSync(path.dirname(collision),{recursive:true});fs.writeFileSync(collision,'Existing user file');
  }
  const input = path.join(dir, 'input.json'); fs.writeFileSync(input, JSON.stringify({ url: origin + url }));
  await new Promise((resolve, reject) => {
    const child = spawn(exe, ['--grabber-link-spec', input], { windowsHide: true });
    const log = fs.createWriteStream(path.join(dir, 'native.log')); child.stdout.pipe(log); child.stderr.pipe(log);
    child.once('error', reject); child.once('exit', code => code === 0 ? resolve() : reject(Error(`Native exit ${code}`)));
  });
  return JSON.parse(fs.readFileSync(path.join(dir, 'result.json'), 'utf8'));
}
(async () => {
  try {
    await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
    const origin = `http://127.0.0.1:${server.address().port}`;
    const result = await run(origin, '/docs/index.html', 'graph');
    check('Native publication completes with cyclic imports', result.project.LinkConversionState === 'Complete');
    for (const url of ['/modules/nested/dep.js', '/shared.js', '/docs/inline.js', '/leaf.js', '/docs/module-asset.bin','/modules/lazy.js','/classic-lazy.js','/classic-inline-lazy.js','/modules/inside.js','/modules/template.js']) {
      const file = result.files.find(f => f.url === origin + url);
      check(`${url}: discovered and published byte-exact`, file && fs.readFileSync(file.path).equals(Buffer.from(routes.get(url)[1])));
    }
    check('Imports use redirected module URL rather than document base', requests.some(r => r.path === '/modules/nested/dep.js') && requests.every(r => r.path !== '/docs/nested/dep.js'));
    check('Module asset assignment retains document-relative resolution', requests.every(r => r.path !== '/modules/nested/module-asset.bin'));
    check('Bare, computed and invalid classic static imports are not guessed', requests.every(r => !/bare-package|dynamic\.js|classic-only\.js|template-fake\.js/.test(r.path)));
    check('Cycles are bounded', requests.filter(r => r.path === '/modules/main.js' && r.method === 'GET').length <= 3);
    const main=result.files.find(f=>f.url===origin+'/modules/main.js');
    const imported=/from '([^']+)'/.exec(fs.readFileSync(main.path,'utf8'));
    const dependency=result.files.find(f=>f.url===origin+'/modules/nested/dep.js');
    check('External module import uses the actual renamed dependency', imported&&path.resolve(fileURLToPath(new URL(imported[1],pathToFileURL(main.path))))===path.resolve(dependency.path));
    const templateImport=/import\(`([^`]+)`\)/.exec(fs.readFileSync(main.path,'utf8'));
    const templateDependency=result.files.find(f=>f.url===origin+'/modules/template.js');
    check('Template import uses the actual renamed dependency', templateImport&&path.resolve(fileURLToPath(new URL(templateImport[1],pathToFileURL(main.path))))===path.resolve(templateDependency.path));
    const page=result.files.find(f=>f.url===origin+'/docs/index.html');
    const inlineImport=/import '([^']+)'/.exec(fs.readFileSync(page.path,'utf8'));
    const inlineDependency=result.files.find(f=>f.url===origin+'/docs/inline.js');
    check('Inline module import uses the actual renamed dependency', inlineImport&&path.resolve(fileURLToPath(new URL(inlineImport[1],pathToFileURL(page.path))))===path.resolve(inlineDependency.path));
    check('Existing colliding files remain unchanged', ['modules/nested/dep.js','docs/inline.js','modules/inside.js','modules/template.js'].every(file=>fs.readFileSync(path.join(output,'graph/downloads',file),'utf8')==='Existing user file'));
    check('Converted file hashes match the published catalog', result.files.every(f=>crypto.createHash('sha256').update(fs.readFileSync(f.path)).digest('hex')===f.sha256));
    const limit = await run(origin, '/limit.html', 'limit');
    check('Module traversal reports the script discovery limit', limit.project.Errors.some(e => e.Message.includes('Script discovery limit reached')));
    check('Module graph does not traverse beyond its 32-script budget', requests.every(r => r.path !== '/chain/33.js'));
    check('Budget exhaustion preserves completed downloads', limit.files.length > 1 && limit.files.every(f => f.status === 'Complete'));
    console.log(`${checks.length} module HTTP checks passed.`);
  } catch (error) { checks.push({ name: 'Fixture completion', passed: false, error: error.message }); console.error(error.message); process.exitCode = 1; }
  finally { server.close(); fs.writeFileSync(path.join(output, 'acceptance.json'), JSON.stringify({ checks, requests, scope: 'Static and literal dynamic module discovery, HTTP publication and saved import rewriting; no browser module execution or import maps.' }, null, 2)); }
})();
