"""Independent SNIPER 0.87 transport, from 401090/401440/401610/401920/401e00.

Not a byte-for-byte reproduction of unsafe parser behavior: sizes, slot indices,
recursion, decompression output and memory are bounded. Modular sequence ordering
is a deliberate improvement over the original signed comparisons at wraparound.
Timing uses explicit seconds, whereas the original uses GetTickCount()/10.
"""
from collections import deque
from dataclasses import dataclass
import socket
import struct

MASK = 0xffffffff

class ProtocolError(ValueError):
    pass


def encode_endpoint(host, port):
    """401530-40155b: IPv4 address bytes followed by network-order UDP port."""
    if not 0 <= port <= 65535:
        raise ProtocolError('invalid port')
    return socket.inet_aton(host) + struct.pack('!H', port)


def decode_endpoint(data):
    if len(data) != 6:
        raise ProtocolError('endpoint must be exactly six bytes')
    return socket.inet_ntoa(data[:4]), struct.unpack('!H', data[4:])[0]


def encode_control(kind, *, sequence=0, slot=0, tick=0, remote_tick=0, payload=b''):
    prefix = bytes((0, kind))
    if kind == 0:
        return prefix + struct.pack('<I', tick & MASK)
    if kind == 1:
        return prefix + struct.pack('<II', tick & MASK, remote_tick & MASK)
    if kind in (2, 3):
        if not 0 <= slot < 256 or (kind == 3 and payload):
            raise ProtocolError('invalid reliable slot or ACK payload')
        if kind == 2 and not 1 <= len(payload) <= 500:
            raise ProtocolError('reliable payload must be 1..500 bytes')
        return prefix + struct.pack('<IH', sequence & MASK, slot) + payload
    if kind == 4:
        if payload:
            raise ProtocolError('close has no payload')
        return prefix
    if kind in (5, 6, 7):
        return prefix + payload
    raise ProtocolError('unknown transport opcode')


def decode_control(data):
    """Strict control decoding; 401920's original parser does no length checks."""
    if len(data) < 2 or data[0] != 0 or data[1] > 7:
        raise ProtocolError('invalid control header')
    kind = data[1]
    out = {'kind': kind}
    exact = {0: 6, 1: 10, 3: 8, 4: 2}
    if kind in exact and len(data) != exact[kind]:
        raise ProtocolError('incorrect control length')
    if kind == 0:
        out['tick'], = struct.unpack('<I', data[2:])
    elif kind == 1:
        out['tick'], out['remote_tick'] = struct.unpack('<II', data[2:])
    elif kind in (2, 3):
        if kind == 2 and not 9 <= len(data) <= 508:
            raise ProtocolError('incorrect reliable length')
        out['sequence'], out['slot'] = struct.unpack('<IH', data[2:8])
        if out['slot'] >= 256:
            raise ProtocolError('slot out of bounds')
        out['payload'] = data[8:]
    elif kind in (5, 6, 7):
        out['payload'] = data[2:]
    return out


def encode_bundle(payloads):
    """4019fd sign-extends bundle lengths: restrict interoperability to 1..127."""
    result = bytearray(b'\0\7')
    for payload in payloads:
        if not 1 <= len(payload) <= 127:
            raise ProtocolError('legacy-safe bundle length is 1..127')
        result.append(len(payload))
        result.extend(payload)
    if len(result) > 500:
        raise ProtocolError('bundle exceeds datagram payload limit')
    return bytes(result)


def decode_bundle(data):
    items = []
    pos = 0
    while pos < len(data):
        length = data[pos]
        pos += 1
        if not 1 <= length <= 127 or pos + length > len(data):
            raise ProtocolError('invalid bundle member length')
        items.append(data[pos:pos + length])
        pos += length
    return items


@dataclass
class _Pending:
    sequence: int
    slot: int
    data: bytes
    last_send: float = float('-inf')
    acknowledged: bool = False


class ReliableChannel:
    """Bounded ordered UDP channel; initial sequence1000, send256/receive16.

    poll() mirrors first14 ring positions and 1.5+5*i second retry thresholds
    (401f08-401f88). ACKed holes still count until the head advances. Fragment
    pumping mirrors 401e23: enqueue one chunk only when <=1 ring position remains.
    Call poll periodically; receive returns ACKs/pongs but does not poll implicitly.
    No automatic ping calibration; send encode_control(0,tick=...) explicitly.
    """
    def __init__(self, *, decoder=None, encoder=None, max_message_size=8*1024*1024,
                 initial_sequence=1000):
        if max_message_size < 1:
            raise ValueError('max_message_size must be positive')
        self.decoder, self.encoder = decoder, encoder
        self.max_message_size = max_message_size
        self.next_send_sequence = initial_sequence & MASK
        self.next_receive_sequence = initial_sequence & MASK
        self._next_slot = 0
        self._pending = deque()
        self._received = {}
        self._large = deque()
        self._fragment = bytearray()
        self._fragment_kind = None
        self.closed = False
        self.last_pong = None

    @property
    def pending_count(self):
        return len(self._pending)

    def _queue(self, payload):
        if len(self._pending) >= 256:
            raise BufferError('reliable send ring full')
        seq, slot = self.next_send_sequence, self._next_slot
        data = encode_control(2, sequence=seq, slot=slot, payload=payload)
        self._pending.append(_Pending(seq, slot, data))
        self.next_send_sequence = (seq + 1) & MASK
        self._next_slot = (slot + 1) % 256

    def send(self, payload, reliable=True, now=0.0, compress=False):
        if self.closed:
            raise ProtocolError('channel closed')
        payload = bytes(payload)
        if not 1 <= len(payload) <= self.max_message_size:
            raise ProtocolError('message size out of bounds')
        if len(payload) > 500:
            if len(self._large) >= 16:
                raise BufferError('large message queue full')
            if compress:
                if self.encoder is None:
                    raise ProtocolError('compressed send requires encoder')
                payload = bytes(self.encoder(payload))
            if len(payload) > self.max_message_size:
                raise ProtocolError('encoded message too large')
            self._large.append([6 if compress else 5, payload, 0])
            return self.poll(now)
        if reliable:
            self._queue(payload)
            return self.poll(now)
        # Original drops unreliable payloads while ring count >=32 (401820).
        return ([payload] if len(self._pending) < 32 else []) + self.poll(now)

    def poll(self, now=0.0):
        if self.closed:
            return []
        if len(self._pending) <= 1 and self._large:
            item = self._large[0]
            kind, data, pos = item
            chunk = data[pos:pos + 480]
            self._queue(bytes((0, kind)) + chunk)
            item[2] += len(chunk)
            if not chunk:  # a separate empty fragment terminates even exact multiples
                self._large.popleft()
        result = []
        for i, entry in enumerate(list(self._pending)[:14]):
            if not entry.acknowledged and now - entry.last_send > 1.5 + 5*i:
                result.append(entry.data)
                entry.last_send = now
        return result

    def receive(self, datagram, now=0.0):
        if self.closed:
            return [], []
        datagram = bytes(datagram)
        if not 1 <= len(datagram) <= 512:
            raise ProtocolError('invalid datagram length')
        payloads, replies = [], []
        self._dispatch(datagram, now, payloads, replies, 0)
        return payloads, replies

    def _dispatch(self, data, now, payloads, replies, depth):
        if depth > 8:
            raise ProtocolError('nested controls too deep')
        if not data:
            raise ProtocolError('empty application message')
        if data[0]:
            payloads.append(data)
            return
        packet = decode_control(data)
        kind = packet['kind']
        if kind == 0:
            replies.append(encode_control(1, tick=packet['tick'], remote_tick=int(now*100)))
        elif kind == 1:
            self.last_pong = packet
        elif kind == 2:
            seq = packet['sequence']
            distance = (seq - self.next_receive_sequence) & MASK
            old = distance >= 0x80000000
            accepted = old or seq in self._received
            if not accepted and len(self._received) < 16:
                self._received[seq] = packet['payload']
                accepted = True
            if accepted:
                replies.append(encode_control(3, sequence=seq, slot=packet['slot']))
            while self.next_receive_sequence in self._received:
                body = self._received.pop(self.next_receive_sequence)
                self.next_receive_sequence = (self.next_receive_sequence + 1) & MASK
                self._dispatch(body, now, payloads, replies, depth + 1)
        elif kind == 3:
            for entry in self._pending:
                if entry.slot == packet['slot'] and entry.sequence == packet['sequence']:
                    entry.acknowledged = True
                    break
            while self._pending and self._pending[0].acknowledged:
                self._pending.popleft()
        elif kind == 4:
            self.closed = True
            self._pending.clear()
            self._large.clear()
            self._received.clear()
            self._fragment.clear()
        elif kind in (5, 6):
            chunk = packet['payload']
            if self._fragment_kind not in (None, kind):
                raise ProtocolError('mixed fragment stream types')
            if chunk:
                if len(chunk) > 480 or len(self._fragment) + len(chunk) > self.max_message_size:
                    raise ProtocolError('fragment stream exceeds bounds')
                self._fragment_kind = kind
                self._fragment.extend(chunk)
            else:
                body = bytes(self._fragment)
                self._fragment.clear()
                self._fragment_kind = None
                if kind == 6:
                    if self.decoder is None:
                        raise ProtocolError('compressed receive requires decoder')
                    body = bytes(self.decoder(body))
                if len(body) > self.max_message_size:
                    raise ProtocolError('decoded message too large')
                self._dispatch(body, now, payloads, replies, depth + 1)
        elif kind == 7:
            for body in decode_bundle(packet['payload']):
                self._dispatch(body, now, payloads, replies, depth + 1)
