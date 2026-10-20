"""Read-only reconstruction of the 2026-10-20 override-aura crash.

Requires the original minidump/report and matching archived executable/map.
Outputs diagnostic identifiers only, not arbitrary captured player memory.
"""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import re
import struct


def audit(dump, report, executable, map_file):
    data = dump.read_bytes()
    if data[:4] != b'MDMP':
        raise ValueError('Expected Windows minidump')

    def unpack(offset, fmt):
        return struct.unpack_from('<' + fmt, data, offset)

    count, directory = unpack(8, 'II')
    streams = {}
    for index in range(count):
        kind, size, offset = unpack(directory + 12 * index, 'III')
        streams[kind] = (offset, size)

    offset, _ = streams[4]  # ModuleListStream
    module = None
    for index in range(unpack(offset, 'I')[0]):
        base, size, checksum, timestamp, name_offset = unpack(offset + 4 + 108 * index, 'QIIII')
        name_length = unpack(name_offset, 'I')[0]
        name = data[name_offset + 4:name_offset + 4 + name_length].decode('utf-16le')
        if name.lower().endswith('worldserver.exe'):
            module = (base, size, timestamp)
    if not module:
        raise ValueError('worldserver module missing')

    binary = executable.read_bytes()
    pe = struct.unpack_from('<I', binary, 0x3c)[0]
    timestamp = struct.unpack_from('<I', binary, pe + 8)[0]
    size = struct.unpack_from('<I', binary, pe + 24 + 56)[0]
    preferred_base = struct.unpack_from('<Q', binary, pe + 24 + 24)[0]
    if (size, timestamp) != module[1:]:
        raise ValueError('Executable does not match the crashed module')

    symbols = []
    with map_file.open(errors='replace') as stream:
        for line in stream:
            stamp = re.search(r'Timestamp is ([0-9A-Fa-f]+)', line)
            if stamp and int(stamp[1], 16) != timestamp:
                raise ValueError('Map does not match crashed executable timestamp')
            match = re.match(r'\s+0001:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]{16})\s+f', line)
            if match:
                symbols.append((int(match[2], 16), match[1]))
    symbols.sort()
    addresses = [address for address, _ in symbols]

    offset, _ = streams[6]  # ExceptionStream
    exception = unpack(offset + 8, 'I')[0]
    instruction = unpack(offset + 24, 'Q')[0]
    access, fault = unpack(offset + 40, 'QQ')
    context_size, context = unpack(offset + 160, 'II')
    if context_size < 256:
        raise ValueError('Missing x64 register context')
    effect_address = unpack(context + 232, 'Q')[0]  # R14, verified against original handler disassembly
    list_node = unpack(context + 144, 'Q')[0]  # RBX at mov r8,[rbx+10h]
    override_id = unpack(context + 160, 'Q')[0]  # RBP at this instruction

    offset, _ = streams[5]  # MemoryListStream (including indirect captured memory)
    ranges = [unpack(offset + 4 + 16 * index, 'QII') for index in range(unpack(offset, 'I')[0])]

    def memory(address, length):
        for start, size, file_offset in ranges:
            if start <= address and address + length <= start + size:
                at = file_offset + address - start
                return data[at:at + length]
        raise ValueError(f'Required diagnostic memory not captured at {address:x}')

    spell_info = struct.unpack('<Q', memory(effect_address + 8, 8))[0]
    spell_id = struct.unpack('<I', memory(spell_info, 4))[0]
    frames = []
    stack = report.read_text(errors='replace').split('Call stack:', 2)[1]
    for match in re.finditer(r'^([0-9A-F]{16})\s', stack, re.MULTILINE):
        address = int(match[1], 16)
        if module[0] <= address < module[0] + module[1]:
            logical = address - module[0] + preferred_base
            index = bisect.bisect_right(addresses, logical) - 1
            if index >= 0:
                symbol_address, symbol = symbols[index]
                frames.append({'symbol': symbol, 'offset': hex(logical - symbol_address)})

    if exception != 0xc0000005 or access != 0 or fault != 0x10 or list_node != 0:
        raise ValueError('Dump is not the expected null list-node read')
    if not frames or not frames[0]['symbol'].startswith('?HandleAuraOverrideSpells@AuraEffect@@'):
        raise ValueError('Exception is outside the override handler')
    return {
        'dump_sha256': hashlib.sha256(data).hexdigest(),
        'original_executable_sha256': hashlib.sha256(binary).hexdigest(),
        'module_timestamp': hex(timestamp),
        'exception': hex(exception),
        'access': 'read',
        'fault_address': hex(fault),
        'fault_rva': hex(instruction - module[0]),
        'null_list_node': list_node == 0,
        'spell': spell_id,
        'override_id': override_id,
        'frames': frames,
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('dump', 'report', 'executable', 'map'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.dump, args.report, args.executable, args.map), indent=2))
