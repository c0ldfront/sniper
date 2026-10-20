"""Independent SNIPER Huffman envelope codec, reconstructed from x86 routines.

0x410920 encoder; 0x410c60 decoder; 0x410e50 stable Huffman builder.
No networking or Binary Ninja dependency. Safety limits deliberately stricter
than the original. Padding bits are canonical zero (original leaves them unspecified).
"""
from collections import Counter


def _tree(freq):
    nodes = [(n, i, None, None) for i, n in enumerate(freq) if n]
    while len(nodes) > 1:
        left = nodes.pop(min(range(len(nodes)), key=lambda i: nodes[i][0]))
        right = nodes.pop(min(range(len(nodes)), key=lambda i: nodes[i][0]))
        nodes.append((left[0] + right[0], None, left, right))
    return nodes[0] if nodes else None


def _codes(node, value=0, width=0, result=None):
    if result is None:
        result = {}
    if node[1] is not None:
        result[node[1]] = (value, width)
    else:
        _codes(node[2], value << 1, width + 1, result)
        _codes(node[3], (value << 1) | 1, width + 1, result)
    return result


def compress(data, max_passes=1, disable=False):
    """Encode envelope, keeping a pass only if it reduces payload size.

    Single-symbol payloads stay raw: the original's zero-bit encoding loses
    their content on decode. Width >25 is conservatively left raw because
    the original writes codes through a single 32-bit window.
    """
    if not isinstance(max_passes, int) or not 0 <= max_passes <= 32:
        raise ValueError('max_passes must be in 0..32')
    data = bytes(data)
    passes = 0
    for _ in range(0 if disable else max_passes):
        counts = Counter(data)
        if len(counts) < 2 or max(counts.values()) > 0xffffff:
            break
        freq = [counts[i] for i in range(256)]
        codes = _codes(_tree(freq))
        if max(width for _, width in codes.values()) > 25:
            break
        bit_count = sum(freq[i] * width for i, (_, width) in codes.items())
        if bit_count > 0xffffffff or 772 + (bit_count + 7) // 8 >= len(data):
            break
        encoded = bytearray((bit_count + 7) // 8)
        pos = 0
        for symbol in data:
            value, width = codes[symbol]
            for shift in range(width - 1, -1, -1):
                encoded[pos >> 3] |= ((value >> shift) & 1) << (7 - (pos & 7))
                pos += 1
        data = b''.join(n.to_bytes(3, 'big') for n in freq) + bit_count.to_bytes(4, 'big') + encoded
        passes += 1
    return passes.to_bytes(4, 'big') + data


def decompress(envelope, max_output=16 * 1024 * 1024, max_passes=32):
    """Strict bounded decoder. Reject malformed/ambiguous legacy layers."""
    envelope = bytes(envelope)
    if len(envelope) < 4:
        raise ValueError('missing pass count')
    passes = int.from_bytes(envelope[:4], 'big')
    if passes > max_passes:
        raise ValueError('pass limit exceeded')
    data = envelope[4:]
    if len(data) > max_output:
        raise ValueError('input exceeds output limit')
    for _ in range(passes):
        if len(data) < 772:
            raise ValueError('truncated layer header')
        freq = [int.from_bytes(data[i:i+3], 'big') for i in range(0, 768, 3)]
        bit_count = int.from_bytes(data[768:772], 'big')
        expected = sum(freq)
        if expected > max_output:
            raise ValueError('output limit exceeded')
        if len(data) != 772 + (bit_count + 7) // 8:
            raise ValueError('layer bit length mismatch')
        root = _tree(freq)
        if root is None or root[1] is not None:
            raise ValueError('empty or ambiguous single-symbol layer')
        node = root
        out = bytearray()
        observed = [0] * 256
        for pos in range(bit_count):
            bit = (data[772 + (pos >> 3)] >> (7 - (pos & 7))) & 1
            node = node[2 + bit]
            if node[1] is not None:
                out.append(node[1])
                observed[node[1]] += 1
                if len(out) > expected:
                    raise ValueError('decoded length exceeds histogram')
                node = root
        if node is not root or observed != freq:
            raise ValueError('bitstream/histogram mismatch')
        data = bytes(out)
    return data
