"""Read original TDB735 quests/POIs and installed native 7.3.5 spell data."""
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

QUESTS = {39803,39804,39796}
OBJECTS = set()
SPELLS = {192170,192171,114371}

archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template', 'quest_objectives', 'creature_questender', 'gameobject_template', 'quest_template_addon', 'gameobject_queststarter', 'gameobject_questender','creature_queststarter','quest_poi','quest_poi_points'}
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
assert quests[39803]['RewardNextQuest']=='39804' and quests[39804]['RewardNextQuest']=='39796'
assert not [r for r in records['quest_objectives'] if r['QuestID']=='39803']
assert [(r['ID'],r['Type'],r['ObjectID'],r['Amount'],r['StorageIndex']) for r in records['quest_objectives'] if r['QuestID']=='39804']==[('279981','0','97340','1','0')]
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[r for r in data.rows(name).values() if r.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions')}
assert any(r['SpellID']==192171 and r['Effect']==134 and r['EffectMiscValue1']==97340 and r['ImplicitTarget1']==1 for r in native['SpellEffect'])
assert any(r['SpellID']==192170 and r['Effect']==219 and r['EffectMiscValue1']==2247 for r in native['SpellEffect'])
assert any(r['SpellID']==114371 and r['EffectAura']==66 for r in native['SpellEffect'])
casting={r['CastingTimeIndex'] for r in native['SpellMisc'] if r['SpellID']==192171}
native['SpellCastTimes']=[data.rows('SpellCastTimes')[i] for i in sorted(casting)]
assert native['SpellCastTimes'][0]['Base']==15000
native['SpellDuration']=[data.rows('SpellDuration')[i] for i in sorted({r['DurationIndex'] for r in native['SpellMisc'] if r['DurationIndex']})]
native['CreatureDisplayInfo']=[data.rows('CreatureDisplayInfo')[i] for i in (63734,76630,64208,57052)]
native['CreatureModelData']=[data.rows('CreatureModelData')[r['ModelID']] for r in native['CreatureDisplayInfo']]
native['ConversationLine_native_reader']=(ROOT/'build-extractors/runtime-test/stormheim-bones/native-lines.txt').read_text(encoding='utf-8-sig').splitlines()
assert native['ConversationLine_native_reader']==['4740 98439 0 1000 4741 0 0','4741 98440 0 1000 4742 0 0','4742 98441 0 1000 4743 0 0','4743 98442 0 1000 0 0 0','invalid_anim_kit192171 0']
reader_inputs=json.loads((ROOT/'build-extractors/runtime-test/stormheim-bones/native-lines-inputs.json').read_text(encoding='utf-8-sig'))
for name in ('ConversationLine','AnimKit'):
    data.hashes[name]=hashlib.sha256((data.dbc/(name+'.db2')).read_bytes()).hexdigest()
    assert data.hashes[name]==reader_inputs[name].lower(), 'Rerun Test-StormheimBonesCatalog.ps1 for the current runtime DB2 files'
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
# Preserve any empty attachment trace instead of inventing a model effect.
assert used_visuals and used_events
trace = {'visuals': [visuals[v] for v in sorted(used_visuals)], 'events': used_events,
         'kits': [kits[k] for k in sorted(used_kits)], 'kit_effects': used_effects,
         'model_attachments': [attachments[a] for a in sorted(used_attachments)],
         'effect_names': [effect_names[n] for n in sorted(used_names)]}
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']], 'objectives':records['quest_objectives'],'enders':records['creature_questender'],'poi':records['quest_poi_points'],'native':native,'visual_trace':trace,'runtime_hashes':data.hashes,'limits':'Metadata: empty travel39803, actual39804 moose marker, native fifteen-second self-targeted effect134 and conversation2247, preserved feign-death aura/model, cast/visual/attachment and native conversation-line records. Conversation playback, complete retail butchery choreography and client UI are unverified.'}
path=ROOT/'docs/audits/stormheim-bones-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: native39803 travel/39804 moose/39796 handoff, fifteen-second self-credit134, Havi conversation2247, pose/model and visual graph.')
