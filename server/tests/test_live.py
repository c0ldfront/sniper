"""Actual server process/socket tests. Faults affect only these loopback sockets."""
import contextlib
import json
from pathlib import Path
import select
import socket
import subprocess
import sys
import tempfile
import time
import unittest
from server import ROOT, Client, encode, decode
from socket_session import bootstrap_client
from transport import encode_control


class FaultSocket:
    def __init__(self,sock):
        self.sock=sock; self.drop_reply=1; self.drop_ack=1
        self.dropped_reply=0;self.dropped_ack=0; self.reorder=False;self.held=[]
    def __getattr__(self,name): return getattr(self.sock,name)
    def recv(self,count):
        data=self.sock.recv(count)
        if self.drop_reply and data[:2]==b'\0\2':
            self.drop_reply-=1;self.dropped_reply+=1
            raise socket.timeout()
        return data
    def send(self,data):
        if self.drop_ack and data[:2]==b'\0\3':
            self.drop_ack-=1;self.dropped_ack+=1
            return len(data)
        if self.reorder and data[:2]==b'\0\2':
            self.held.append(data)
            if len(self.held)==2:
                for item in (self.held[1],self.held[1],self.held[0]): self.sock.send(item)
                self.held=[];self.reorder=False
            return len(data)
        return self.sock.send(data)


class LiveTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.data=Path(self.temp.name)
        with socket.socket() as s:
            s.bind(('127.0.0.1',0));self.port=s.getsockname()[1]
        self.clients=[]
        self.start()
    def start(self,idle_timeout=10):
        self.proc=subprocess.Popen([sys.executable,'-u',str(ROOT/'server.py'),'--port',str(self.port),'--data',str(self.data),'--idle-timeout',str(idle_timeout)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        if not select.select([self.proc.stdout],[],[],5)[0]: self.fail('server did not start')
        self.assertIn('listening',self.proc.stdout.readline())
    def stop(self):
        self.proc.terminate()
        stdout,stderr=self.proc.communicate(timeout=8)
        self.assertEqual(self.proc.returncode,0,stderr)
        self.assertNotIn('Traceback',stderr)
    def tearDown(self):
        for c in self.clients:
            with contextlib.suppress(OSError): c.close()
        if self.proc.poll() is None: self.stop()
        self.temp.cleanup()
    def connection(self):
        c=bootstrap_client('127.0.0.1',self.port);self.clients.append(c);return c
    def receive(self,c,opcode,timeout=8,others=()):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            for peer in others: peer.receive(timeout=0.001)
            for payload in c.receive(timeout=0.02):
                p=decode('s2c',payload)
                if p['opcode']==opcode:return p['fields']
        self.fail(f'timed out waiting for opcode {opcode}')
    def login(self,c,name,create=1,password=b'pw'):
        c.send(encode('c2s',9,create_user=create,name=name,password=password,third_string=b''))
        return self.receive(c,10)['status']
    def join(self,c,others=()):
        c.send(encode('c2s',1,ship_type=0,team=0,option=1,screen_width=640,screen_height=480))
        self.receive(c,2,others=others)
    def pump(self,clients,seconds):
        end=time.monotonic()+seconds
        out=[]
        while time.monotonic()<end:
            for c in clients:
                out.extend((c,decode('s2c',p)) for p in c.receive(timeout=0.01))
        return out
    def test_seven_players_sustained_state_fanout(self):
        peers=[]
        for i in range(7):
            c=self.connection()
            self.assertEqual(self.login(c,f'load{i}'.encode()),0)
            self.join(c,others=peers)
            peers.append(c)
        self.pump(peers,0.5)
        counts=[0]*7
        end=time.monotonic()+25
        next_send=time.monotonic()
        while time.monotonic()<end:
            now=time.monotonic()
            if now>=next_send:
                for c in peers:
                    c.send(encode('c2s',3,state_byte=0,word2=100,word4=200,
                                  word6=0,word8=0,tick=1234,extra_byte=0,
                                  extra_word=0,flags=0),reliable=False)
                next_send=now+0.05
            for i,c in enumerate(peers):
                for payload in c.receive(timeout=0.001):
                    if decode('s2c',payload)['opcode']==5:counts[i]+=1
        self.assertTrue(all(n>2200 for n in counts),counts)

    def test_lost_login_reply_ack_reordered_duplicate_chat(self):
        a=self.connection();fault=FaultSocket(a.socket);a.socket=fault
        self.assertEqual(self.login(a,b'a'),0)
        self.pump([a],2)
        self.assertEqual((fault.dropped_reply,fault.dropped_ack),(1,1))
        self.join(a)
        b=self.connection();self.assertEqual(self.login(b,b'b'),0);self.join(b,others=[a])
        self.pump([a,b],0.3)
        fault.reorder=True
        for text in (b'first',b'second'):
            a.send(encode('c2s',6,target=-1,text=text))
        received=self.pump([a,b],2)
        self.assertEqual([p['fields']['text'] for c,p in received if c is b and p['opcode']==7],[b'first',b'second'])
        self.assertEqual([p for c,p in received if c is a and p['opcode']==7],[])
        self.assertFalse(fault.reorder)
    def test_restart_persists_accounts_squad_and_reconnect(self):
        a=self.connection();self.assertEqual(self.login(a,b'persistent'),0)
        a.send(encode('c2s',19,name=b'crew',password=b'squadpw'))
        self.assertEqual(self.receive(a,19)['status'],0)
        self.stop();a.close();self.start()
        b=self.connection();self.assertEqual(self.login(b,b'persistent',create=0),0)
        self.join(b)
        b.send(encode('c2s',10))
        record=self.receive(b,11)['records'][0]
        self.assertEqual((record['string1'],record['string2']),(b'persistent',b'crew'))
        b.send(encode('c2s',2));self.pump([b],0.2)
        self.join(b)
    def test_malformed_peer_does_not_kill_healthy_peer(self):
        a=self.connection();self.assertEqual(self.login(a,b'healthy'),0);self.join(a)
        bad=self.connection();bad.socket.send(b'\x09bad')
        self.pump([a],0.3)
        a.send(encode('c2s',13))
        self.assertGreater(self.receive(a,13)['total_size'],0)
        self.assertIsNone(self.proc.poll())
        events=[json.loads(l) for l in (self.data/'events.jsonl').read_text().splitlines()]
        self.assertTrue(any(e['event']=='session_error' and e['kind']=='ValueError' for e in events))
    def test_idle_cleanup_but_ping_keeps_session_alive(self):
        self.stop();self.start(idle_timeout=0.6)
        a=self.connection();self.assertEqual(self.login(a,b'idle'),0)
        for _ in range(8):
            a.probe_clock();self.pump([a],0.15)
        events=[json.loads(l) for l in (self.data/'events.jsonl').read_text().splitlines()]
        self.assertFalse(any(e['event']=='idle_timeout' for e in events))
        time.sleep(0.8)
        events=[json.loads(l) for l in (self.data/'events.jsonl').read_text().splitlines()]
        self.assertTrue(any(e['event']=='idle_timeout' for e in events))
        b=self.connection();self.assertEqual(self.login(b,b'idle',create=0),0)

    def test_transport_close_releases_login(self):
        a=self.connection();self.assertEqual(self.login(a,b'reconnect'),0);self.join(a)
        a.socket.send(encode_control(4));a.close();time.sleep(0.1)
        b=self.connection();self.assertEqual(self.login(b,b'reconnect',create=0),0);self.join(b)


if __name__=='__main__': unittest.main()
