"""IPv4 TCP endpoint exchange and connected UDP adapter for SNIPER v87.

Evidence: 0x401090 and 0x401440. No game/authentication server policy is
invented here. Caller owns application messages and drives poll/receive.
"""
import socket
import time

from sniper_compression import compress, decompress
from transport import ReliableChannel, encode_endpoint, decode_endpoint, encode_control


class ClockEstimate:
    """Minimum-RTT offset used by 401920; game tick formula from 411860.

    Explicit probes avoid tying calibration policy to an application loop.
    Tick arguments are integer units of 10ms. Wrap uses modular arithmetic.
    """
    def __init__(self):
        self.minimum_rtt = None
        self.offset = 0

    def observe(self, echoed_tick, remote_tick, local_tick):
        rtt = (local_tick - echoed_tick) & 0xffffffff
        if rtt >= 0x80000000:
            return
        if self.minimum_rtt is None or rtt < self.minimum_rtt:
            self.minimum_rtt = rtt
            self.offset = remote_tick - local_tick + rtt // 2

    def game_tick(self, local_tick):
        return ((local_tick + self.offset) & 0xffffffff) % 0x19999999


def _exact(sock, count):
    data = bytearray()
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        if not chunk:
            raise ConnectionError('TCP endpoint exchange ended before six bytes')
        data.extend(chunk)
    return bytes(data)


class SocketSession:
    """Single-owner session; receive() also services retries and acknowledgements."""
    def __init__(self, udp, channel=None):
        self.socket = udp
        self.channel = channel or ReliableChannel(encoder=compress, decoder=decompress)
        self.clock = ClockEstimate()

    def probe_clock(self):
        self._emit([encode_control(0, tick=int(time.monotonic()*100))])

    def game_tick(self):
        return self.clock.game_tick(int(time.monotonic()*100))

    def _emit(self, datagrams):
        for datagram in datagrams:
            if self.socket.send(datagram) != len(datagram):
                raise OSError('short UDP send')

    def send(self, payload, reliable=True, compressed=False):
        self._emit(self.channel.send(payload, reliable=reliable,
                                    compress=compressed, now=time.monotonic()))

    def poll(self):
        self._emit(self.channel.poll(time.monotonic()))

    def receive(self, timeout=0.1):
        if timeout < 0:
            raise ValueError('timeout must be nonnegative')
        self.poll()
        self.socket.settimeout(timeout)
        try:
            data = self.socket.recv(65535)
        except (socket.timeout, BlockingIOError):
            return []
        now = time.monotonic()
        previous_pong = self.channel.last_pong
        messages, replies = self.channel.receive(data, now=now)
        pong = self.channel.last_pong
        if pong is not None and pong is not previous_pong:
            self.clock.observe(pong['tick'], pong['remote_tick'], int(now*100))
        self._emit(replies)
        self.poll()
        return messages

    def close(self):
        self.socket.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


def bootstrap_client(host, port, timeout=5.0):
    """Connect TCP, advertise local ephemeral UDP endpoint, then close TCP."""
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as tcp:
            tcp.settimeout(timeout)
            tcp.connect((host, port))
            udp.bind((tcp.getsockname()[0], 0))
            tcp.sendall(encode_endpoint(*udp.getsockname()))
            endpoint = decode_endpoint(_exact(tcp, 6))
            if endpoint[1] == 0:
                raise ValueError('peer advertised UDP port zero')
            udp.connect(endpoint)
        return SocketSession(udp)
    except BaseException:
        udp.close()
        raise


def bootstrap_server(listener, timeout=5.0):
    """Accept one peer; listener must be an already-bound IPv4 TCP socket.

    Sends exactly six reply bytes, compatible with the original client's
    strict MSG_PEEK length==6 test. Original client uses a ~5 second deadline.
    This supplies the inferred reciprocal bootstrap, not original server code.
    """
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        tcp, _peer = listener.accept()
        with tcp:
            tcp.settimeout(timeout)
            endpoint = decode_endpoint(_exact(tcp, 6))
            if endpoint[1] == 0:
                raise ValueError('peer advertised UDP port zero')
            udp.bind((tcp.getsockname()[0], 0))
            udp.connect(endpoint)
            tcp.sendall(encode_endpoint(*udp.getsockname()))
        return SocketSession(udp)
    except BaseException:
        udp.close()
        raise
