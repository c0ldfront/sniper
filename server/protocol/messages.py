"""Independent SNIPER 0.87 application codec.

No network or filesystem side effects. Fixed-size strings are raw bytes so no
unproven character encoding is imposed. Unknown opcodes fail closed.
"""
import struct

# opcode: (name, struct format excluding opcode, ordered field names)
C2S = {
 1: ('join', 'Biiii', 'ship_type team option screen_width screen_height'),
 2: ('leave', '', ''),
 3: ('state', 'BhhhhIBhH', 'state_byte word2 word4 word6 word8 tick extra_byte extra_word flags'),
 5: ('death', 'Bh', 'other_player value'),
 7: ('pickup', 'hhhB', 'x y value player'),
 8: ('action', 'B', 'action'),
 9: ('login', 'B32s32s32s', 'create_user name password third_string'),
 10: ('list_request', '', ''), 11: ('file_request', '', ''),
 12: ('map_request', '', ''), 13: ('news_request', '', ''),
 14: ('download_done_or_cancel', '', ''),
 16: ('team', 'i', 'team'), 17: ('attach', 'h', 'target'),
 18: ('arena_list_file_request', '', ''),
 19: ('squad_request', '32s32s', 'name password'),
}
S2C = {
 1: ('assign_player', 'B', 'player'), 2: ('ready', '', ''),
 3: ('add_player', 'Bb32s32sIhhIhI', 'player ship_type name squad field43 field47 field49 field4b field4f field51'),
 4: ('remove_player', 'B', 'player'),
 5: ('state', 'bhhhhBIbhH', 'state_byte word2 word4 word6 word8 player tick extra_byte extra_word flags'),
 6: ('death', 'BBhb', 'player other_player value extra'),
 8: ('pickup', 'hhhB', 'x y value player'),
 9: ('event', 'bhhI', 'event word2 word4 value'),
 10: ('login_reply', 'BiIII', 'status minimum_version map_checksum field0a arena_list_checksum'),
 13: ('file_size', 'I', 'total_size'),
 15: ('team', 'ii', 'player team'), 16: ('attach', 'hh', 'player target'),
 17: ('settings', '288s', 'opaque'),
 19: ('squad_reply', 'b', 'status'),
}
VARIABLE = {
 ('c2s', 6): ('chat', 'h', 'target', 'text', True),
 ('s2c', 7): ('chat', 'Bh', 'sender target', 'text', True),
 ('c2s', 15): ('voice', 'hB', 'target slot', 'payload', False),
 ('s2c', 14): ('voice', 'b', 'player', 'payload', False),
 ('s2c', 12): ('file_chunk', 'I', 'offset', 'data', False),
 ('s2c', 18): ('file_push', '16s', 'filename', 'data', False),
}


def _table(direction):
    if direction not in ('c2s', 's2c'):
        raise ValueError('direction must be c2s or s2c')
    return C2S if direction == 'c2s' else S2C


def _pack(fmt, names, fields):
    values = [fields[n] for n in names.split()]
    # struct.pack silently truncates byte strings: reject that explicitly.
    import re
    specs = re.findall(r'\d*s|[BbHhIi]', fmt)
    for index, (spec, value) in enumerate(zip(specs, values)):
        if spec.endswith('s'):
            if not isinstance(value, bytes) or len(value) > int(spec[:-1]):
                raise ValueError('fixed byte string exceeds capacity or is not bytes')
            field = names.split()[index]
            capacity = int(spec[:-1])
            if field == 'opaque' and len(value) != capacity:
                raise ValueError('opaque settings require exactly 288 bytes')
            if field != 'opaque' and len(value) == capacity and b'\0' not in value:
                raise ValueError('fixed text requires space for terminal NUL')
    try:
        return struct.pack('<' + fmt, *values)
    except (struct.error, TypeError) as exc:
        raise ValueError(str(exc)) from exc


def encode(direction, opcode, **fields):
    table = _table(direction)
    if direction == 's2c' and opcode == 11:
        if set(fields) != {'records'}:
            raise ValueError('list reply requires records')
        parts = [b'\x0b']
        for record in fields['records']:
            if set(record) != {'string1', 'string2', 'value1', 'value2', 'value3'}:
                raise ValueError('invalid list record fields')
            for key in ('string1', 'string2'):
                value = record[key]
                if not isinstance(value, bytes) or b'\0' in value or len(value) > 31:
                    raise ValueError('list string must fit original 32-byte field')
                parts.append(value + b'\0')
            parts.append(_pack('HHI', 'value1 value2 value3', record))
        return b''.join(parts)
    if opcode in table:
        _, fmt, names = table[opcode]
        if set(fields) != set(names.split()):
            raise ValueError('missing or unexpected fields')
        return bytes([opcode]) + _pack(fmt, names, fields)
    if (direction, opcode) in VARIABLE:
        _, fmt, names, tail, terminated = VARIABLE[direction, opcode]
        if set(fields) != set(names.split()) | {tail}:
            raise ValueError('missing or unexpected fields')
        value = fields[tail]
        if not isinstance(value, bytes):
            raise ValueError('payload must be bytes')
        if terminated and b'\0' in value:
            raise ValueError('text must not include a NUL')
        result = bytes([opcode]) + _pack(fmt, names, fields) + value
        if terminated:
            result += b'\0'
            if direction == 'c2s' and len(result) > 512:
                raise ValueError('chat exceeds original client buffer')
        return result
    raise ValueError('unsupported opcode for direction')


def decode(direction, packet):
    table = _table(direction)
    if not isinstance(packet, bytes) or not packet:
        raise ValueError('nonempty bytes required')
    opcode = packet[0]
    if opcode in table:
        name, fmt, names = table[opcode]
        if len(packet) != 1 + struct.calcsize('<' + fmt):
            raise ValueError('incorrect fixed packet length')
        result = dict(zip(names.split(), struct.unpack('<' + fmt, packet[1:])))
    elif (direction, opcode) in VARIABLE:
        name, fmt, names, tail, terminated = VARIABLE[direction, opcode]
        end = 1 + struct.calcsize('<' + fmt)
        if len(packet) < end + int(terminated):
            raise ValueError('truncated packet')
        result = dict(zip(names.split(), struct.unpack('<' + fmt, packet[1:end])))
        value = packet[end:]
        if terminated:
            if value[-1:] != b'\0' or b'\0' in value[:-1]:
                raise ValueError('text requires exactly one terminal NUL')
            if direction == 'c2s' and len(packet) > 512:
                raise ValueError('chat exceeds original client buffer')
            value = value[:-1]
        result[tail] = value
    elif direction == 's2c' and opcode == 11:
        name, result = 'list_reply', {'records': []}
        cursor = 1
        while cursor < len(packet):
            strings = []
            for _ in range(2):
                end = packet.find(b'\0', cursor)
                if end < 0:
                    raise ValueError('unterminated list string')
                strings.append(packet[cursor:end])
                cursor = end + 1
            if len(packet) - cursor < 8:
                raise ValueError('truncated list record')
            a, b, c = struct.unpack_from('<HHI', packet, cursor)
            cursor += 8
            result['records'].append(dict(string1=strings[0], string2=strings[1], value1=a, value2=b, value3=c))
    else:
        raise ValueError('unsupported opcode for direction')
    return dict(direction=direction, opcode=opcode, name=name, fields=result)


def pack_state_flags(field0, field3, field5, field8, field10, field15):
    result = 0
    for value, shift, width in zip((field0, field3, field5, field8, field10, field15), (0,3,5,8,10,15), (3,2,3,2,5,1)):
        if not isinstance(value, int) or not 0 <= value < 1 << width:
            raise ValueError('flag field out of range')
        result |= value << shift
    return result


def unpack_state_flags(flags):
    if not isinstance(flags, int) or not 0 <= flags <= 65535:
        raise ValueError('flags must fit u16')
    return tuple((flags >> shift) & ((1 << width)-1) for shift, width in zip((0,3,5,8,10,15), (3,2,3,2,5,1)))
