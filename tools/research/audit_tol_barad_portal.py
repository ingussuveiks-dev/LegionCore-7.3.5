"""Check the actual portal M2 tracks and native 7.3.5 animation kit.

The CASC asset is read-only and is not committed. This verifies references and
animation topology, not the client's rendering or state-animation priority.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
sys.dont_write_bytecode = True
from audit_spell_mechanics import Data

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--model', type=Path, required=True, help='CASC FileDataID 165828')
p.add_argument('--output', type=Path, required=True)
args = p.parse_args()
d = Data(Path('build-extractors/bin/Release/dbc/enUS'), Path('__native_only__'))
display = d.rows('GameObjectDisplayInfo')[8834]
assert display['FileDataID'] == 165828
raw = args.model.read_bytes()
model = raw[8:] if raw[:4] == b'MD21' else raw
assert model[:4] == b'MD20'
length, offset = struct.unpack_from('<II', model, 8)
name = model[offset:offset+length].rstrip(b'\0').decode()
assert name == 'Creature_SpellPortal_LargeShadow'
count, offset = struct.unpack_from('<II', model, 28)
sequences = [dict(zip(('id','variation','duration_ms','movement_speed','flags','frequency'),
                     struct.unpack_from('<HHIfIh', model, offset+i*64)))
             for i in range(count)]
assert [(r['id'],r['duration_ms']) for r in sequences] == [(0,1667),(158,3334),(159,633)]

def keys(track, sequence):
    interpolation, global_sequence, nt, ot, nk, ok = struct.unpack_from('<hhIIII', model, track)
    assert global_sequence == -1, 'Expected per-animation alpha track'
    if sequence >= nk:
        return []
    n, off = struct.unpack_from('<II', model, ok+sequence*8)
    return list(struct.unpack_from('<'+'h'*n, model, off)) if n else []

# The visible opening flare goes 0 -> full -> 0 in Stand and is disabled
# throughout Hold. Decay fades the stable portal mesh to zero instead.
ncolors, colors = struct.unpack_from('<II', model, 72)
assert ncolors == 20
opening = []
for i in range(ncolors):
    track = colors+i*40+20
    stand, hold, decay = (keys(track,s) for s in range(3))
    if stand and stand[0] == 0 and stand[-1] == 0 and max(stand) == 32767:
        assert hold and max(hold) == 0
        opening.append(i)
assert opening == list(range(5,20))
assert keys(colors+20,2) == [32767,0]
kit = d.rows('AnimKit')[2133]
segments = sorted((r for r in d.rows('AnimKitSegment').values() if r['ParentAnimKitID']==2133),
                  key=lambda r:r['OrderIndex'])
assert len(segments) == 2 and kit['OneShotDuration'] == 0
assert [(r['AnimID'],r['LoopToSegmentIndex']) for r in segments] == [(0,255),(158,1)]
assert segments[1]['StartCondition'] == 2 and segments[1]['StartConditionParam'] == 0
assert all(r['AnimStartTime']==0 and r['Speed']==1 and r['ForcedVariation']==0 for r in segments)
assert d.rows('AnimKitConfig')[341]['ConfigFlags'] == 256

result = {'build':'7.3.5.26972','gameobject':251699,'display':8834,'file_data_id':165828,
          'model':name,'model_sha256':hashlib.sha256(raw).hexdigest(),
          'sequences':sequences,'opening_flare_color_tracks':opening,
          'opening_flare_alpha_stand':keys(colors+5*40+20,0),
          'opening_flare_alpha_hold':keys(colors+5*40+20,1),
          'decay_mesh_alpha':keys(colors+20,2),'kit':kit,'segments':segments,
          'source_sha256':d.hashes,'result':'PASS','client_rendering_verified':False,
          'limits':'Native M2/DB2 evidence; actual first appearance, sustained display, use, re-entry and a second player require client verification.'}
args.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('PASS: actual portal opening-flare tracks; native kit opens once then loops Hold only')
