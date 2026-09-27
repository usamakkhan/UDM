/* Shared site permission scopes. Embedded players and media hosts need their own grant. */
(function(root){
 'use strict';
 function origins(address){
  const u=new URL(address);if(!/^https?:$/.test(u.protocol))throw Error('Open a website first.');
  if(u.hostname==='dailymotion.com'||u.hostname.endsWith('.dailymotion.com'))
   return [...new Set([u.origin+'/*','https://*.dailymotion.com/*','https://*.dmcdn.net/*'])];
  return [u.origin+'/*'];
 }
 const value={origins};root.UdmSiteAccess=value;if(typeof module==='object'&&module.exports)module.exports=value;
})(globalThis);
