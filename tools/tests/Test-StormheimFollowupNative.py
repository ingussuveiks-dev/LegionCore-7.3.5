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

QUESTS = {38058, 38059, 38360}
OBJECTS = {241031, 241032}
SPELLS = {180645, 180860, 180737, 182313, 180677, 183386, 183454}
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
objectives = {int(row['ID']): row for row in records['quest_objectives']}
assert objectives[277453]['ObjectID'] == '90866' and objectives[277457]['Flags'] == '12'
assert objectives[277455]['ObjectID'] == '90948' and objectives[277455]['Amount'] == '6'
assert [(objectives[i]['ObjectID'], objectives[i]['Flags']) for i in (277870,277871,277872)] == [('241031','0'),('91904','2'),('241032','2')]
assert all(int(quests[q]['Flags']) & 65536 for q in (38058,38059))
data = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/stormheim-no-overlays')
native = {name:[row for row in data.rows(name).values() if row.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
effects = native['SpellEffect']
for spell, index, effect, aura, amount, target in ((180737,0,3,0,0,25),(180737,1,30,0,1,1),(183386,1,30,0,4,104),(182313,0,134,0,0,1)):
    assert any((r['SpellID'],r['EffectIndex'],r['Effect'],r['EffectAura'],r['EffectBasePoints'],r['ImplicitTarget1']) == (spell,index,effect,aura,amount,target) for r in effects)
assert any(r['SpellID']==180645 and r['EffectAura']==346 and r['EffectMiscValue1']==329 for r in effects)
assert all(any(r['SpellID']==spell and r['EffectIndex']==1 and r['Effect']==90 and r['EffectMiscValue1']==90903 for r in effects) for spell in (180677,183454))
native['UnitPowerBar']=[data.rows('UnitPowerBar')[329]]
assert native['UnitPowerBar'][0]['MaxPower']==100
native['AnimKit']=[data.rows('AnimKit')[6050]]
native['AnimKitSegment']=[r for r in data.rows('AnimKitSegment').values() if r['ParentAnimKitID']==6050]
assert native['AnimKitSegment']
native['Lock']=[data.rows('Lock')[1690]]
native['SpellTargetRestrictions']=[r for r in data.rows('SpellTargetRestrictions').values() if r['SpellID'] in SPELLS]
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
used_visuals = {45944} | {row['SpellVisualID'] for row in native['SpellXSpellVisual'] if row['SpellVisualID']}
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
out = {'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
       'quests':[{key:row[key] for key in ('ID','LogTitle','Flags','RewardNextQuest','QuestCompletionLog')} for row in records['quest_template']],
       'objectives':records['quest_objectives'],'objects':records['gameobject_template'],'enders':records.get('creature_questender',[]),
       'native':native,'visual_trace':trace,'runtime_hashes':data.hashes,
       'limits':'Native records identify real marker, rescue quantity, ordered clues, alternate bar, native spell targets and actor visual/animation dependencies. No client playback or pet combat is verified.'}
path=ROOT/'docs/audits/stormheim-followup-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: native three quests/next routes, marker 90866, six releases, ordered clues, actual clicked/self targets, electricity/energize quantities, alternate bar and dragoon pose/visual/animation dependencies.')
