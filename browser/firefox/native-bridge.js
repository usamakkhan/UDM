/* UDM's versioned native connection. A lost acknowledgement never retries a download. */
(function(root){
 'use strict';
 function create(api,host){
  const channels=new Map();let playerLane=0,sequence=0,connections=0,completed=0,failures=0;
  const connectionError=(message,beforeSend=false)=>Object.assign(Error(message),{nativeConnectionLost:true,nativeBeforeSend:beforeSend});
  const next=()=>sequence=sequence>=2147483646?1:sequence+1;
  function channel(name){
   let state=channels.get(name);if(state)return state;
   state={port:null,ready:null,hello:null,pending:new Map(),legacy:false,negotiating:false};channels.set(name,state);return state;
  }
  function close(state,error){
   const port=state.port;state.port=null;state.ready=null;state.hello=null;state.negotiating=false;
   for(const entry of state.pending.values()){clearTimeout(entry.timer);entry.reject(error);}state.pending.clear();
   try{port?.disconnect();}catch{}
  }
  function exchange(state,port,message,timeout,connecting=false,beforeSend){
   if(state.port!==port)return Promise.reject(connectionError('UDM connection closed before the request was sent.',true));
   if(state.pending.size>=16)return Promise.reject(Error('UDM is busy. Wait for the pending downloads.'));
   beforeSend?.();
   const requestId=next();
   return new Promise((resolve,reject)=>{
    const timer=setTimeout(()=>{failures++;close(state,connectionError(connecting?'UDM did not finish connecting. No download request was sent.':'UDM did not acknowledge the request. Check its download list before trying again.'));},timeout);
    state.pending.set(requestId,{resolve,reject,timer});
    try{port.postMessage({...message,requestId});}catch(e){close(state,connectionError(e.message||'UDM connection closed.'));}
   });
  }
  async function connect(state){
   if(state.legacy||typeof api.runtime.connectNative!=='function')return false;
   if(state.ready)return state.ready;
   const port=api.runtime.connectNative(host);state.port=port;connections++;
   port.onMessage.addListener(reply=>{
    // A queued reply from a retired host must not negotiate or resolve requests
    // belonging to the replacement connection (including legacy-shaped hello).
    if(state.port!==port)return;
    // Old hosts do not echo IDs. Only the side-effect-free hello may fall back.
    if(!Number.isSafeInteger(reply?.requestId)){
     if(state.pending.size===1&&state.negotiating){const entry=[...state.pending.values()][0];clearTimeout(entry.timer);state.pending.clear();entry.resolve({ok:false,legacy:true});}
     return;
    }
    const entry=state.pending.get(reply.requestId);if(!entry)return;
    state.pending.delete(reply.requestId);clearTimeout(entry.timer);completed++;entry.resolve(reply);
   });
   port.onDisconnect.addListener(()=>{
    const detail=api.runtime.lastError?.message||port.error?.message;void detail;
    if(state.port!==port)return;
    failures++;
    const beforeSend=state.negotiating;
    close(state,connectionError(beforeSend?'UDM disconnected before the request was sent.':'UDM disconnected. Check its download list before retrying a download.',beforeSend));
   });
   state.negotiating=true;
   // Cold native-host launch can exceed five seconds under CPU contention.
   // Proceed as soon as hello arrives; never resend an admission to recover startup.
   const ready=exchange(state,port,{action:'hello',protocol:1},15000,true).then(reply=>{
    if(state.port!==port)throw connectionError('UDM connection changed during startup. No download request was sent.',true);
    state.negotiating=false;
    if(!reply?.ok||reply.protocol!==1){state.legacy=true;close(state,Error('UDM native protocol is unavailable.'));return false;}
    state.hello=reply;
    return port;
   },error=>{if(state.ready===ready){state.negotiating=false;state.ready=null;}throw error;});
   state.ready=ready;return ready;
  }
  async function warmControl(){
   // Negotiate the host protocol only. A later request owns dispatch and retry.
   await connect(channel('control'));
  }
  async function request(message,timeout=40000,beforeSend,validateSelection){
   if(!message||typeof message.action!=='string')throw Error('Invalid UDM request.');
   // Player retrieval can spend seconds on the network; it must not block Add URL.
   const lane=message.action==='youtube-player'?'player-'+(playerLane++%2):'control',state=channel(lane);
   // Reconnect once after a failure before dispatch. Only token-bound status
   // lookups may be repeated after dispatch.
   const statusRead=message.action==='capture-status',deadline=Math.max(1000,Math.min(timeout,statusRead?10000:45000));
   for(let attempt=0;;attempt++){
    try{
     const port=await connect(state);
     // Revalidate after startup and on any reconnect before exchange.
     if(validateSelection)await validateSelection(message);
     if(!port){beforeSend?.();return await api.runtime.sendNativeMessage(host,message);}
     // A plain hello reads static host metadata. Reuse only the reply from
     // this port, and give callers a copy so they cannot change the cache.
     if(state.port===port&&message.action==='hello'&&Object.keys(message).length===1&&state.hello)
      return JSON.parse(JSON.stringify(state.hello));
     return await exchange(state,port,message,deadline,false,beforeSend);
    }catch(error){if(attempt!==0||error?.nativeConnectionLost!==true||(!statusRead&&!error.nativeBeforeSend))throw error;}
   }
  }
  function disconnect(){for(const state of channels.values())close(state,Error('UDM connection closed.'));channels.clear();}
  return {request,warmControl,disconnect,diagnostics:()=>({connections,completed,failures,pending:[...channels.values()].reduce((n,s)=>n+s.pending.size,0),persistent:[...channels.values()].filter(s=>!!s.port).length})};
 }
 root.UdmNativeBridge={create};if(typeof module==='object'&&module.exports)module.exports=root.UdmNativeBridge;
})(globalThis);
