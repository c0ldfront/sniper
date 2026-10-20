"""Loopback integration only; no original game/client process is launched."""
import socket
import threading
import time
import unittest

from socket_session import bootstrap_client, bootstrap_server, _exact, ClockEstimate


class SocketTests(unittest.TestCase):
    def test_clock_minimum_rtt(self):
        clock = ClockEstimate()
        clock.observe(100, 1000, 110)
        self.assertEqual((clock.minimum_rtt, clock.offset), (10, 895))
        self.assertEqual(clock.game_tick(111), 1006)
        clock.observe(100, 2000, 120)
        self.assertEqual(clock.offset, 895)
        clock.observe(0xfffffffe, 50, 2)
        self.assertEqual(clock.minimum_rtt, 4)

    def test_tcp_split_reads_and_eof(self):
        a, b = socket.socketpair()
        with a, b:
            b.sendall(b'abc')
            b.sendall(b'def')
            self.assertEqual(_exact(a, 6), b'abcdef')
            b.shutdown(socket.SHUT_WR)
            with self.assertRaises(ConnectionError):
                _exact(a, 6)

    def test_bootstrap_and_bidirectional_messages(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
            listener.bind(('127.0.0.1', 0))
            listener.listen(1)
            listener.settimeout(3)
            server, errors = [], []
            def accept():
                try:
                    server.append(bootstrap_server(listener))
                except BaseException as exc:
                    errors.append(exc)
            thread = threading.Thread(target=accept, daemon=True)
            thread.start()
            with bootstrap_client(*listener.getsockname()) as client:
                thread.join(4)
                self.assertFalse(thread.is_alive())
                self.assertFalse(errors, errors)
                self.assertEqual(len(server), 1)
                with server[0] as peer:
                    expected = [b'\x01\x2a', b'\x06hello\0', b'\x12'+bytes(range(256))*20,
                                b'\x12'+b'ABCD'*3000]
                    received = []
                    for i, message in enumerate(expected):
                        client.send(message, compressed=(i==3))
                        deadline = time.monotonic()+3
                        while len(received) < i+1 and time.monotonic() < deadline:
                            received.extend(peer.receive(0.001))
                            client.receive(0.001)
                        self.assertEqual(received, expected[:i+1])
                    peer.send(b'\x02', reliable=True)
                    replies = []
                    deadline = time.monotonic()+3
                    while not replies and time.monotonic() < deadline:
                        replies.extend(client.receive(0.001))
                        peer.receive(0.001)
                    self.assertEqual(replies, [b'\x02'])


if __name__ == '__main__':
    unittest.main()
