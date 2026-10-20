"""Real local TCP bootstrap/UDP traffic; no external servers."""
import ctypes as C,pathlib,socket,threading,time,struct,subprocess
root=pathlib.Path(__file__).resolve().parents[2]
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC',str(root/'code/platform/network_socket.c'),'-o',str(root/'code/platform/build/libnetwork_socket.so')],check=True)
lib=C.CDLL(str(root/'code/platform/build/libnetwork_socket.so'))
class Socket(C.Structure):_fields_=[('socket',C.c_size_t),('started',C.c_int),('connected',C.c_int)]
lib.sniper_network_socket_open.argtypes=[C.POINTER(Socket),C.c_char_p,C.c_uint16]
lib.sniper_network_socket_send.argtypes=[C.POINTER(Socket),C.c_void_p,C.c_int32]
lib.sniper_network_socket_read.argtypes=[C.POINTER(Socket),C.c_void_p,C.c_int32,C.c_int]
for fragmented in [False,True]:
    tcp=socket.socket();tcp.bind(('127.0.0.1',0));tcp.listen();tcp.settimeout(5)
    udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.bind(('127.0.0.1',0));udp.settimeout(5)
    advertised=[];errors=[]
    def peer():
        try:
            conn,_=tcp.accept();conn.settimeout(5);packet=b''
            while len(packet)<6:packet+=conn.recv(6-len(packet))
            advertised.append((socket.inet_ntoa(packet[:4]),struct.unpack('!H',packet[4:])[0]))
            response=socket.inet_aton('127.0.0.1')+struct.pack('!H',udp.getsockname()[1])
            if fragmented:
                conn.sendall(response[:3]);time.sleep(.03);conn.sendall(response[3:])
            else:conn.sendall(response)
            assert conn.recv(1)==b'';conn.close()
            data,addr=udp.recvfrom(512);assert data==b'\x01client';assert addr==advertised[0];udp.sendto(b'\x02server',addr)
        except BaseException as e:errors.append(e)
    t=threading.Thread(target=peer);t.start();s=Socket()
    assert lib.sniper_network_socket_open(C.byref(s),b'localhost' if fragmented else b'127.0.0.1',tcp.getsockname()[1])==1
    assert lib.sniper_network_socket_send(C.byref(s),b'\x01client',7)==7
    buf=C.create_string_buffer(512);limit=time.monotonic()+5
    while lib.sniper_network_socket_read(C.byref(s),buf,512,1)<0:
        assert time.monotonic()<limit;time.sleep(.001)
    assert buf.raw[:7]==b'\x02server';assert lib.sniper_network_socket_read(C.byref(s),buf,512,0)==7
    assert lib.sniper_network_socket_read(C.byref(s),buf,512,0)==-1
    lib.sniper_network_socket_close(C.byref(s));assert not s.connected and not s.started
    t.join();assert not errors,errors;tcp.close();udp.close()
# A bound but non-listening TCP endpoint refuses immediately.
blocked=socket.socket();blocked.bind(('127.0.0.1',0));s=Socket()
assert not lib.sniper_network_socket_open(C.byref(s),b'127.0.0.1',blocked.getsockname()[1]);assert not s.connected and not s.started;blocked.close()
print('PASS real loopback bootstrap: exact advertised IPv4/network-order port, partial TCP reply, DNS, connected UDP peek/read/reply, close and connection refusal')
