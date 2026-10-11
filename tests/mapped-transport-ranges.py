"""Real native mapped-address byte ranges, shared sessions, HEAD and resume."""
import hashlib,http.server,json,pathlib,re,subprocess,sys,threading
root=pathlib.Path(sys.argv[1]);root.mkdir(exist_ok=False);probe=sys.argv[2]
payload=b"".join(hashlib.sha256(str(i).encode()).digest() for i in range(8192));seen=[];lock=threading.Lock()
class Handler(http.server.BaseHTTPRequestHandler):
 protocol_version='HTTP/1.1'
 def do_HEAD(self):self.respond(True)
 def do_GET(self):self.respond(False)
 def respond(self,head):
  value=self.headers.get('Range','');match=re.fullmatch(r'bytes=(\d+)-(\d*)',value);first=int(match[1]) if match else 0;last=int(match[2]) if match and match[2] else len(payload)-1
  with lock:seen.append({'method':self.command,'range':value,'host':self.headers.get('Host'),'proxyAuth':bool(self.headers.get('Proxy-Authorization')),'connection':self.client_address[1]})
  self.send_response(206 if match else 200);self.send_header('Content-Length',str(last-first+1));self.send_header('Accept-Ranges','bytes');self.send_header('ETag','"mapped-ranges"')
  if match:self.send_header('Content-Range',f'bytes {first}-{last}/{len(payload)}')
  self.end_headers()
  if not head:self.wfile.write(payload[first:last+1])
 def log_message(self,*args):pass
server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);threading.Thread(target=server.serve_forever,daemon=True).start()
url=f'http://[::ffff:7f00:1]:{server.server_port}/file';settings={'ProxyMode':'Connect directly'}
parts=[{'id':f'part-{i}','url':url,'settings':settings,'session':'pooled','start':i*65536,'end':(i+1)*65536-1} for i in range(4)]
config={'cases':[{'id':'head','url':url,'settings':settings,'head':True}], 'groups':[{'id':'parallel','parallel':True,'cases':parts},{'id':'resume','cases':[{'id':'tail','url':url,'settings':settings,'session':'pooled','start':131072}]}]}
(root/'input.json').write_text(json.dumps(config))
try:
 subprocess.run([probe,str(root/'input.json'),str(root/'probe.json')],check=True)
 result=json.loads((root/'probe.json').read_text());items={x['id']:x for x in result['cases']};checks=[]
 def check(value,name):
  assert value,name;checks.append({'name':name,'passed':True});print('PASS',name)
 check(all(x['ok'] and x['explicitProxyTransport'] for x in items.values()),'All native cases use the selected mapped-address transport')
 check(items['head']['status']==200 and items['head']['bytes']==0,'HEAD returns metadata without body bytes')
 for i in range(4):
  part=items[f'part-{i}'];check(part['status']==206 and part['contentRange']==f'bytes {i*65536}-{(i+1)*65536-1}/{len(payload)}',f'Parallel part {i} preserves exact response range')
 check(b''.join((root/f'part-{i}.bin').read_bytes() for i in range(4))==payload,'Four parallel ranges reconstruct the exact file')
 check((root/'tail.bin').read_bytes()==payload[131072:] and items['tail']['contentRange']==f'bytes 131072-{len(payload)-1}/{len(payload)}','Open-ended resume returns exactly the remaining bytes')
 check(all(x['host']==f'[::ffff:7f00:1]:{server.server_port}' and not x['proxyAuth'] for x in seen),'Every request retains mapped Host identity without proxy authentication')
 check(len(seen)==6 and sorted(x['range'] for x in seen)==sorted(['','bytes=131072-']+[f'bytes={i*65536}-{(i+1)*65536-1}' for i in range(4)]),'Server sees only the requested metadata and byte ranges')
 result.update({'checks':checks,'traffic':seen});(root/'results.json').write_text(json.dumps(result,indent=2))
finally:server.shutdown();server.server_close()
