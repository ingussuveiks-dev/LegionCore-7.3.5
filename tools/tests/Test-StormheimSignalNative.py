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

QUESTS = {38060,38362}
OBJECTS = {240235}
SPELLS = {180860,183439,183454,183005,220129,210262,183265,183266,183267,183557}
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

quests = {int(row['ID']):row for row in records['quest_template']}
objectives = {int(row['ID']):row for row in records['quest_objectives']}
assert set(quests)==QUESTS
assert [(objectives[i]['ObjectID'],objectives[i]['StorageIndex'],objectives[i]['Flags']) for i in (278188,277473)]==[('92475','1','0'),('90907','0','3')]
assert objectives[277873]['Type']=='1' and objectives[277873]['ObjectID']=='122746' and objectives[277873]['Amount']=='1'
go=records['gameobject_template'][0]
assert (go['Data0'],go['Data1'],go['Data2'],go['Data10'])==('1690','38060','44654','0')
data = Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[row for row in data.rows(name).values() if row.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions')}
assert any(r['SpellID']==183439 and r['Effect']==6 and r['EffectAura']==4 and r['ImplicitTarget1']==1 for r in native['SpellEffect'])
assert any(r['SpellID']==210262 and r['Effect']==3 and r['EffectMiscValue1']==0 and r['EffectTriggerSpell']==0 for r in native['SpellEffect'])
assert any(r['SpellID']==183266 and r['EffectTriggerSpell']==183265 for r in native['SpellEffect'])
assert any(r['SpellID']==183267 and r['EffectAura']==395 and r['EffectMiscValue1']==4069 for r in native['SpellEffect'])
native['Lock']=[data.rows('Lock')[1690]]

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
native['GameObjectDisplayInfo']=[visual_records('GameObjectDisplayInfo',{'model_file_id':0})[26905]]
assert native['GameObjectDisplayInfo'][0]['model_file_id']>0
data.hashes['ItemSparse']=hashlib.sha256((data.dbc/'ItemSparse.db2').read_bytes()).hexdigest()
native['ItemSparse_probe']='Test-StormheimTrophyItemNative.ps1 uses the production sparse-record DB2 loader for head 122746: Stackable=1, Bonding=4, LimitCategory=0.'

out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
 'quests':[{key:row[key] for key in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for row in records['quest_template']],
 'objectives':records['quest_objectives'],'objects':records['gameobject_template'],'enders':records['creature_questender'],
 'native':native,'visual_trace':trace,'runtime_hashes':data.hashes,
 'limits':'Native quest objectives/GO event field, item, signal and retained combat spell/visual dependency data. 44654 is a GO event ID, not its spell field; 210262 is a dummy, not a helper summon. No client animation, dialogue, actual loot roll or retail choreography verified.'}
path=ROOT/'docs/audits/stormheim-signal-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original signal/trophy objectives, real propeller event/lock/model, trophy item, native dummy/area-trigger dependencies and signal/retained combat visual kits, attachments and models.')
