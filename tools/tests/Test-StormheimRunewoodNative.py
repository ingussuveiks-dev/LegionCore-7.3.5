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

QUESTS = {39796,38778,38810}
OBJECTS = set()
SPELLS = {184579,184582,184792,184794,184585,192155,192156,192157,192159,192160,192161}

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
assert quests[39796]['RewardNextQuest']=='38778' and quests[38778]['RewardNextQuest']=='38810'
assert not [r for r in records['quest_objectives'] if r['QuestID']=='39796']
objectives=[r for r in records['quest_objectives'] if r['QuestID']=='38778']
assert {(r['ObjectID'],r['Amount'],r['StorageIndex'],r['Flags']) for r in objectives}=={('93196','3','0','0'),('93185','1','1','28'),('93340','1','2','28'),('93341','1','3','28')}
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[r for r in data.rows(name).values() if r.get('SpellID') in SPELLS] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions')}
for controller,summon,entry in ((192155,184582,93182),(192156,184792,93342),(192157,184794,93343)):
 assert any(r['SpellID']==controller and r['EffectAura']==428 and r['EffectTriggerSpell']==summon for r in native['SpellEffect'])
 assert any(r['SpellID']==summon and r['Effect']==28 and r['EffectMiscValue1']==entry and r['EffectMiscValue2']==719 and r['ImplicitTarget1']==46 for r in native['SpellEffect'])
assert any(r['SpellID']==184579 and r['Effect']==3 and r['ImplicitTarget1']==25 for r in native['SpellEffect'])
assert not native['SpellXSpellVisual'] # Names alone are not evidence of an activation VFX.
native['SummonProperties']=[data.rows('SummonProperties')[719]]
assert native['SummonProperties'][0]['Flags']&16 and native['SummonProperties'][0]['Control']==0
native['SpellCastTimes']=[data.rows('SpellCastTimes')[i] for i in sorted({r['CastingTimeIndex'] for r in native['SpellMisc']})]
assert all(r['Base']==0 for r in native['SpellCastTimes'])
native['SpellDuration']=[data.rows('SpellDuration')[i] for i in sorted({r['DurationIndex'] for r in native['SpellMisc'] if r['DurationIndex']})]
native['SpellRange']=[data.rows('SpellRange')[i] for i in sorted({r['RangeIndex'] for r in native['SpellMisc'] if r['RangeIndex']})]
native['CreatureDisplayInfo']=[data.rows('CreatureDisplayInfo')[i] for i in (63471,65138)]
native['CreatureModelData']=[data.rows('CreatureModelData')[r['ModelID']] for r in native['CreatureDisplayInfo']]
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']], 'objectives':records['quest_objectives'],'enders':records['creature_questender'],'poi':records['quest_poi_points'],'native':native,'runtime_hashes':data.hashes,'limits':'Native empty travel39796, mandatory3+three optional unique markers, personal wild summons719, controller428 and destination46, instant turn184579/explicit-any target25, native stone/Vydhar display/model dependencies. These spells have no native SpellXSpellVisual rows. Quarter-turn facing and retry radii are authored; no recovered retail puzzle, M2 animation or client playback certification.'}
path=ROOT/'docs/audits/stormheim-runewood-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: native39796 travel,38778 three unique sites/38810 handoff, controller428->personal wild719 summons/destination46, instant explicit-any turn and real model dependencies; no invented spell VFX.')
