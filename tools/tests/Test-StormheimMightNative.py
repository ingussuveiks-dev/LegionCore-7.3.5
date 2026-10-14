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

QUESTS = {39594,39597}
OBJECTS = {243817}
SPELLS = {190763,190773,68442}

archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template', 'quest_objectives', 'creature_questender', 'gameobject_template', 'quest_template_addon', 'gameobject_queststarter', 'gameobject_questender'}
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

quests={int(row['ID']):row for row in records['quest_template']}
objectives={int(row['ID']):row for row in records['quest_objectives']}
assert [(objectives[i]['Type'],objectives[i]['ObjectID'],objectives[i]['StorageIndex'],objectives[i]['Amount'],objectives[i]['Flags']) for i in (279739,279732)]==[('0','96290','0','1','0'),('0','96175','5','1','2')]
assert quests[39594]['RewardNextQuest']=='39597'
objects={int(row['entry']):row for row in records['gameobject_template']}
assert objects[243817]['Data0']=='1691' and objects[243817]['Data1']=='39594' and objects[243817]['Data2']=='0'
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[r for r in data.rows(name).values() if r.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions','SpellAuraOptions')}
assert [(r['SpellID'],r['Effect'],r['EffectAura']) for r in native['SpellEffect']]==[(68442,6,12),(190763,6,4),(190773,3,0)]
assert not any(r['SpellID'] in (190763,190773) and r['Effect']==28 for r in native['SpellEffect'])
native['Lock']=[data.rows('Lock')[1691]]
native['CreatureDisplayInfo']=[data.rows('CreatureDisplayInfo')[25217]]
native['CreatureModelData']=[data.rows('CreatureModelData')[native['CreatureDisplayInfo'][0]['ModelID']]]
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
displays=visual_records('GameObjectDisplayInfo',{'model_file_id':0})
native['GameObjectDisplayInfo']=[displays[int(objects[i]['displayId'])] for i in sorted(objects)]
native['GameObjectBounds']=[data.rows('GameObjectDisplayInfo')[int(objects[243817]['displayId'])]]
assert all(r['model_file_id']>0 for r in native['GameObjectDisplayInfo'])
# Resolve names including native copy records, but do not use names as visual evidence.
block=data.meta.split('struct SpellMeta\n')[1].split('\n};')[0]
index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[0-9A-Fa-f]+)',block).groups()
t=Table(data.dbc/'Spell.db2',int(index),int(layout,16));names={t.ids[i]:t.name(i) for i in range(t.count)}
for dest,source in t.copy_pairs:names[dest]=names[source]
data.hashes['Spell']=t.sha256
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
 'quests':[{k:row[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for row in records['quest_template']],
 'objectives':records['quest_objectives'],'addons':records.get('quest_template_addon',[]),
 'objects':records['gameobject_template'],'go_starters':records.get('gameobject_queststarter',[]),'go_enders':records.get('gameobject_questender',[]),
 'spell_names':{i:names[i] for i in sorted(SPELLS)},'native':native,'visual_trace':trace,'runtime_hashes':data.hashes,
 'limits':'Native spell/effect/visual/kit/attachment/model definitions and quest slots are data evidence. Client interaction, narration, encounter combat, projection placement/animation, watcher light arrangement, audio and visuals remain unverified.'}
path=ROOT/'docs/audits/stormheim-might-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original two required Might objectives/slots and sequenced flag, archive lock/event, real power/activation/kneel effects, Yotnar model, visual kits/attachments/animations/models and hashes.')
