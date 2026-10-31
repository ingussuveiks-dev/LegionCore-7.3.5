"""Read the exact local26972 Grash model; metadata evidence, not a renderer.

Layout reference (verify against this local payload, not modern client behavior):
https://github.com/wowmodelviewer/wowmodelviewer/blob/ab6060333415dbcc31328447896ea33c2a36a4f6/Source/games/wow/modelheaders.h
The project's matching vmap4_extractor/modelheaders.h defines the header fields.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('model', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
payload = args.model.read_bytes()
digest = hashlib.sha256(payload).hexdigest()
assert digest == '35b768f5f5d9c2c8c110c21ea65217c991e8187ad0e68f6427176bdc062bae66'
start = payload.find(b'MD20')
assert start == 8 and payload[:4] == b'MD21'
chunk_size, = struct.unpack_from('<I', payload, 4)
assert start + chunk_size <= len(payload)
u32 = lambda at: struct.unpack_from('<I', payload, start + at)[0]
assert u32(4) == 274
count, offset, lookup_count, lookup_offset = [u32(at) for at in (28, 32, 36, 40)]
assert count == 59 and lookup_count == 113
assert offset + count * 64 <= chunk_size and lookup_offset + lookup_count * 2 <= chunk_size
fmt = '<hhIfIHHIII7fhh'
assert struct.calcsize(fmt) == 64
sequences = [struct.unpack_from(fmt, payload, start + offset + i * 64) for i in range(count)]
lookup = struct.unpack_from('<' + 'h' * lookup_count, payload, start + lookup_offset)
rows = []
for index, row in enumerate(sequences):
    assert 0 <= row[0] < 2000 and 0 < row[2] <= 600000
    if row[4] & 0x40:
        assert 0 <= row[-1] < count
    rows.append(dict(index=index, id=row[0], variation=row[1], duration_ms=row[2],
                     flags=row[4], next=row[-2], alias=row[-1]))
lookups = []
for animation, index in enumerate(lookup):
    if index < 0:
        continue
    assert index < count
    # This valid model contains remapped IDs. Preserve them for review instead
    # of inventing a same-ID invariant or silently discarding those records.
    lookups.append(dict(request=animation, sequence=index, sequence_id=sequences[index][0]))
assert len(lookups) == 55
stomp, = [row for row in rows if row['id'] == 181]
assert stomp['duration_ms'] == 1600 and not stomp['flags'] & 0x40
result = dict(source='matching26972/CASC File1276106/display69084/model8628',
              sha256=digest, md20_offset=start, version=u32(4), sequences=count,
              sequence_bytes=64, lookup_count=lookup_count, lookups=lookups,
              animations=rows, limitation='Metadata only; no rendered carry/stomp pose, client playback or boss threshold proof.')
args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print('PASS exact native Grash MD21/MD20/version274:59 bounded64-byte sequences,55 valid lookup indices, real non-alias181 stomp1600ms; no rendering claim')
