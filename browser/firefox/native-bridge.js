/* UDM's versioned native connection. A lost acknowledgement never retries a download. */
(function(root){
 'use strict';
 function create(api,host){
  const channels=new Map();let playerLane=0,sequence=0,connections=0,completed=0,failures=0;
  const next=()=>sequence=sequence>=2147483646?1:sequence+1;
  function channel(name){
   let state=channels.get(name);if(state)return state;
   state={port:null,ready:null,pending:new Map(),legacy:false};channels.set(name,state);return state;
  }
  function close(state,error){
   const port=state.port;state.port=null;state.ready=null;
   for(const entry of state.pending.values()){clearTimeout(entry.timer);entry.reject(error);}state.pending.clear();
   try{port?.disconnect();}catch{}
  }
  function exchange(state,message,timeout){
   if(state.pending.size>=16)return Promise.reject(Error('UDM is busy. Wait for the pending downloads.'));
   const requestId=next();
   return new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>{failures++;close(state,Error('UDM did not acknowledge the request. Check its download list before trying again.'));},timeout);
    state.pending.set(requestId,{resolve,reject,timer});
    try{state.port.postMessage({...message,requestId});}catch(e){close(state,Error(e.message||'UDM connection closed.'));}
   });
  }
  async function connect(state){
   if(state.legacy||typeof api.runtime.connectNative!=='function')return false;
   if(state.ready)return state.ready;
   const port=api.runtime.connectNative(host);state.port=port;connections++;
   port.onMessage.addListener(reply=>{
    // Old hosts do not echo IDs. Only the side-effect-free hello may fall back.
    if(!Number.isSafeInteger(reply?.requestId)){
     if(state.pending.size===1&&state.negotiating){const entry=[...state.pending.values()][0];clearTimeout(entry.timer);state.pending.clear();entry.resolve({ok:false,legacy:true});}
     return;
    }
    const entry=state.pending.get(reply.requestId);if(!entry)return;
    state.pending.delete(reply.requestId);clearTimeout(entry.timer);completed++;entry.resolve(reply);
   });
   port.onDisconnect.addListener(()=>{const detail=api.runtime.lastError?.message||port.error?.message;void detail;if(state.port===port){failures++;close(state,Error('UDM disconnected. Check its download list before retrying a download.'));}});
   state.negotiating=true;
   state.ready=exchange(state,{action:'hello',protocol:1},5000).then(reply=>{
    state.negotiating=false;
    if(!reply?.ok||reply.protocol!==1){state.legacy=true;close(state,Error('UDM native protocol is unavailable.'));return false;}
    return true;
   },error=>{state.negotiating=false;state.ready=null;throw error;});
   return state.ready;
  }
  async function request(message,timeout=40000){
   if(!message||typeof message.action!=='string')throw Error('Invalid UDM request.');
   // Player retrieval can spend seconds on the network; it must not block Add URL.
   const lane=message.action==='youtube-player'?'player-'+(playerLane++%2):'control',state=channel(lane);
   if(await connect(state))return exchange(state,message,Math.max(1000,Math.min(timeout,45000)));
   return api.runtime.sendNativeMessage(host,message);
  }
  function disconnect(){for(const state of channels.values())close(state,Error('UDM connection closed.'));channels.clear();}
  return {request,disconnect,diagnostics:()=>({connections,completed,failures,pending:[...channels.values()].reduce((n,s)=>n+s.pending.size,0),persistent:[...channels.values()].filter(s=>!!s.port).length})};
 }
 root.UdmNativeBridge={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmNativeBridge;
})(globalThis);
