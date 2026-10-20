"""Server policy/packet contract tests; original-client UI evidence is separate."""
import json
from pathlib import Path
import tempfile
import unittest
from server import Arena, Client, Accounts, checksum, encode, decode, pickup_pool
from unittest.mock import Mock
import argparse


class ServerTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.arena=Arena(Path(self.temp.name),38383)

    def tearDown(self):
        self.arena.events.close()
        self.arena.accounts.db.close()
        self.temp.cleanup()

    def packets(self,c):
        result=[]
        while not c.outbox.empty(): result.append(decode('s2c',c.outbox.get_nowait()[0]))
        return result

    def send(self,c,op,**fields):
        self.arena.handle(c,encode('c2s',op,**fields))

    def client(self,name=b'pilot',team=0):
        c=Client(None);self.arena.clients.append(c)
        self.send(c,9,create_user=1,name=name,password=b'test',third_string=b'')
        self.assertEqual(self.packets(c)[0]['fields']['status'],0)
        self.send(c,1,ship_type=0,team=team,option=1,screen_width=640,screen_height=480)
        return c

    def test_auth_creation_rejection_and_restart(self):
        c=Client(None);self.arena.clients.append(c)
        for create,password,status in [(0,b'test',1),(1,b'test',0),(0,b'wrong',2),(0,b'test',0)]:
            self.send(c,9,create_user=create,name=b'alice',password=password,third_string=b'')
            self.assertEqual(self.packets(c)[0]['fields']['status'],status)
        other=Accounts(Path(self.temp.name)/'accounts.sqlite3')
        try: self.assertEqual(other.login(b'alice',b'test',False),0)
        finally: other.db.close()

    def test_initialization_order_and_no_duplicate_announcements(self):
        a=self.client(b'a');packets=self.packets(a)
        self.assertEqual([p['opcode'] for p in packets],[1,17,3,2])
        self.assertEqual(len(packets[1]['fields']['opaque']),288)
        b=self.client(b'b'); packets=self.packets(b)
        self.assertEqual([p['opcode'] for p in packets],[1,17,3,3,2])
        self.assertEqual([p['fields']['player'] for p in packets if p['opcode']==3],[a.player,b.player])
        self.assertEqual([p['opcode'] for p in self.packets(a)],[3])
        self.send(b,2)
        self.assertEqual(self.packets(a)[0]['fields']['player'],2)

    def test_private_team_public_chat_no_sender_echo(self):
        a=self.client(b'a',1); b=self.client(b'b',1); c=self.client(b'c',2)
        for p in (a,b,c): self.packets(p)
        for target,expected in [(-1,(1,1)),(-2,(1,0)),(c.player,(0,1))]:
            self.send(a,6,target=target,text=b'hello')
            self.assertEqual(self.packets(a),[])
            self.assertEqual((len(self.packets(b)),len(self.packets(c))),expected)

    def test_state_preserves_weapon_bits_excludes_sender(self):
        a=self.client(b'a'); b=self.client(b'b')
        self.packets(a);self.packets(b)
        self.send(a,3,state_byte=30,word2=100,word4=200,word6=-300,word8=400,tick=1234,extra_byte=128,extra_word=10,flags=0x801b)
        self.assertEqual(self.packets(a),[])
        packet=self.packets(b)[0]
        self.assertEqual(packet['opcode'],5)
        self.assertEqual(packet['fields']['player'],a.player)
        self.assertEqual(packet['fields']['flags'],0x801b)
        self.assertEqual(packet['fields']['extra_byte'],-128)

    def test_death_identity_and_session_score(self):
        killer=self.client(b'killer');victim=self.client(b'victim')
        self.packets(killer);self.packets(victim)
        self.send(victim,5,other_player=killer.player,value=12)
        fields=self.packets(killer)[0]['fields']
        self.assertEqual((fields['player'],fields['other_player'],fields['value']),(killer.player,victim.player,12))
        self.assertEqual(self.arena.accounts.profile(killer.name)[:3],(1,0,12))
        self.assertEqual(self.arena.accounts.profile(victim.name)[:3],(0,1,0))
        self.send(victim,5,other_player=killer.player,value=12)
        self.assertEqual(self.packets(killer),[])

    def test_reconnect_resets_scores_but_ship_change_and_failed_login_do_not(self):
        a=self.client(b'a');b=self.client(b'b')
        self.arena.accounts.squad(a.name,b'crew',b'pw')
        self.send(b,5,other_player=a.player,value=12)
        self.packets(a);self.packets(b)
        # Joining another ship on the same authenticated connection keeps score.
        self.send(a,1,ship_type=1,team=0,option=1,screen_width=640,screen_height=480)
        self.assertEqual(self.arena.accounts.profile(b'a')[:3],(1,0,12))
        outsider=Client(None);self.arena.clients.append(outsider)
        for password in (b'wrong',b'test'):
            self.send(outsider,9,create_user=0,name=b'a',password=password,third_string=b'')
            self.assertNotEqual(self.packets(outsider)[0]['fields']['status'],0)
            self.assertEqual(self.arena.accounts.profile(b'a')[:3],(1,0,12))
        self.send(a,2);a.alive=False
        self.send(outsider,9,create_user=0,name=b'a',password=b'test',third_string=b'')
        self.assertEqual(self.packets(outsider)[0]['fields']['status'],0)
        self.assertEqual(self.arena.accounts.profile(b'a'),(0,0,0,b'crew'))
        self.assertEqual(self.arena.accounts.profile(b'b')[:3],(0,1,0))
        self.packets(b)
        self.send(outsider,1,ship_type=0,team=0,option=1,screen_width=640,screen_height=480)
        row=next(p['fields'] for p in self.packets(b) if p['opcode']==3 and p['fields']['name'].split(b'\0',1)[0]==b'a')
        self.assertEqual((row['field47'],row['field49'],row['field4b']),(0,0,0))
        self.send(outsider,10)
        rows=next(p['fields']['records'] for p in self.packets(outsider) if p['opcode']==11)
        row=next(r for r in rows if r['string1']==b'a')
        self.assertEqual((row['value1'],row['value2'],row['value3']),(0,0,0))

    def test_voice_upload_repeat_and_missing_slot(self):
        a=self.client(b'a'); b=self.client(b'b');self.packets(a);self.packets(b)
        for audio in (b'opaque-audio',b''):
            self.send(a,15,target=b.player,slot=0,payload=audio)
            self.assertEqual(self.packets(a),[])
            self.assertEqual(self.packets(b)[0]['fields']['payload'],b'opaque-audio')
        with self.assertRaises(ValueError): self.send(a,15,target=b.player,slot=1,payload=b'')

    def test_invalid_killer_does_not_suppress_valid_death(self):
        a=self.client(b'a');b=self.client(b'b')
        self.packets(a);self.packets(b)
        self.send(b,5,other_player=250,value=7)
        self.send(b,5,other_player=a.player,value=7)
        self.assertEqual(self.arena.accounts.profile(a.name)[:3],(1,0,7))
        self.assertEqual(self.arena.accounts.profile(b.name)[:3],(0,1,0))

    def test_score_boundaries_and_self_kill(self):
        a=self.client(b'a')
        self.arena.accounts.death(a.name,a.name,0)
        self.assertEqual(self.arena.accounts.profile(a.name)[:3],(0,1,0))
        self.arena.accounts.death(a.name,a.name,2147483647)
        self.arena.accounts.death(a.name,a.name,100)
        self.assertEqual(self.arena.accounts.profile(a.name)[:3],(0,3,2147483647))
        self.packets(a)
        self.send(a,10)
        self.assertEqual(self.packets(a)[0]['fields']['records'][0]['value3'],2147483647)

    def test_squad_create_join_password_and_persistence(self):
        a=self.client(b'a');b=self.client(b'b');self.packets(a);self.packets(b)
        for c,password,status in [(a,b'pw',0),(b,b'wrong',2),(b,b'pw',1)]:
            self.send(c,19,name=b'group',password=password)
            self.assertEqual(self.packets(c)[0]['fields']['status'],status)
        self.assertEqual(self.arena.accounts.profile(b.name)[3],b'group')

    def test_pickup_identity_and_no_double_grant(self):
        a=self.client(b'a');b=self.client(b'b');self.packets(a);self.packets(b)
        pos=next((x,y) for x in range(500,520) for y in range(500,520) if not self.arena.tiles.get((x,y),0))
        self.assertTrue(self.arena.spawn_pickup(*pos,1))
        self.packets(a);self.packets(b)
        self.send(a,7,x=pos[0],y=pos[1],value=1,player=b.player)
        self.assertEqual(self.packets(a),[])
        self.assertEqual(self.packets(b)[0]['fields']['player'],a.player)
        self.send(a,7,x=pos[0],y=pos[1],value=1,player=b.player)
        self.assertEqual(self.packets(b),[])

    def test_attachment_cycle_prevented_and_leave_detaches(self):
        a=self.client(b'a');b=self.client(b'b');self.packets(a);self.packets(b)
        self.send(a,17,target=b.player)
        self.assertEqual(a.attached,b.player)
        self.send(b,17,target=a.player)
        self.assertEqual(b.attached,-1)
        self.send(b,2)
        self.assertEqual(a.attached,-1)

    def test_download_exact_bytes_and_cancel(self):
        a=self.client();self.packets(a)
        self.send(a,12)
        packets=self.packets(a)
        self.assertEqual(packets[0]['fields']['total_size'],len(self.arena.map))
        chunks=[p['fields'] for p in packets if p['opcode']==12]
        self.assertEqual(chunks[-1]['offset'],0xffffffff)
        self.assertEqual(b''.join(c['data'] for c in chunks),self.arena.map)
        self.send(a,12);self.send(a,14)
        self.assertEqual(self.packets(a),[])

    def test_attached_player_rejoins_as_fresh_session(self):
        a=self.client(b'a');b=self.client(b'b')
        self.send(a,17,target=b.player)
        self.send(a,8,action=b.player)
        self.send(a,15,target=b.player,slot=0,payload=b'audio')
        self.send(a,5,other_player=b.player,value=1)
        self.send(a,2)
        self.packets(a);self.packets(b)
        self.send(a,1,ship_type=0,team=0,option=1,screen_width=640,screen_height=480)
        own=next(p for p in self.packets(a) if p['opcode']==3 and p['fields']['player']==a.player)
        self.assertEqual(own['fields']['field4f'],-1)
        self.assertEqual(a.spectating,-1)
        with self.assertRaises(ValueError):self.send(a,15,target=b.player,slot=0,payload=b'')
        self.send(a,5,other_player=b.player,value=1)
        self.assertEqual(self.arena.accounts.profile(a.name)[1],2)

    def test_slow_destination_does_not_disconnect_sender(self):
        a=self.client(b'a');b=self.client(b'b');self.packets(a);self.packets(b)
        for _ in range(b.outbox.maxsize): b.send(2)
        self.send(a,6,target=-1,text=b'healthy sender')
        self.assertTrue(a.alive)
        self.assertFalse(b.alive)

    def test_lists_include_offline_accounts_and_push_is_allowlisted(self):
        a=self.client(b'a');b=self.client(b'b');self.packets(a);self.packets(b)
        self.send(b,2);self.packets(a)
        self.send(a,10)
        self.assertEqual([r['string1'] for r in self.packets(a)[0]['fields']['records']],[b'a',b'b'])
        self.arena.push_asset(a,'news.txt')
        packet=self.packets(a)[0]
        self.assertEqual(packet['fields']['data'],self.arena.news)
        with self.assertRaises(ValueError): self.arena.push_asset(a,'../secret')

    def test_malformed_and_unauthenticated_rejected(self):
        c=Client(None)
        for packet in (b'',b'\x01',b'\x09bad',b'\xff'):
            with self.assertRaises(ValueError): self.arena.handle(c,packet)
        with self.assertRaises(ValueError): self.send(c,12)

    def test_full_prize_pool_and_disabled_spawning(self):
        c=self.client();self.packets(c)
        c.last_state={'word2':8000,'word4':8000}
        self.assertEqual(self.arena.prizes,tuple(range(1,22)))
        self.arena.rng=Mock()
        self.arena.rng.choice.side_effect=[c,21]
        self.arena.rng.randint.return_value=0
        self.arena.next_pickup=0
        self.arena.tick()
        self.assertEqual(self.packets(c)[0]['fields']['event'],21)
        self.arena.prizes=pickup_pool('none')
        self.arena.next_pickup=0
        self.arena.tick()
        self.assertEqual(self.packets(c),[])
        self.assertEqual(pickup_pool('1,-13,21'),(1,-13,21))
        for value in ('','0','22','-22','abc','1,'):
            with self.assertRaises(argparse.ArgumentTypeError):pickup_pool(value)


if __name__=='__main__': unittest.main()
