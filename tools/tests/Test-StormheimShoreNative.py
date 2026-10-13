"""Read original TDB735 quests/objects and installed native 7.3.5 spell data."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table

QUESTS = {38036, 38052, 38053, 38057, 38332, 38357, 38358}
OBJECTS = {240600, 249829, 241275}
SPELLS = {181970, 181981, 182021, 182144, 182151}
archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template', 'quest_objectives', 'creature_questender', 'gameobject_template'}
columns, records, table = {}, {}, None
for line in archive.open(encoding='utf8'):
    match = re.match(r'CREATE TABLE `([^`]+)`', line)
    if match:
        table = match[1] if match[1] in selected else None
        columns[table] = []
    elif table:
        if line.startswith(')'):
            table = None
        else:
            match = re.match(r'  `([^`]+)`', line)
            if match:
                columns[table].append(match[1])
    match = re.match(r'INSERT INTO `([^`]+)`', line)
    if not match or match[1] not in selected:
        continue
    name = match[1]
    for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        row = dict(zip(columns[name], re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+", match[0][1:-1])))
        key = int(row.get('QuestID', row.get('quest', row.get('ID', row.get('entry', 0)))))
        if key in (OBJECTS if name == 'gameobject_template' else QUESTS):
            records.setdefault(name, []).append(row)

quests = {int(row['ID']): row for row in records['quest_template']}
assert set(quests) == QUESTS
objectives = records['quest_objectives']
assert not any(int(row['QuestID']) in (38057, 38332) for row in objectives)
waste = next(row for row in objectives if row['ID'] == '277818')
assert (waste['QuestID'], waste['Type'], waste['ObjectID'], waste['Amount'], waste['Flags']) == ('38358', '0', '91693', '100', '12')
filled = next(row for row in objectives if row['ID'] == '277817')
assert (filled['ObjectID'], filled['Amount'], filled['Flags']) == ('91590', '1', '0')
assert quests[38358]['StartItem'] == '122702'
assert all(quests[q]['RewardNextQuest'] == str(n) for q, n in ((38053, 38058), (38057, 38059), (38332, 38360)))
assert all(row['Data0'] == '2382' and row['Data10'] == '181981' for row in records['gameobject_template'] if int(row['entry']) in (240600, 249829))

data = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/stormheim-no-overlays')
native = {name: [row for row in data.rows(name).values() if row.get('SpellID') in SPELLS]
          for name in ('SpellEffect', 'SpellMisc', 'SpellXSpellVisual')}
effects = native['SpellEffect']
pump = next(row for row in effects if row['SpellID'] == 181981 and row['EffectIndex'] == 0)
assert (pump['Effect'], pump['EffectMiscValue1'], pump['EffectBasePoints'], pump['EffectDieSides'], pump['ImplicitTarget1']) == (90, 91693, 14, 6, 1)
assert any(row['SpellID'] == 182021 and row['Effect'] == 33 and row['ImplicitTarget1'] == 23 for row in effects)
assert any(row['SpellID'] == 181970 and row['EffectAura'] == 346 and row['EffectMiscValue1'] == 335 for row in effects)
assert any(row['SpellID'] == 181970 and row['EffectAura'] == 226 and row['EffectAuraPeriod'] == 1000 for row in effects)
assert any(row['SpellID'] == 182151 and row['EffectAura'] == 4 for row in effects)
assert next(row for row in native['SpellMisc'] if row['SpellID'] == 181981)['CastingTimeIndex'] == 1
bar = data.rows('UnitPowerBar')[335]
assert bar['MaxPower'] == 100 and bar['StartPower'] == 0
native['UnitPowerBar'] = [bar]
times = data.rows('SpellCastTimes')
assert times[1]['Base'] == 0 and times[5]['Base'] > 0
native['SpellCastTimes'] = [times[1], times[5]]
native['Lock'] = [data.rows('Lock')[1691], data.rows('Lock')[2382]]
assert any(native['Lock'][1][f'Index{i}'] == 182021 for i in range(1, 9))
native['ItemEffect'] = [row for row in data.rows('ItemEffect').values() if row['ItemID'] == 122702]
assert not native['ItemEffect']  # Native quest item is not an assumed usable Pump spell.
# Several visual-table load-info field orders differ from the actual 26972
# layout. Use the reviewed native column indices rather than naming fields
# by those load-info structures (see audit_character_spell_chains.py).
def visual_records(name, fields):
    block = data.meta.split('struct ' + name + 'Meta\n', 1)[1].split('\n};', 1)[0]
    index, layout = re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)', block).groups()
    table = Table(data.dbc / (name + '.db2'), int(index), int(layout, 16))
    data.hashes[name] = table.sha256
    rows = {table.ids[i]: dict(ID=table.ids[i], **{field: table.value(i, column) for field, column in fields.items()})
            for i in range(table.count)}
    for i, parent in table.parents.items():
        rows[table.ids[i]]['parent'] = parent
    for dest, source in table.copy_pairs:
        rows[dest] = dict(rows[source], ID=dest)
    return rows

visuals = visual_records('SpellVisual', {'targeting_kit': 8, 'hostile': 11, 'caster': 12, 'low_violence': 13})
events = visual_records('SpellVisualEvent', {'kit': 7})
kits = visual_records('SpellVisualKit', {'fallback': 2})
kit_effects = visual_records('SpellVisualKitEffect', {'type': 0, 'effect': 1})
attachments = visual_records('SpellVisualKitModelAttach', {'effect_name': 3, 'attachment': 4, 'start_anim': 15,
    'loop_anim': 16, 'end_anim': 17, 'anim_kit': 18, 'low_def': 19})
effect_names = visual_records('SpellVisualEffectName', {'model_file_id': 10, 'texture_file_id': 8})
used_visuals = {row['SpellVisualID'] for row in native['SpellXSpellVisual'] if row['SpellVisualID']}
while True:
    more = {visuals[v][key] for v in used_visuals for key in ('hostile', 'caster', 'low_violence') if visuals[v][key]}
    if more <= used_visuals:
        break
    used_visuals |= more
used_events = [row for row in events.values() if row.get('parent') in used_visuals]
used_kits = {row['kit'] for row in used_events if row['kit']} | {visuals[v]['targeting_kit'] for v in used_visuals if visuals[v]['targeting_kit']}
while True:
    more = {kits[k]['fallback'] for k in used_kits if kits[k]['fallback']}
    if more <= used_kits:
        break
    used_kits |= more
used_effects = [row for row in kit_effects.values() if row.get('parent') in used_kits]
used_attachments = {row['effect'] for row in used_effects if row['type'] == 2 and row['effect']}
while True:
    more = {attachments[a]['low_def'] for a in used_attachments if attachments[a]['low_def']}
    if more <= used_attachments:
        break
    used_attachments |= more
used_names = {attachments[a]['effect_name'] for a in used_attachments if attachments[a]['effect_name']}
assert used_events and used_attachments and used_names
trace = {'visuals': [visuals[v] for v in sorted(used_visuals)], 'events': used_events,
         'kits': [kits[k] for k in sorted(used_kits)], 'kit_effects': used_effects,
         'model_attachments': [attachments[a] for a in sorted(used_attachments)],
         'effect_names': [effect_names[n] for n in sorted(used_names)]}
out = {'archive': archive.name, 'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
       'quests': [{key: row[key] for key in ('ID', 'LogTitle', 'StartItem', 'RewardNextQuest', 'QuestCompletionLog')} for row in records['quest_template']],
       'objectives': objectives, 'enders': records.get('creature_questender', []),
       'objects': records['gameobject_template'], 'native': native, 'visual_trace': trace, 'runtime_hashes': data.hashes,
       'limits': 'Native records establish counters, lock/open spell, instant Pump quantity and visuals. They do not verify client animation/interaction or prove missing world loot links correct.'}
path = ROOT / 'docs/audits/stormheim-shore-native-2026-10-13.json'
if '--write-evidence' in sys.argv:
    path.write_text(json.dumps(out, indent=2) + '\n')
else:
    assert json.loads(json.dumps(out)) == json.loads(path.read_text())
print('PASS: native seven quests, optional/hidden waste counter, required filled marker, next quests, locked barrels, native open-lock/Pump effects, 15..20 quantity, alternate bar 0..100 and visual references.')
