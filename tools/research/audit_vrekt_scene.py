"""Read-only Vrekt client visual links; not a retail placement or rendering test.

The server DB2 loader does not support all client-only SpellVisualEvent fields.
Use the regular WDC1 reader with the repository's exact metadata layout here;
the companion C++ catalog executes native spell/target/animation/model reads.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
sys.dont_write_bytecode = True
from audit_spell_visuals import Table
from audit_spell_mechanics import Data

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--model', type=Path, help='Read-only CASC FileDataID1280971')
p.add_argument('--output', type=Path, required=True)
args = p.parse_args()
dbc = Path('build-extractors/bin/Release/dbc/enUS')
metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
hashes = {}

def records(name, fields):
    block = metadata.split('struct '+name+'Meta\n', 1)[1].split('\n};', 1)[0]
    index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
    table = Table(dbc/(name+'.db2'), int(index), int(layout, 16))
    hashes[name] = table.sha256
    result = {table.ids[i]: dict(ID=table.ids[i], **{key: table.value(i, col) for key, col in fields.items()}) for i in range(table.count)}
    for i, parent in table.parents.items():
        result[table.ids[i]]['parent'] = parent
    for dest, source in table.copy_pairs:
        result[dest] = dict(result[source], ID=dest)
    return result

links = records('SpellXSpellVisual', {'visual': 0})
for spell, visual in [(211409, 55619), (230506, 62640), (230352, 11919)]:
    assert [r['visual'] for r in links.values() if r.get('parent') == spell] == [visual]
events = records('SpellVisualEvent', {'start': 0, 'end': 3, 'target': 6, 'kit': 7})
for event, visual, kit in [(119515, 55619, 18459), (141301, 62640, 77615), (19018, 11919, 11590)]:
    assert events[event]['parent'] == visual and events[event]['kit'] == kit
effects = records('SpellVisualKitEffect', {'type': 0, 'effect': 1})
for effect, kit, kind, value in [(30633, 18459, 1, 2384), (14443, 11590, 1, 1216), (173753, 77615, 6, 30728)]:
    assert effects[effect] == dict(ID=effect, type=kind, effect=value, parent=kit)
d = Data(dbc, Path('__native_only__'))
procedural = d.rows('SpellProceduralEffect')
assert all(procedural[i]['Type'] == 26 and procedural[i]['Value1'] == 458 for i in [1216, 2384])
iron = d.rows('SpellChainEffects')[458]
assert iron['TextureFileDataID1'] == 167097
anim = d.rows('SpellVisualAnim')[30728]
assert anim['AnimKitID'] == 12209
result = dict(quest=44923, chain_spell=211409, captured_pose=230506,
              chain_procedural_ids=[2384, 1216], chain_effect=458,
              chain_texture=167097, chain_texture_name='Beam_ChainIron.blp',
              animation_kit=12209, model_file=1280971,
              hashes=dict(hashes, **d.hashes), client_verified=False,
              boundary='Client-only WDC1 references, not native server loader execution or visual rendering. Placement, rotation replacement and escape spline are reconstructed.')
if args.model:
    raw = args.model.read_bytes()
    model = raw[8:] if raw[:4] == b'MD21' else raw
    assert model[:4] == b'MD20'
    count, offset = struct.unpack_from('<II', model, 8)
    assert model[offset:offset+count].rstrip(b'\0') == b'stormdragon'
    count, offset = struct.unpack_from('<II', model, 28)
    seq = [struct.unpack_from('<HHI', model, offset+i*64) for i in range(count)]
    assert (0, 2, 5333) in seq
    result['model_sha256'] = hashlib.sha256(raw).hexdigest()
    result['stand_variation2_duration_ms'] = 5333
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(result, indent=2)+'\n')
print('PASS: WDC1 visual events, procedural iron chain, captured-pose kit and optional actual stormdragon Stand variation2. Rendering and retail placement/spline remain unverified.')
