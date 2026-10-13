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
# Model particle tracks, not a spell name, establish the burning animation.
# EnabledIn has no keys when enabled by default (WoWModelViewer particle.cpp).
particle_count, particle_offset = struct.unpack_from('<II', model, 296)
assert particle_count == 7
def enabled(emitter, sequence):
    block = particle_offset + emitter * 492 + 456
    _, _, _, _, keys_count, keys_offset = struct.unpack_from('<hhIIII', model, block)
    assert keys_count == len(sequences)
    n, off = struct.unpack_from('<II', model, keys_offset + sequence * 8)
    return list(model[off:off+n]) if n else [1]
burn_index = next(i for i,s in enumerate(sequences) if s[0] == 214)
assert enabled(4, 0) == [0] and enabled(4, burn_index) == [1]
texture_index = struct.unpack_from('<H', model, particle_offset + 4 * 492 + 22)[0]
texture_count, texture_offset = struct.unpack_from('<II', model, 80)
assert texture_index < texture_count
_, _, texture_length, texture_name_offset = struct.unpack_from('<IIII', model, texture_offset + texture_index * 16)
fire_texture = model[texture_name_offset:texture_name_offset+texture_length].rstrip(b'\0').decode()
assert fire_texture == r'Spells\Warlock_DemonicPortal_Fire_Greyscale.blp'
burn = [r for r in d.rows('AnimKitSegment').values() if r['ParentAnimKitID'] == 1244]
assert len(burn) == 1 and burn[0]['AnimID'] == 214 and burn[0]['LoopToSegmentIndex'] == 0
effects = d.rows('SpellEffect').values()
assert any(r['SpellID'] == 200011 and r['Effect'] == 98 and r['EffectTriggerSpell'] == 204833 for r in effects)
assert any(r['SpellID'] == 204833 and r['EffectAura'] == 12 and r['ImplicitTarget1'] == 1 for r in effects)
assert any(r['SpellID'] == 199994 and r['SpellVisualID'] == 51954 for r in d.rows('SpellXSpellVisual').values())
print(json.dumps({'result': 'PASS', 'model': name, 'fileDataID': 607277,
                  'modelSHA256': hashlib.sha256(raw).hexdigest(), 'birthMs': 13000,
                  'birthAnimKit': 1254, 'sequences': sequences,
                  'knockback': 200011, 'nativeStun': 204833,
                  'portalFireAnimKit': 1244, 'fireEmitter': 4,
                  'fireTexture': fire_texture,
                  'clientRenderingVerified': False}))
