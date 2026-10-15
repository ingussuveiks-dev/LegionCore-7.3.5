"""Read original TDB735 quest/chest records and native 7.3.5 Open mechanics."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

QUESTS = {38817,38815}
archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template','quest_objectives','creature_questender','creature_queststarter','gameobject_template','gameobject_loot_template'}
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
        key = int(next((row[k] for k in ('QuestID','quest','ID','entry','Entry') if k in row),'0'))
        if (name == 'gameobject_template' and key in (241870,241871,241872,241873,241874)) or (name == 'gameobject_loot_template' and key in (241870,241871,241872,241873,241874)) or (name not in ('gameobject_template','gameobject_loot_template') and key in QUESTS):
            records.setdefault(name, []).append(row)

quests={int(row['ID']):row for row in records['quest_template']}
assert quests[38817]['RewardNextQuest']=='38815' and quests[38815]['RewardNextQuest']=='38818'
objective=[r for r in records['quest_objectives'] if r['QuestID']=='38817']
assert len(objective)==1 and objective[0]['Type']=='1' and objective[0]['ObjectID']=='124512' and objective[0]['Amount']=='8' and objective[0]['StorageIndex']=='1' and objective[0]['Flags']=='0'
chests=records['gameobject_template']
assert {int(r['entry']) for r in chests}==set(range(241870,241875))
assert all(r['type']=='3' and r['Data0']=='1690' and r['Data8']=='38817' and r['Data1']=='0' for r in chests)
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
effects=[r for r in data.rows('SpellEffect').values() if r.get('SpellID')==3365]
assert any(r['Effect']==33 and r['EffectMiscValue1']==5 for r in effects)
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']],'objectives':records['quest_objectives'],'chests':chests,'original_loot':records.get('gameobject_loot_template',[]),'starters':records.get('creature_queststarter',[]),'enders':records.get('creature_questender',[]),'native_open3365':effects,'runtime_hashes':data.hashes,'limits':'Original TDB required eight Ashildir bones, ITEM storage1, five chest types/native lock/display/quest settings and38815 next. The archive retains their missing loot selectors; current guaranteed loot rows/spawns/relations are checked separately in live World SQL. Separate native C++ DB2Storage reads sparse124512, lock1690 and all five GO displays/Iounn. Physical access, loot rendering and full gameplay/reward execution remain client-unverified.'}
path=ROOT/'docs/audits/stormheim-regal-remains-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original38817 collect8 Ashildir bones/storage1, five chest templates/lock settings and38815 handoff; native Open3365/effect33/type5.')
