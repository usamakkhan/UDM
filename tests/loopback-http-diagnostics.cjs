'use strict';
const headerNames=new Set(['content-length','content-type','content-range','transfer-encoding','connection','etag']);
function headers(value){
 const result={};
 for(const [name,data]of Object.entries(value||{}))if(headerNames.has(name.toLowerCase()))result[name.toLowerCase()]=String(data);
 return result;
}
function createTrace(base){
 const origin=new URL(base);if(origin.protocol!=='http:'||origin.hostname!=='127.0.0.1'||origin.username||origin.password)throw Error('HTTP diagnostics require the owned loopback origin');
 const requests=new Map(),events=[],excluded=new Set();
 return {
  request(event){let url;try{url=new URL(event.request.url);}catch{excluded.add(event.requestId);requests.delete(event.requestId);return;}
   // Redirects may reuse an ID. Exclude the whole chain if any hop leaves the
   // owned origin, including extra-info events that arrived before that hop.
   if(url.protocol!=='http:'||url.origin!==origin.origin||url.username||url.password){excluded.add(event.requestId);requests.delete(event.requestId);return;}
   if(excluded.has(event.requestId))return;
   const history=requests.get(event.requestId)||[];history.push({path:url.pathname,method:event.request.method});requests.set(event.requestId,history);
  },
  response(event){events.push({requestId:event.requestId,event:'response-headers',statusCode:event.statusCode,headers:headers(event.headers)});},
  failed(event){events.push({requestId:event.requestId,event:'failed',error:/^net::ERR_[A-Z0-9_]+$/.test(event.errorText||'')?event.errorText:'network request failed'});},
  finished(event){events.push({requestId:event.requestId,event:'finished',encodedDataLength:event.encodedDataLength});},
  result(){return {scope:'Owned loopback requests only; URL queries, credentials and unrelated headers omitted',requests:[...requests].map(([requestId,history])=>({requestId,history})),events:events.filter(e=>requests.has(e.requestId))};}
 };
}
module.exports={createTrace,headers};
