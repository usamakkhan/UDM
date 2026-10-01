/* Read-only Chromium routing metadata. Never changes browser proxy settings. */
(function(root){
 'use strict';
 const unsupported=()=>({unsupported:true});
 function ip(raw){
  try{
   const host=new URL('http://'+(raw.includes(':')&&!raw.startsWith('[')?'['+raw+']':raw)+'/').hostname;
   if(/^\d+\.\d+\.\d+\.\d+$/.test(host))return {bits:32,value:host.split('.').reduce((n,s)=>(n<<8n)|BigInt(s),0n)};
   if(!host.startsWith('['))return null;
   const parts=host.slice(1,-1).split('::'),left=parts[0]?parts[0].split(':'):[],right=parts[1]?parts[1].split(':'):[];
   const words=parts.length===2?[...left,...Array(8-left.length-right.length).fill('0'),...right]:left;
   return words.length===8?{bits:128,value:words.reduce((n,s)=>(n<<16n)|BigInt('0x'+s),0n)}:null;
  }catch{return null;}
 }
 function implicit(u){
  const host=u.hostname.toLowerCase(),plain=host.replace(/\.$/,'');
  if(plain==='localhost'||plain.endsWith('.localhost')||plain==='loopback')return true;
  const value=ip(host);if(!value)return false;
  return value.bits===32?(value.value>>24n)===127n||(value.value>>16n)===0xa9fen:
   value.value===1n||(value.value>>118n)===0x3fan;
 }
 function pattern(rule,u){
  if(typeof rule!=='string'||rule.length>2048||/[^\x21-\x7e]/.test(rule))return null;
  rule=rule.toLowerCase();
  if(rule==='<local>')return !u.hostname.includes('.')&&!ip(u.hostname);
  if(rule==='<-loopback>')return implicit(u)?'subtract':false;
  if(rule.includes('/')&&!rule.includes('://')){
   const m=/^([^/]+)\/(\d{1,3})$/.exec(rule);if(!m||m[1].includes('['))return null;
   const subnet=ip(m[1]),value=ip(u.hostname),bits=Number(m[2]);if(!subnet||bits>subnet.bits)return null;
   return !!value&&subnet.bits===value.bits&&(subnet.value>>BigInt(subnet.bits-bits))===(value.value>>BigInt(value.bits-bits));
  }
  const m=/^(?:([a-z][a-z0-9+.-]*):\/\/)?(\[[0-9a-f:.]+\]|[^/:]+)(?::(\d{1,5}))?$/.exec(rule);if(!m)return null;
  if(m[1]&&m[1]+':'!==u.protocol)return false;
  if(m[3]&&(Number(m[3])>65535||Number(m[3])<1))return null;
  if(m[3]&&Number(m[3])!==Number(u.port||(u.protocol==='https:'?443:80)))return false;
  const value=ip(m[2]),target=ip(u.hostname);if(value)return !!target&&value.bits===target.bits&&value.value===target.value;
  let host=m[2];if(host.startsWith('.'))host='*'+host;
  if(!/^[a-z0-9*_.-]+$/.test(host))return null;
  // Greedy wildcard matching avoids backtracking regular expressions on settings.
  const targetHost=u.hostname.toLowerCase();let at=0,position=0,star=-1,retry=0;
  while(at<targetHost.length){if(host[position]===targetHost[at]){position++;at++;}else if(host[position]==='*'){star=position++;retry=at;}else if(star>=0){position=star+1;at=++retry;}else return false;}
  while(host[position]==='*')position++;return position===host.length;
 }
 function route(config,address){
  try{
   const u=new URL(address);if(!['http:','https:'].includes(u.protocol)||u.username||u.password||u.href.length>16000)return unsupported();u.hash='';
   if(!config||typeof config!=='object')return unsupported();
   if(config.mode==='system')return undefined; // No browser-specific endpoint is exposed.
   if(config.mode==='direct')return {url:u.href,type:'direct'};
   if(config.mode!=='fixed_servers')return unsupported(); // PAC resolution/failover is browser-owned.
   const rules=config.rules;if(!rules||typeof rules!=='object')return unsupported();
   if(Object.keys(rules).some(k=>!['singleProxy','proxyForHttp','proxyForHttps','proxyForFtp','fallbackProxy','bypassList'].includes(k)))return unsupported();
   if(rules.singleProxy&&['proxyForHttp','proxyForHttps','proxyForFtp','fallbackProxy'].some(k=>rules[k]))return unsupported();
   const bypass=rules.bypassList||[];if(!Array.isArray(bypass)||bypass.length>1024)return unsupported();
   let direct=implicit(u);
   for(const rule of bypass){const match=pattern(rule,u);if(match===null)return unsupported();if(match==='subtract')direct=false;else if(match)direct=true;}
   if(direct)return {url:u.href,type:'direct'};
   const endpoint=rules.singleProxy||rules[u.protocol==='https:'?'proxyForHttps':'proxyForHttp']||rules.fallbackProxy;
   if(!endpoint)return {url:u.href,type:'direct'};
   if(typeof endpoint!=='object'||Object.keys(endpoint).some(k=>!['scheme','host','port'].includes(k)))return unsupported();
   const type=endpoint.scheme||'http',port=endpoint.port??(type==='http'?80:type==='https'?443:1080),raw=endpoint.host;
   // Chromium SOCKS4 resolves locally; SOCKS5 resolves at the proxy.
   if(!['http','https','socks4','socks5'].includes(type)||!Number.isInteger(port)||port<1||port>65535||typeof raw!=='string'||raw.length>255||/[^\x21-\x7e]|[/\\@?#;=]/.test(raw))return unsupported();
   const host=new URL('http://'+(raw.includes(':')&&!raw.startsWith('[')?'['+raw+']':raw)+':'+port+'/').hostname.replace(/^\[|\]$/g,'');
   if(!host)return unsupported();return {url:u.href,type,host,port,...(['socks4','socks5'].includes(type)?{proxyDNS:type==='socks5'}:{})};
  }catch{return unsupported();}
 }
 function create(api){
  if(typeof api.runtime?.getBrowserInfo==='function'||!api.proxy?.settings?.get)return null;
  let generation=0,installed=false;const states=new Map();
  function read(incognito=false){
   if(!states.has(incognito))states.set(incognito,{});const state=states.get(incognito);
   if(state.cached&&Date.now()<state.expiry)return Promise.resolve(state.cached);
   if(state.pending)return state.pending;
   const started=generation;
   const request=new Promise(resolve=>{
    let complete=false;const done=value=>{if(complete)return;complete=true;clearTimeout(timer);resolve(value);};
    const timer=setTimeout(()=>done({}),1000);
    try{api.proxy.settings.get({incognito},value=>done(api.runtime?.lastError?{}:value?.value||{}));}catch{done({});}
   }).then(value=>{if(started===generation){state.cached=value;state.expiry=Date.now()+30000;}return value;}).finally(()=>{if(state.pending===request)state.pending=null;});
   state.pending=request;return request;
  }
  function changed(){generation++;states.clear();void read();}
  // Proxy-error events have no request identity. An unrelated tab's failure
  // cannot change a successful request's fixed, single-endpoint mapping.
  function install(){if(installed)return;installed=true;api.proxy.settings.onChange?.addListener(changed);void read();}
  function observe(address,incognito=false){
   const observation={generation,pending:true,value:unsupported()};
   observation.ready=read(incognito===true).then(config=>{observation.value=route(config,address);observation.pending=false;});return observation;
  }
  function resolve(observation){return observation.generation===generation?observation.value:unsupported();}
  return {install,observe,resolve};
 }
 root.UdmChromiumProxy={route,create};if(typeof module==='object'&&module.exports)module.exports=root.UdmChromiumProxy;
})(globalThis);
