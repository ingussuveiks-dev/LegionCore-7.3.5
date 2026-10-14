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

QUESTS = {39592,39593}
OBJECTS = {243802,243814}
SPELLS = {190707,190708,190549,233562,190746,190752,190758,197658,197659,197660,195506,187653,187656,187661}

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
assert (objectives[279721]['Type'],objectives[279721]['ObjectID'],objectives[279721]['StorageIndex'],objectives[279721]['Amount'])==('2','243802','3','1')
objects={int(row['entry']):row for row in records['gameobject_template']}
assert objects[243814]['Data0']=='1691' and objects[243814]['Data10']=='0'
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[r for r in data.rows(name).values() if r.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions')}
assert any(v['SpellID']==190707 and v['Effect']==28 and v['EffectMiscValue1']==96144 and v['ImplicitTarget1']==46 for v in native['SpellEffect'])
assert any(v['SpellID']==190707 and v['Effect']==64 and v['EffectTriggerSpell']==190708 for v in native['SpellEffect'])
assert any(v['SpellID']==190708 and v['Effect']==28 and v['EffectMiscValue1']==96067 for v in native['SpellEffect'])
assert [(v['SpellID'],v['EffectMiscValue1']) for v in native['SpellEffect'] if v['SpellID'] in (197658,197659,197660) and v['Effect']==219]==[(197658,773),(197659,774),(197660,775)]
assert not any(v['Effect']==16 and v['EffectMiscValue1']==39592 for v in data.rows('SpellEffect').values())
native['SummonProperties']=[data.rows('SummonProperties')[719]]
native['Lock']=[data.rows('Lock')[1691]]
native['CreatureDisplayInfo']=[data.rows('CreatureDisplayInfo')[67133]]
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
native['GameObjectBounds']=[data.rows('GameObjectDisplayInfo')[30191]]
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
 'limits':'Native spell/effect/visual/kit/attachment/model definitions and quest slots are data evidence. Client interaction, cast bar, encounter combat, projection animation, audio and visuals remain unverified.'}
path=ROOT/'docs/audits/stormheim-will-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original powered-console slot, native owner-copy/dragon summon, dragon model, three conversations, no native quest-complete spell, visual kits/attachments/models and hashes.')
