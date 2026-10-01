'use strict';
const {spawn}=require('node:child_process');
const path=require('node:path');
const assert=require('node:assert/strict');
// The desktop must already be running. This only sends a ping; it creates no downloads.
const child=spawn(process.env.UDM_HOST_EXE||path.join(__dirname,'../release/Udm.NativeHost.exe'),[],{windowsHide:true});
const body=Buffer.from(JSON.stringify({action:'ping'}));const prefix=Buffer.alloc(4);prefix.writeUInt32LE(body.length);
const chunks=[];child.stdout.on('data',c=>chunks.push(c));child.stderr.on('data',c=>process.stderr.write(c));
const timeout=setTimeout(()=>{child.kill();throw Error('Native host timed out');},15000);
child.on('error',e=>{clearTimeout(timeout);console.error(e);process.exitCode=1;});
child.on('exit',code=>{clearTimeout(timeout);try{assert.equal(code,0);const frame=Buffer.concat(chunks);assert.equal(frame.readUInt32LE(0),frame.length-4);assert.equal(JSON.parse(frame.subarray(4).toString()).ok,true);console.log('PASS real native host stdio framing and desktop pipe round trip');}catch(e){console.error(e);process.exitCode=1;}});
child.stdin.write(prefix.subarray(0,2));setTimeout(()=>{child.stdin.write(prefix.subarray(2));child.stdin.end(body);},20);
