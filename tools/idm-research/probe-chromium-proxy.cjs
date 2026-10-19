'use strict';
// Executes only the hash-pinned proxy-settings callback in an isolated JS context.
// Never loads the extension or contacts IDM. Proprietary source stays local.
const fs = require('node:fs');
const crypto = require('node:crypto');
const vm = require('node:vm');
const assert = require('node:assert/strict');
assert(process.argv[2] && process.argv[3], 'Supply reference background.js and output JSON');
const source = fs.readFileSync(process.argv[2], 'utf8');
const sha256 = crypto.createHash('sha256').update(source).digest('hex');
assert.equal(sha256, '3590408894c701937ffa51048c14c3e6d1b534e9616b76d99e4af523925294d7', 'Unknown specimen');
assert(!fs.existsSync(process.argv[3]), 'Output must be new');
const start = source.indexOf('n.bb=function(a)'), end = source.indexOf(';function Rb(', start);
assert(start >= 0 && end > start, 'Reference callback boundaries not found');
const context = vm.createContext({});
vm.runInContext('var n={},browser={runtime:{}},za={direct:2,auto_detect:3,system:4,pac_script:5,fixed_servers:7};', context);
vm.runInContext(source.slice(start, end) + ';', context, { timeout: 1000 });
const results = [];
for (const [name, config, expected] of [
  ['direct', { mode: 'direct' }, '2'],
  ['auto-detect', { mode: 'auto_detect' }, '3'],
  ['system', { mode: 'system' }, '4'],
  ['pac-url', { mode: 'pac_script', pacScript: { url: 'http://pac.invalid/config.pac' } }, '5:http://pac.invalid/config.pac'],
  ['pac-url-mandatory', { mode: 'pac_script', pacScript: { url: 'http://pac.invalid/config.pac', mandatory: true } }, '5:http://pac.invalid/config.pac'],
  ['inline-pac', { mode: 'pac_script', pacScript: { data: 'function FindProxyForURL(url,host){return "PROXY proxy.invalid:8080";}' } }, '6:function FindProxyForURL(url,host){return "PROXY proxy.invalid:8080";}'],
  ['oversized-inline', { mode: 'pac_script', pacScript: { data: 'x'.repeat(50001) } }, '4']
]) {
  context.config = config;
  const actual = vm.runInContext('var receiver={va:new Array(16)};n.bb.call(receiver,{value:config});receiver.J;', context, { timeout: 1000 });
  assert.equal(actual, expected, name);
  results.push({ name, serialized: actual, passed: true });
}
fs.writeFileSync(process.argv[3], JSON.stringify({ referenceSha256: sha256, checks: results.length, results }, null, 2));
console.log(`${results.length} reference callback checks passed`);
