"""Read native Dalaran Portal Stone visual dependencies, without claiming playback."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path
from audit_spell_visuals import Table

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--model-directory', type=Path)
args = parser.parse_args()
metadata = Path('src/server/game/DataStores/DB2Metadata.h').read_text()
hashes = {}


def records(name, fields, layout_override=None):
    block = metadata.split('struct ' + name + 'Meta\n', 1)[1].split('\n};', 1)[0]
    index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
    table = Table(args.dbc / (name + '.db2'), int(index), layout_override or int(layout, 16))
    hashes[name] = table.sha256
    rows = {table.ids[row]: dict(ID=table.ids[row], **{field: table.value(row, column) for field, column in fields.items()}) for row in range(table.count)}
    for row, parent in table.parents.items():
        rows[table.ids[row]]['parent'] = parent
    for destination, source in table.copy_pairs:
        rows[destination] = dict(rows[source], ID=destination)
    return rows


links = [row for row in records('SpellXSpellVisual', {'visual': 0}).values() if row.get('parent') == 205040]
assert links and {row['visual'] for row in links} == {38616}
events = [row for row in records('SpellVisualEvent', {'start': 0, 'start_min_ms': 1, 'start_max_ms': 2, 'end': 3, 'target': 6, 'kit': 7}).values() if row.get('parent') == 38616]
kit_ids = {row['kit'] for row in events}
all_kits = records('SpellVisualKit', {'flags': 0, 'fallback': 2})
pending = list(kit_ids)
while pending:
    kit = all_kits[pending.pop()]
    if kit['fallback'] and kit['fallback'] not in kit_ids:
        kit_ids.add(kit['fallback'])
        pending.append(kit['fallback'])
kits = [all_kits[identity] for identity in sorted(kit_ids)]
effects = [row for row in records('SpellVisualKitEffect', {'type': 0, 'effect': 1}).values() if row.get('parent') in kit_ids]
animation_ids = {row['effect'] for row in effects if row['type'] == 6}
animations = [row for row in records('SpellVisualAnim', {'initial': 0, 'loop': 1, 'anim_kit': 2}).values() if row['ID'] in animation_ids]
anim_kit_ids = {row['anim_kit'] for row in animations if row['anim_kit']}
anim_kits = [row for row in records('AnimKit', {'duration': 0, 'stop_kit': 1, 'low_def_kit': 2}).values() if row['ID'] in anim_kit_ids]
assert anim_kit_ids == {row['ID'] for row in anim_kits}
sound_ids = {row['effect'] for row in effects if row['type'] == 5}
sounds = [row for row in records('SoundKit', {}).values() if row['ID'] in sound_ids]
assert animation_ids == {row['ID'] for row in animations}
assert sound_ids == {row['ID'] for row in sounds}
attach_ids = {row['effect'] for row in effects if row['type'] == 2}
attachments = [row for row in records('SpellVisualKitModelAttach', {'name': 3, 'attachment': 4, 'start_anim': 15, 'anim': 16, 'end_anim': 17, 'anim_kit': 18}).values() if row['ID'] in attach_ids]
name_ids = {row['name'] for row in attachments}
names = [row for row in records('SpellVisualEffectName', {'model': 10, 'texture': 8}, 0xB930A934).values() if row['ID'] in name_ids]
assert events and effects and attach_ids <= {row['ID'] for row in attachments}
assert name_ids <= {row['ID'] for row in names}
result = dict(build='7.3.5.26972', spell=205040, visual=38616, links=links, events=events, kits=kits,
              kit_effects=effects, visual_animations=animations, animation_kits=anim_kits, sound_kits=sounds, attachments=attachments, effect_names=names,
              referenced_file_ids=sorted({row[field] for row in names for field in ('model', 'texture') if row[field]}),
              source_sha256=hashes, rendering_verified=False,
              boundary='Native WDC1 relationships/kit fallbacks and known26972 EffectName B930A934 columns, not the otherwise-unused later core schema. Numeric event/target/animation and non-model effect types retained without invented semantics. Client pose, color, audio and exact timing require playback.')
if args.model_directory:
    models = []
    for identity in result['referenced_file_ids']:
        raw = (args.model_directory / f'File{identity:08X}').read_bytes()
        model = raw[8:] if raw[:4] == b'MD21' else raw
        assert model[:4] == b'MD20'
        count, offset = struct.unpack_from('<II', model, 8)
        name = model[offset:offset+count].rstrip(b'\0').decode()
        count, offset = struct.unpack_from('<II', model, 28)
        model_animations = [struct.unpack_from('<HHI', model, offset+index*64) for index in range(count)]
        models.append(dict(file=identity, name=name, animations=model_animations, sha256=hashlib.sha256(raw).hexdigest()))
    result['models'] = models
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(result, indent=2) + '\n')
print('PASS: native205040 -> visual38616 -> events/kits/attachments/effect names; rendering unverified.')
print('Referenced files:', result['referenced_file_ids'])
