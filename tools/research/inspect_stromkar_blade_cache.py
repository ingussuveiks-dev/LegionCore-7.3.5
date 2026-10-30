"""Read the selected public DB2 record from a closed Legion26972 cache."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('cache', type=Path)
args = parser.parse_args()
data = args.cache.read_bytes()
assert data[:4] == b'XFTH' and struct.unpack_from('<II', data, 4) == (5, 26972), 'Expected matching26972 cache5'
offset = 44  # signature/version/build followed by the native32-byte cache digest
records = []
while offset < len(data):
    assert offset + 28 <= len(data), 'Incomplete cache header'
    magic, unused, hotfix, size, table, record, extra = struct.unpack_from('<4s6I', data, offset)
    assert magic == b'XFTH', f'Unexpected cache boundary at{offset}'
    end = offset + 28 + size
    assert end <= len(data), 'Incomplete cache record'
    if table == 4033975491 and record == 318611:
        payload = data[offset + 28:end]
        assert size == 86 and struct.unpack_from('<I', payload, 24)[0] == 318611
        assert struct.unpack_from('<I', payload, 82)[0] == 64756
        records.append({'hotfix_id': hotfix, 'record_id': record, 'table_hash': table,
                        'payload_bytes': size, 'scale': struct.unpack_from('<f', payload, 58)[0],
                        'kit': 64756, 'payload_sha256': hashlib.sha256(payload).hexdigest()})
    offset = end
assert records, 'Selected attachment not cached; absence does not prove failed delivery'
print(json.dumps({'build': 26972, 'version': 5, 'cache_sha256': hashlib.sha256(data).hexdigest(),
                  'records': records}, indent=2))
