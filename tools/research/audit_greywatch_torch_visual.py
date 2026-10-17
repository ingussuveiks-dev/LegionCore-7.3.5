"""Read native WDC1 visual dependencies for the Greywatch torch, without edits.

SpellVisual/SpellVisualEvent have disabled/incomplete C++ loaders in this core.
Decode their checked layout and relationship sections, not a guessed fixed row.
"""
import argparse
import json
import re
import struct
import hashlib
from pathlib import Path
from audit_spell_visuals import Table

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
p.add_argument('--output', type=Path, required=True)
p.add_argument('--model-directory', type=Path, help='Read-only CASC exports named FileXXXXXXXX.bin, including helper1112735')
args = p.parse_args()
meta = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
hashes = {}

def records(name, fields):
    block = meta.split('struct ' + name + 'Meta\n', 1)[1].split('\n};', 1)[0]
    index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
    t = Table(args.dbc / (name + '.db2'), int(index), int(layout, 16))
    hashes[name] = t.sha256
    rows = {t.ids[i]: dict(ID=t.ids[i], **{k: t.value(i, f) for k, f in fields.items()}) for i in range(t.count)}
    for i, parent in t.parents.items():
        rows[t.ids[i]]['parent'] = parent
    for dest, source in t.copy_pairs:
        rows[dest] = dict(rows[source], ID=dest)
    return rows

links = [r for r in records('SpellXSpellVisual', {'visual': 0}).values() if r.get('parent') in (186745, 186746, 186747)]
assert [(r['parent'], r['visual']) for r in links] == [(186746, 48032), (186747, 48033)]
visuals = [r for r in records('SpellVisual', {'missile_set': 3, 'destination_attachment': 4, 'cast_attachment': 5}).values() if r['ID'] in (48032, 48033)]
events = [r for r in records('SpellVisualEvent', {'start': 0, 'end': 3, 'target': 6, 'kit': 7}).values() if r.get('parent') in (48032, 48033)]
kit_ids = {r['kit'] for r in events}
effects = [r for r in records('SpellVisualKitEffect', {'type': 0, 'effect': 1}).values() if r.get('parent') in kit_ids]
attach_ids = {r['effect'] for r in effects if r['type'] == 2}
attachments = [r for r in records('SpellVisualKitModelAttach', {'name': 3, 'attachment': 4, 'low': 19}).values() if r['ID'] in attach_ids]
name_ids = {r['name'] for r in attachments}
missile_ids = {r['missile_set'] & 0xffff for r in visuals} - {0}
missiles = [r for r in records('SpellVisualMissile', {'name': 5}).values() if r.get('parent') in missile_ids]
name_ids.update(r['name'] & 0xffff for r in missiles)
names = [r for r in records('SpellVisualEffectName', {'model': 10, 'texture': 8}).values() if r['ID'] in name_ids]
assert any(r['parent'] == 48033 for r in events) and attachments and missiles and names
assert name_ids <= {r['ID'] for r in names}
result = dict(links=links, visuals=visuals, events=events, kit_effects=effects, attachments=attachments, missiles=missiles, effect_names=names,
              sha256=hashes, boundary='Native visual references only; CASC model contents and actual client rendering must be checked separately.')
if args.model_directory:
    models = []
    for fid in sorted({r['model'] for r in names if r['model']} | {1112735}):
        raw = (args.model_directory / f'File{fid:08X}.bin').read_bytes()
        data = raw[8:] if raw[:4] == b'MD21' else raw
        assert data[:4] == b'MD20'
        n, offset = struct.unpack_from('<II', data, 8)
        name = data[offset:offset+n].rstrip(b'\0').decode()
        n, offset = struct.unpack_from('<II', data, 28)
        animations = [struct.unpack_from('<HHI', data, offset+i*64) for i in range(n)]
        n, offset = struct.unpack_from('<II', data, 0xf0)
        sockets = [struct.unpack_from('<IHHfff', data, offset+i*40) for i in range(n)]
        models.append(dict(file=fid, name=name, sha256=hashlib.sha256(raw).hexdigest(), animations=animations, attachments=sockets))
        if fid == 1112735:
            assert name == 'Rabbit2' and any(s[0] == 19 and s[3:] == (0, 0, 0) for s in sockets)
    assert {m['name'] for m in models} == {'TorchSpell', '6FX_Fire_SmolderingObject_Doodad', '6FX_TorchFire_Doodad', 'Rabbit2'}
    result['models'] = models
    result['boundary'] = 'CASC model names, animation records and helper ground attachment19 checked; actual timing, fire scale/placement and rendering remain client checks.'
args.output.write_text(json.dumps(result, indent=2) + '\n')
print('PASS: native torch186746/visual48032 missile and burn186747/visual48033 event/kit/attachment/effect-name references; no direct745 visual.')
print('Referenced model files:', sorted({r['model'] for r in names if r['model']}))
