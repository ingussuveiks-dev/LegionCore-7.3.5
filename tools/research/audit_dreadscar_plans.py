"""Check the native scouting-board asset; rendering still requires a client."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--model', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
args = p.parse_args()
d = Data(Path('build-extractors/bin/Release/dbc/enUS'), Path('__native_only__'))
display = d.rows('CreatureDisplayInfo')[71039]
model = d.rows('CreatureModelData')[display['ModelID']]
assert display['CreatureModelAlpha'] == 255 and model['FileDataID'] == 1308068
raw = args.model.read_bytes()
b = raw[8:] if raw[:4] == b'MD21' else raw
assert b[:4] == b'MD20'
n, off = struct.unpack_from('<II', b, 8)
name = b[off:off+n].rstrip(b'\0').decode()
assert name == '7AF_Warlock_MissionBoard01'
n, off = struct.unpack_from('<II', b, 28)
assert n == 1 and struct.unpack_from('<HHI', b, off) == (0, 0, 3333)
n, off = struct.unpack_from('<II', b, 44)
assert n == 1
# Neither the root's translation nor its scale has animated keys. There is
# no native sinking animation to substitute for the broken movement setup.
assert struct.unpack_from('<hhIIII', b, off+16)[2:] == (0, 0, 0, 0)
assert struct.unpack_from('<hhIIII', b, off+56)[2:] == (0, 0, 0, 0)
result = dict(build='7.3.5.26972', display=display, model=model, model_name=name,
              model_sha256=hashlib.sha256(raw).hexdigest(), source_sha256=d.hashes,
              static_root=True, client_rendering_verified=False)
args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
print('PASS: native opaque scouting-board model, static root, no sinking animation')
