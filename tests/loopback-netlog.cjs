'use strict';
const {headers}=require('./loopback-http-diagnostics.cjs');
// Never export raw NetLog params: they can include payloads, cookies and URLs
// from background browser traffic, even with a fresh isolated profile.
function summarizeNetlog(log,base){
 const origin=new URL(base);
 if(origin.protocol!=='http:'||origin.hostname!=='127.0.0.1'||origin.username||origin.password)throw Error('NetLog summary requires owned loopback HTTP');
 if(!Array.isArray(log.events)||!log.constants?.logEventTypes||!log.constants?.logSourceType)throw Error('Unsupported NetLog format');
 const names=new Map(Object.entries(log.constants.logEventTypes).map(([k,v])=>[v,k]));
 const requestType=log.constants.logSourceType.URL_REQUEST;
 if(!Number.isInteger(requestType))throw Error('Missing URL_REQUEST source type');
 const requests=new Map(),excluded=new Set();
 for(const e of log.events){
  if(e.source?.type!==requestType||!Number.isSafeInteger(e.source.id)||typeof e.params?.url!=='string')continue;
  const id=e.source.id;let url;
  try{url=new URL(e.params.url);}catch{excluded.add(id);continue;}
  if(url.protocol!=='http:'||url.origin!==origin.origin||url.username||url.password){excluded.add(id);continue;}
  if(!requests.has(id))requests.set(id,{sourceId:id,paths:[],bodyBytes:0,events:[]});
  const row=requests.get(id);if(row.paths.at(-1)!==url.pathname)row.paths.push(url.pathname);
 }
 const eventNames=new Set(['HTTP_STREAM_PARSER_READ_HEADERS','HTTP_TRANSACTION_READ_HEADERS','HTTP_TRANSACTION_READ_RESPONSE_HEADERS','URL_REQUEST_START_JOB','REQUEST_ALIVE']);
 for(const e of log.events){
  if(e.source?.type!==requestType||excluded.has(e.source.id))continue;
  const row=requests.get(e.source.id);if(!row)continue;
  const name=names.get(e.type),p=e.params||{};
  if(name==='URL_REQUEST_JOB_FILTERED_BYTES_READ'&&Number.isSafeInteger(p.byte_count)&&p.byte_count>=0)row.bodyBytes+=p.byte_count;
  if(!eventNames.has(name))continue;
  const event={name};if(Number.isInteger(e.phase)&&e.phase>=0&&e.phase<=2)event.phase=e.phase;
  if(Number.isInteger(p.net_error)&&p.net_error<=0&&p.net_error>=-999)event.netError=p.net_error;
  if(name==='HTTP_TRANSACTION_READ_RESPONSE_HEADERS'&&Array.isArray(p.headers)){
   const values={};
   for(const line of p.headers){
    if(typeof line!=='string')continue;
    if(/^HTTP\/\d(?:\.\d)? [1-5]\d\d(?: |$)/.test(line))event.status=Number(line.match(/ ([1-5]\d\d)/)[1]);
    const colon=line.indexOf(':');if(colon>0)values[line.slice(0,colon).toLowerCase()]=line.slice(colon+1).trim();
   }
   event.headers=headers(values);
  }
  row.events.push(event);
 }
 return {scope:'Owned loopback URL_REQUEST sources only; query, raw payload, cookies, credentials, unrelated traffic and dependency graphs omitted',requests:[...requests.values()].filter(r=>!excluded.has(r.sourceId))};
}
module.exports={summarizeNetlog};
