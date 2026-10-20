import unittest
from transport import (ReliableChannel, ProtocolError, encode_control, decode_control,
                       encode_endpoint, decode_endpoint, encode_bundle)


class TransportTests(unittest.TestCase):
    def test_literal_wire_vectors(self):
        self.assertEqual(encode_endpoint('127.0.0.1', 1234), bytes.fromhex('7f00000104d2'))
        self.assertEqual(decode_endpoint(bytes.fromhex('c0a801020050')), ('192.168.1.2', 80))
        self.assertEqual(encode_control(0, tick=0x12345678), bytes.fromhex('000078563412'))
        self.assertEqual(encode_control(1, tick=1, remote_tick=2), bytes.fromhex('00010100000002000000'))
        self.assertEqual(encode_control(2, sequence=1000, slot=7, payload=b'\x19x'),
                         bytes.fromhex('0002e803000007001978'))
        self.assertEqual(encode_control(3, sequence=1000, slot=7), bytes.fromhex('0003e80300000700'))
        self.assertEqual(encode_bundle([b'\x19a', b'\x20']), bytes.fromhex('00070219610120'))

    def test_order_duplicates_and_ack_matching(self):
        a, b = ReliableChannel(), ReliableChannel()
        first = a.send(b'\x19first')[0]
        second = a.send(b'\x19second')[0]
        out, ack = b.receive(second)
        self.assertEqual(out, [])
        a.receive(ack[0])
        self.assertEqual(a.pending_count, 2)  # acknowledged hole retained
        out, ack = b.receive(first)
        self.assertEqual(out, [b'\x19first', b'\x19second'])
        a.receive(ack[0])
        self.assertEqual(a.pending_count, 0)
        out, ack = b.receive(first)
        self.assertEqual(out, [])
        self.assertEqual(len(ack), 1)
        c = ReliableChannel()
        c.send(b'\x19x')
        c.receive(encode_control(3, sequence=1001, slot=0))
        self.assertEqual(c.pending_count, 1)

    def test_retry_loss_and_strict_threshold(self):
        a = ReliableChannel()
        first = a.send(b'\x19x', now=10)[0]
        self.assertEqual(a.poll(11.5), [])
        self.assertEqual(a.poll(11.51), [first])
        second = a.send(b'\x19y', now=12)[0]
        self.assertNotIn(second, a.poll(18.5))
        self.assertIn(second, a.poll(18.51))

    def test_sequence_wrap(self):
        a, b = ReliableChannel(initial_sequence=0xffffffff), ReliableChannel(initial_sequence=0xffffffff)
        first, second = a.send(b'A')[0], a.send(b'B')[0]
        self.assertEqual(b.receive(second)[0], [])
        self.assertEqual(b.receive(first)[0], [b'A', b'B'])
        self.assertEqual(b.next_receive_sequence, 1)
        self.assertEqual(b.receive(first)[0], [])

    def test_receive_capacity(self):
        b = ReliableChannel()
        for seq in range(1001, 1017):
            self.assertEqual(len(b.receive(encode_control(2, sequence=seq, payload=b'X'))[1]), 1)
        # Original also refuses missing head when all16 slots occupied; peer must
        # keep its active window <=14, as original sender does.
        self.assertEqual(b.receive(encode_control(2, sequence=1000, payload=b'X')), ([], []))

    def test_send_window_and_slots(self):
        a = ReliableChannel()
        for i in range(256):
            got = a.send(b'X')
            self.assertEqual(len(got), 1 if i < 14 else 0)
        with self.assertRaises(BufferError):
            a.send(b'X')
        a.receive(encode_control(3, sequence=1000, slot=0))
        a.send(b'X')
        self.assertEqual(a._pending[-1].slot, 0)

    def test_fragments_and_empty_terminator(self):
        a, b = ReliableChannel(), ReliableChannel()
        data = b'\x19' + bytes(range(256))*8
        datagrams = a.send(data)
        output = []
        seen = []
        for step in range(100):
            for datagram in datagrams:
                seen.append(decode_control(datagram)['payload'])
                payloads, replies = b.receive(datagram, step)
                output.extend(payloads)
                for reply in replies:
                    a.receive(reply, step)
            datagrams = a.poll(step + 1)
            if not datagrams and not a._large and not a.pending_count:
                break
        self.assertEqual(output, [data])
        self.assertEqual(seen[-1], b'\0\5')
        self.assertTrue(all(len(x) <= 482 for x in seen))

    def test_bundle_ping_close(self):
        b = ReliableChannel()
        self.assertEqual(b.receive(encode_bundle([b'A', b'B']))[0], [b'A', b'B'])
        self.assertEqual(b.receive(bytes.fromhex('000078563412'), now=2)[1],
                         [bytes.fromhex('000178563412c8000000')])
        b.receive(b'\0\4')
        self.assertTrue(b.closed)
        self.assertEqual(b.poll(), [])

    def test_bounds(self):
        for bad in [b'\0', b'\0\x08', b'\0\0', b'\0\4x',
                    bytes.fromhex('0003e80300000001'), b'\0\7\x80' + b'x'*128]:
            with self.assertRaises(ProtocolError):
                ReliableChannel().receive(bad)
        b = ReliableChannel(max_message_size=4)
        with self.assertRaises(ProtocolError):
            b.receive(b'\0\5abcde')
        with self.assertRaises(ProtocolError):
            encode_bundle([b'x'*128])
        b = ReliableChannel()
        b.receive(b'\0\5abc')
        with self.assertRaises(ProtocolError):
            b.receive(b'\0\6def')

    def test_injected_compression(self):
        # Tests transport integration only; the Huffman codec has independent tests.
        a = ReliableChannel(encoder=lambda x: b'\0\0\0\0' + x)
        b = ReliableChannel(decoder=lambda x: x[4:])
        original = b'X'*700
        datagrams = a.send(original, compress=True)
        output = []
        for step in range(20):
            for datagram in datagrams:
                payloads, replies = b.receive(datagram)
                output.extend(payloads)
                for reply in replies:
                    a.receive(reply)
            datagrams = a.poll(step + 1)
        self.assertEqual(output, [original])

if __name__ == '__main__':
    unittest.main()
