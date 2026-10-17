"""Read native Communing visual dependencies; does not claim client playback."""
import argparse
import json
import re
from pathlib import Path
from audit_spell_visuals import Table

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--dbc', type=Path, default=Path('build-extractors/bin/Release/dbc/enUS'))
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
    return rows


links = [r for r in records('SpellXSpellVisual', {'visual': 0}).values() if r.get('parent') == 186982]
assert links and {r['visual'] for r in links} == {41037}
visuals = [r for r in records('SpellVisual', {'missile_set': 3, 'destination_attachment': 4, 'cast_attachment': 5}).values() if r['ID'] == 41037]
events = [r for r in records('SpellVisualEvent', {'start': 0, 'end': 3, 'target': 6, 'kit': 7}).values() if r.get('parent') == 41037]
kit_ids = {r['kit'] for r in events}
all_kits = records('SpellVisualKit', {'flags': 0, 'fallback': 2})
pending = list(kit_ids)
while pending:
    kit = all_kits[pending.pop()]
    if kit['fallback'] and kit['fallback'] not in kit_ids:
        kit_ids.add(kit['fallback'])
        pending.append(kit['fallback'])
kits = [all_kits[i] for i in sorted(kit_ids)]
effects = [r for r in records('SpellVisualKitEffect', {'type': 0, 'effect': 1}).values() if r.get('parent') in kit_ids]
attach_ids = {r['effect'] for r in effects if r['type'] == 2}
attachments = [r for r in records('SpellVisualKitModelAttach', {'name': 3, 'attachment': 4}).values() if r['ID'] in attach_ids]
name_ids = {r['name'] for r in attachments}
names = [r for r in records('SpellVisualEffectName', {'model': 10, 'texture': 8}).values() if r['ID'] in name_ids]
assert visuals and events and effects and attach_ids <= {r['ID'] for r in attachments}
assert name_ids <= {r['ID'] for r in names}
result = dict(spell=186982, links=links, visuals=visuals, events=events, kits=kits, kit_effects=effects,
              attachments=attachments, effect_names=names, sha256=hashes,
              boundary='Checked native WDC1 layouts/relationships, kit fallbacks and attachment references. Non-model kit effects retain their numeric types/IDs; no interpretation or playback claim. Animation, sound and timing require the 7.3.5 client; no invented Ascendant transformation.')
args.output.write_text(json.dumps(result, indent=2) + '\n')
print('PASS: Communing186982 -> visual41037 -> native events/kits/attachments/effect names.')
print('Referenced model files:', sorted({r['model'] for r in names if r['model']}))
