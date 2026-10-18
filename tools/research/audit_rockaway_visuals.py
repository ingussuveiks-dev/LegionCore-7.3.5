"""Read checked WDC1 visual relationships; optionally inspect exported native M2s.

SpellVisualEvent has an implicit parent omitted by the core's C++ loader.
Use the existing layout/compression/relationship reader, never fixed row offsets.
This proves asset references, not a retail actor binding or client playback.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
from audit_spell_visuals import Table

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
p.add_argument('--model-directory', type=Path)
p.add_argument('--output', type=Path, required=True)
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
    return list(rows.values())


links = [r for r in records('SpellXSpellVisual', {'visual': 0}) if r.get('parent') in (231328, 231333, 231298, 231354)]
assert {(r['parent'], r['visual']) for r in links} == {(231328, 62809), (231333, 62810), (231298, 62804), (231354, 62823)}
visual_ids = {r['visual'] for r in links}
visuals = [r for r in records('SpellVisual', {'missile_set': 3, 'destination_attachment': 4, 'cast_attachment': 5}) if r['ID'] in visual_ids]
events = [r for r in records('SpellVisualEvent', {'start': 0, 'end': 3, 'target': 6, 'kit': 7}) if r.get('parent') in visual_ids]
kits = {r['kit'] for r in events}
effects = [r for r in records('SpellVisualKitEffect', {'type': 0, 'effect': 1}) if r.get('parent') in kits]
attach_ids = {r['effect'] for r in effects if r['type'] == 2}
attachments = [r for r in records('SpellVisualKitModelAttach', {'name': 3, 'attachment': 4, 'start_anim': 15, 'anim': 16, 'end_anim': 17, 'animkit': 18}) if r['ID'] in attach_ids]
name_ids = {r['name'] for r in attachments}
missile_sets = {r['missile_set'] & 0xffff for r in visuals} - {0}
missiles = [r for r in records('SpellVisualMissile', {'name': 5}) if r.get('parent') in missile_sets]
name_ids.update(r['name'] & 0xffff for r in missiles)
names = [r for r in records('SpellVisualEffectName', {'model': 10}) if r['ID'] in name_ids]
assert attach_ids == {r['ID'] for r in attachments} and name_ids == {r['ID'] for r in names}
assert {r['model'] for r in names} == {166811, 201106, 166806, 953296, 1302948, 1036812}
result = dict(links=links, visuals=visuals, missiles=missiles, events=events, kit_effects=effects, attachments=attachments, effect_names=names, db2_sha256=hashes)
if args.model_directory:
    models = []
    files = {r['model'] for r in names} | {1260044, 1266228, 1267366, 390813}
    for file_id in sorted(files):
        raw = (args.model_directory / f'File{file_id:08X}').read_bytes()
        data = raw[8:] if raw[:4] == b'MD21' else raw
        assert data[:4] == b'MD20'
        count, offset = struct.unpack_from('<II', data, 8)
        name = data[offset:offset+count].rstrip(b'\0').decode()
        count, offset = struct.unpack_from('<II', data, 28)
        animations = [struct.unpack_from('<HHI', data, offset+i*64) for i in range(count)]
        count, offset = struct.unpack_from('<II', data, 0xf0)
        sockets = [struct.unpack_from('<IHHfff', data, offset+i*40) for i in range(count)]
        models.append(dict(file=file_id, name=name, animations=animations, attachments=sockets, sha256=hashlib.sha256(raw).hexdigest()))
    result['models'] = models
result['boundary'] = ('Native aura/impact kit, attachment, animation and model records only. Ritual auras include Shadow hand/rune assets; '
                      'their actor assignment and the observed green channel beam are NOT established by matching spell names. '
                      'Kit-effect types other than model attachment2 remain opaque here. First appearance, beam target, projection, '
                      'slam direction, missiles and timing require the client; no whole visual is marked verified.')
args.output.write_text(json.dumps(result, indent=2) + '\n')
print('PASS: checked native visual relationships and optional M2 metadata; actor assignment/beam/client playback remain pending.')
