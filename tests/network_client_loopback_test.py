"""Controlled historical-wire peer, loopback only; no external credentials/server."""
import pathlib,socket,struct,threading,subprocess,time
root=pathlib.Path(__file__).resolve().parents[2]
exe=root/'code/build/network-client-loopback-test'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined',str(root/'code/tests/network_client_loopback_test.c'),*[str(root/'code/src'/f'network_{n}.c') for n in ['client','login','game_packets','game_receive','send','receive','transport','compression','lifecycle']],*[str(root/'code/src'/f'{n}.c') for n in ['network_backend','player_registry','ship_create','ship_reset','player_team','death_event','notices','chat_history','pickup_lifecycle','pickup_collect','attachment','movement','ship_effect','flight_math']],str(root/'code/platform/network_socket.c'),'-lm','-o',str(exe)],check=True)
tcp=socket.socket();tcp.bind(('127.0.0.1',0));tcp.listen();tcp.settimeout(15)
udp=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);udp.bind(('127.0.0.1',0));udp.settimeout(15)
errors=[];seen=[];bye=[]
def peer():
    try:
        conn,_=tcp.accept();conn.settimeout(15);hello=b''
        while len(hello)<6:hello+=conn.recv(6-len(hello))
        addr=(socket.inet_ntoa(hello[:4]),struct.unpack('!H',hello[4:])[0]);conn.sendall(socket.inet_aton('127.0.0.1')+struct.pack('!H',udp.getsockname()[1]));assert conn.recv(1)==b'';conn.close()
        sequence=1000;dedup=set()
        def reliable(body):
            nonlocal sequence
            udp.sendto(b'\0\2'+struct.pack('<IH',sequence,(sequence-1000)%256)+body,addr);sequence+=1
        while len(bye)<4:
            data,source=udp.recvfrom(4096);assert source==addr
            if data[:2]==b'\0\0':udp.sendto(b'\0\1'+data[2:6]+struct.pack('<I',int(time.monotonic()*100)%0x19999999),addr);continue
            if data[:2]==b'\0\3':continue
            if data==b'\0\4':bye.append(time.monotonic());continue
            if data[:2]==b'\0\2':
                seq=struct.unpack('<I',data[2:6])[0];udp.sendto(b'\0\3'+data[2:8],addr)
                if seq in dedup:continue
                dedup.add(seq);data=data[8:]
            op=data[0];seen.append(op)
            if op==9:
                assert len(data)==98 and data[1]==0 and data[2:10]==b'Pilot87\0' and data[34:51]==b'fixture-password\0'
                reliable(b'\x0a\0'+struct.pack('<iIII',87,0,0x68fc54a0,0))
            elif op==12:
                reliable(b'\x0d'+struct.pack('<i',20));reliable(b'\x0c'+struct.pack('<i',0)+b'controlled map bytes');reliable(b'\x0c'+struct.pack('<i',-1))
            elif op==1:
                assert data==b'\1\2'+struct.pack('<iiii',17,1,640,480)
                reliable(b'\1\7');join=bytearray(85);join[:3]=b'\3\7\2';join[3:11]=b'Pilot87\0';join[71:73]=struct.pack('<h',17);reliable(join)
                settings=bytearray(289);settings[0]=17;settings[-1]=0xa5;reliable(settings);reliable(b'\2')
            elif op==6:assert data==b'\6\xff\xffhello peer\0'
            elif op==16:assert data==b'\x10'+struct.pack('<i',22)
            elif op==17:assert data==b'\x11\xff\xff'
            elif op==7:assert data==b'\7'+struct.pack('<hhhB',9,10,3,7)
            elif op==15:assert data in (b'\x0f\xff\xff\2cache',b'\x0f\xff\xff\2')
            elif op==3:
                assert len(data)==19
                pos=bytearray(20);pos[0]=5;pos[10]=7;pos[11:15]=struct.pack('<I',int(time.monotonic()*100)%0x19999999);reliable(pos)
            else:raise AssertionError(('unknown opcode',op,data))
        assert bye[-1]-bye[0]>=.98
    except BaseException as e:errors.append(e)
t=threading.Thread(target=peer);t.start()
result=subprocess.run([str(exe),str(tcp.getsockname()[1])],capture_output=True,text=True,timeout=30)
t.join(timeout=16);tcp.close();udp.close();assert not t.is_alive();assert not errors,errors
assert result.returncode==0,(result.returncode,result.stdout,result.stderr)
assert seen==[9,12,1,6,16,17,7,15,15,3],seen
print(result.stdout.strip())
