'use strict';
const net=require('node:net'),assert=require('node:assert/strict');
const payload=Buffer.alloc(8*1024*1024+713);for(let i=0;i<payload.length;i++)payload[i]=(i*19+Math.floor(i/65536)*7)%251;
function fixture(options={}){
 const o={epsv:true,eprt:true,rest:true,mdtm:true,size:true,body:payload,modified:'20260926000000',delay:3,...options};
 const ports=new Set(),sockets=new Set(),listeners=new Set(),metrics={commands:[],offsets:[],active:0,maxActive:0,bytes:0,retr:0,logins:0},timers=new Set();let drop=false;
 const track=s=>{sockets.add(s);s.on('error',()=>{});s.on('close',()=>sockets.delete(s));};
 const server=net.createServer(s=>{track(s);s.setEncoding('utf8');let buffer='',data=null,passive=null,active=null,offset=0,loginUser="",authenticated=!o.auth;
  const reply=(code,message)=>{if(!s.destroyed)s.write(`${code} ${message}\r\n`);};
  s.on('close',()=>{if(data)data.destroy();if(passive)passive.close();});
  if(!o.stallGreeting)s.write(o.multiline?'220-UDM fixture\r\nserver details\r\n220 Ready\r\n':'220 UDM fixture\r\n');
  s.on('data',chunk=>{buffer+=chunk;if(buffer.length>16384)return s.destroy();while(buffer.includes('\r\n')){const n=buffer.indexOf('\r\n'),line=buffer.slice(0,n);buffer=buffer.slice(n+2);const at=line.indexOf(' '),cmd=(at<0?line:line.slice(0,at)).toUpperCase(),arg=at<0?'':line.slice(at+1);metrics.commands.push(cmd);
   if(cmd==='USER'){loginUser=arg;metrics.logins++;reply(331,'Password required');}
   else if(cmd==='PASS'){authenticated=!o.auth||(arg==='fixture-password'&&(!o.authUser||loginUser===o.authUser));reply(authenticated?230:530,'Login result');}
   else if(!authenticated)reply(530,'Not logged in');
   else if(cmd==='TYPE')reply(200,'Binary');
   else if(cmd==='SIZE')reply(o.size?213:502,o.size?String(o.declaredSize??o.body.length):'Unsupported');
   else if(cmd==='MDTM')reply(o.mdtm?213:502,o.mdtm?o.modified:'Unsupported');
   else if(cmd==='REST'){if(!o.rest||(o.rejectNonzero&&Number(arg)>0))reply(502,'Restart unsupported');else {offset=Number(arg);reply(350,'Restart accepted');}}
   else if(cmd==='EPSV'||cmd==='PASV'){
    if(cmd==='EPSV'&&!o.epsv){reply(502,'EPSV unsupported');continue;}
    if(o.badPort){reply(cmd==='EPSV'?229:227,cmd==='EPSV'?'Entering (|||22|)':'Entering (127,0,0,1,0,22)');continue;}
    if(o.malformedPasv){reply(227,'Entering (999,0,0,1,100,1)');continue;}
    if(passive)passive.close();active=null;passive=net.createServer(d=>{track(d);data=d;});listeners.add(passive);passive.listen(0,'127.0.0.1',()=>{if(s.destroyed)return;const port=passive.address().port;ports.add(port);reply(cmd==='EPSV'?229:227,cmd==='EPSV'?`Entering (|||${port}|)`:`Entering (${o.foreignPasv?'203,0,113,7':'127,0,0,1'},${port>>8},${port&255})`);});
   }else if(cmd==='EPRT'||cmd==='PORT'){
    if(cmd==='EPRT'&&!o.eprt){reply(502,'EPRT unsupported');continue;}
    const v=arg.split(cmd==='EPRT'?'|':',');const host=cmd==='EPRT'?v[2]:v.slice(0,4).join('.'),port=cmd==='EPRT'?Number(v[3]):Number(v[4])*256+Number(v[5]);assert.equal(host,'127.0.0.1');active={host,port};reply(200,'Active accepted');
   }else if(cmd==='RETR'){
    metrics.retr++;metrics.offsets.push(offset);if(o.rejectRetr){reply(550,'Cannot read');continue;}reply(150,'Data follows');let begin=offset;offset=0;
    const write=d=>{metrics.active++;metrics.maxActive=Math.max(metrics.maxActive,metrics.active);let ended=false;const finish=()=>{if(!ended){ended=true;metrics.active--;}};d.once('close',finish);
     const truncate=o.dropOnce&&!drop;if(truncate)drop=true;let sent=0;
     const pump=()=>{if(d.destroyed||s.destroyed){finish();return;}if(o.stallData)return;
      if(truncate&&sent>=196608){d.destroy();s.destroy();finish();return;}
      if(begin>=o.body.length){if(o.mutateAfterData)o.modified='20260927000000';d.end(()=>{finish();reply(o.failCompletion?451:226,'Transfer finished');});return;}
      const b=o.body.subarray(begin,Math.min(o.body.length,begin+32768));begin+=b.length;sent+=b.length;metrics.bytes+=b.length;
      const schedule=()=>{const timer=setTimeout(()=>{timers.delete(timer);pump();},o.delay);timers.add(timer);};if(d.write(b))schedule();else d.once('drain',schedule);
     };pump();
    };
    if(active){data=net.connect(active);track(data);data.once('connect',()=>write(data));}else if(data)write(data);else {const timer=setInterval(()=>{if(data){clearInterval(timer);timers.delete(timer);write(data);}},5);timers.add(timer);s.once('close',()=>{clearInterval(timer);timers.delete(timer);});}
   }else reply(502,'Unsupported');
  }});
 });
 return {o,metrics,ports,async start(){await new Promise(r=>server.listen(0,'127.0.0.1',r));this.url=`ftp://127.0.0.1:${server.address().port}/fixture.bin`;return this;},close(){for(const t of timers)clearTimeout(t);for(const s of sockets)s.destroy();for(const l of listeners)l.close();server.close();}};
}
module.exports={payload,fixture};
