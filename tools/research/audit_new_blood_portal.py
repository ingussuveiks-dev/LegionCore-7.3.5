"""Validate ritual assets against native 7.3.5 DB2 and an exported CASC M2."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data

p = argparse.ArgumentParser()
p.add_argument('--model', type=Path, required=True, help='CASC FileDataID 607277')
p.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
args = p.parse_args()
d = Data(args.dbc, Path('__native_only__'))
display = d.rows('CreatureDisplayInfo')[68219]
assert d.rows('CreatureModelData')[display['ModelID']]['FileDataID'] == 607277
raw = args.model.read_bytes()
model = raw[8:] if raw[:4] == b'MD21' else raw
assert model[:4] == b'MD20'
length, offset = struct.unpack_from('<II', model, 8)
name = model[offset:offset + length].rstrip(b'\0').decode()
assert name == 'Warlock_DemonicPortal_Green'
count, offset = struct.unpack_from('<II', model, 28)
sequences = [struct.unpack_from('<HHI', model, offset + i * 64) for i in range(count)]
assert (127, 0, 13000) in sequences
segments = [r for r in d.rows('AnimKitSegment').values() if r['ParentAnimKitID'] == 1254]
assert len(segments) == 1 and segments[0]['AnimID'] == 127 and segments[0]['Speed'] == 1
assert segments[0]['AnimStartTime'] == 0
effects = d.rows('SpellEffect').values()
assert any(r['SpellID'] == 200011 and r['Effect'] == 98 and r['EffectTriggerSpell'] == 204833 for r in effects)
assert any(r['SpellID'] == 204833 and r['EffectAura'] == 12 and r['ImplicitTarget1'] == 1 for r in effects)
assert any(r['SpellID'] == 199994 and r['SpellVisualID'] == 51954 for r in d.rows('SpellXSpellVisual').values())
print(json.dumps({'result': 'PASS', 'model': name, 'fileDataID': 607277,
                  'modelSHA256': hashlib.sha256(raw).hexdigest(), 'birthMs': 13000,
                  'birthAnimKit': 1254, 'sequences': sequences,
                  'knockback': 200011, 'nativeStun': 204833,
                  'portalMalfunction': 199994, 'clientRenderingVerified': False}))
