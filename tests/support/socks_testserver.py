"""Loopback-only SOCKS4a/5 fixture in front of the independent FTP server."""
import select, socket, socketserver, threading

class SocksServer:
    def __init__(self, control_port, passive_ports):
        self.allowed = {control_port, *passive_ports}
        self.sockets = set()
        self.lock = threading.Lock()
        self.destinations = []
        self.errors = []
        owner = self
        class Handler(socketserver.BaseRequestHandler):
            def handle(self):
                client = self.request
                client.settimeout(10)
                with owner.lock:
                    owner.sockets.add(client)
                def read(n):
                    result = b''
                    while len(result) < n:
                        chunk = client.recv(n-len(result))
                        if not chunk:
                            raise EOFError()
                        result += chunk
                    return result
                def string():
                    value = b''
                    for _ in range(256):
                        b = read(1)
                        if b == b'\0':
                            return value
                        value += b
                    raise ValueError('Overlong SOCKS4 string')
                remote = None
                try:
                    version = read(1)[0]
                    if version == 4:
                        h = read(7)
                        assert h[0] == 1 and h[3:] == b'\0\0\0\1'
                        port = int.from_bytes(h[1:3], 'big')
                        assert string() == b''
                        host = string().decode('ascii')
                        reply = b'\0\x5a\0\0\x7f\0\0\1'
                    else:
                        assert version == 5 and read(read(1)[0]) == b'\0'
                        client.sendall(b'\5\0')
                        assert read(4) == b'\5\1\0\3'
                        host = read(read(1)[0]).decode('ascii')
                        port = int.from_bytes(read(2), 'big')
                        reply = b'\5\0\0\1\x7f\0\0\1\0\0'
                    assert host == 'udm-independent.invalid' and port in owner.allowed
                    with owner.lock:
                        owner.destinations.append((version, host, port))
                    remote = socket.create_connection(('127.0.0.1', port), timeout=10)
                    with owner.lock:
                        owner.sockets.add(remote)
                    client.sendall(reply)
                    while True:
                        ready, _, _ = select.select([client, remote], [], [], 10)
                        if not ready:
                            break
                        for source in ready:
                            data = source.recv(65536)
                            if not data:
                                return
                            (remote if source is client else client).sendall(data)
                except (OSError, EOFError):
                    pass  # cancellation closes either side mid-transfer
                except Exception as e:
                    with owner.lock:
                        owner.errors.append(str(e) or type(e).__name__)
                finally:
                    if remote:
                        remote.close()
                    with owner.lock:
                        owner.sockets.discard(client)
                        owner.sockets.discard(remote)
        class Server(socketserver.ThreadingTCPServer):
            daemon_threads = True
        self.server = Server(('127.0.0.1', 0), Handler)
        self.worker = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.worker.start()
    def settings(self, version):
        return {'address':f'127.0.0.1:{self.server.server_address[1]}',
                'mode':'Use a SOCKS4 / 4a proxy' if version == 4 else 'Use a SOCKS5 proxy'}
    def close(self):
        self.server.shutdown()
        with self.lock:
            active = list(self.sockets)
        for s in active:
            try:
                s.shutdown(socket.SHUT_RDWR)
            except OSError:
                pass
            s.close()
        self.server.server_close()
        self.worker.join(3)
