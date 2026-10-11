import datetime,hashlib,http.server,ipaddress,json,pathlib,ssl,subprocess,sys,threading
from cryptography import x509
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
root=pathlib.Path(sys.argv[1]);root.mkdir(exist_ok=False);probe=sys.argv[2];seen=[]
key=rsa.generate_private_key(public_exponent=65537,key_size=2048);name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'UDM isolated untrusted fixture')]);now=datetime.datetime.now(datetime.timezone.utc)
cert=x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-datetime.timedelta(minutes=1)).not_valid_after(now+datetime.timedelta(hours=1)).add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address('::ffff:127.0.0.1'))]),False).sign(key,hashes.SHA256())
(root/'key.pem').write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()));(root/'cert.pem').write_bytes(cert.public_bytes(serialization.Encoding.PEM))
class Handler(http.server.BaseHTTPRequestHandler):
 def do_GET(self):
  seen.append({'path':self.path,'host':self.headers.get('Host'),'proxyAuthorization':bool(self.headers.get('Proxy-Authorization'))});self.send_response(200);self.end_headers();self.wfile.write(b'mapped transport fixture')
 def log_message(self,*args):pass
plain=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);secure=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(root/'cert.pem',root/'key.pem');secure.socket=ctx.wrap_socket(secure.socket,server_side=True)
for server in (plain,secure):threading.Thread(target=server.serve_forever,daemon=True).start()
settings={'ProxyMode':'Connect directly','Proxy':'127.0.0.1:1','ProxyUser':'unrelated-user'}
cases=[{'id':'http-control','url':f'http://[::ffff:7f00:1]:{plain.server_port}/plain','settings':settings},{'id':'untrusted-https','url':f'https://[::ffff:7f00:1]:{secure.server_port}/secure','settings':settings}]
(root/'input.json').write_text(json.dumps({'cases':cases}))
try:
 subprocess.run([probe,str(root/'input.json'),str(root/'probe.json')],check=True)
 result=json.loads((root/'probe.json').read_text());a,b=result['cases'];assert a['ok'] and a['explicitProxyTransport'];assert a['finalUrl']==cases[0]['url'];assert a['sha256'].lower()==hashlib.sha256(b'mapped transport fixture').hexdigest();assert not b['ok'];assert any(word in b['error'].lower() for word in ['certificate','cert','trust','ssl']);assert len(seen)==1;assert seen[0]['host']==f'[::ffff:7f00:1]:{plain.server_port}';assert not seen[0]['proxyAuthorization'];result.update({'passedChecks':7,'traffic':seen,'trustStoreModified':False});(root/'results.json').write_text(json.dumps(result,indent=2));print('PASS 7 mapped transport identity and untrusted TLS checks')
finally:
 for server in (plain,secure):server.shutdown();server.server_close()
