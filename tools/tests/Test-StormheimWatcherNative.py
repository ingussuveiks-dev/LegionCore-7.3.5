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

QUESTS = {39593,39594}
OBJECTS = {243819,243820,243822,243823}
SPELLS = set()

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

assert {i:(v['Type'],v['ObjectID'],v['StorageIndex'],v['Amount']) for i,v in objectives.items() if v['QuestID']=='39593'}=={279722:('1','128493','3','1'),279724:('1','128495','1','1'),279725:('1','128496','2','1'),279726:('1','128497','4','1')}
objects={int(v['entry']):v for v in records['gameobject_template']}
assert all(v['type']=='3' and v['Data0']=='1691' for v in objects.values())
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={'Lock':[data.rows('Lock')[1691]],'Phase':[data.rows('Phase')[6665]]}
assert native['Phase'][0]['Flags']==0
groups=[v for v in data.rows('PhaseXPhaseGroup').values() if v.get('PhaseID')==6665]
assert not groups, groups
effects=[v for v in data.rows('SpellEffect').values() if v['EffectAura'] in (261,326)
    and (v['EffectMiscValue1']==6665 or v['EffectMiscValue2']==6665)]
assert not effects, effects
native['GameObjectDisplayInfo']=[data.rows('GameObjectDisplayInfo')[int(v['displayId'])] for v in objects.values()]
native['CreatureDisplayInfo']=[data.rows('CreatureDisplayInfo')[25217]]
native['CreatureModelData']=[data.rows('CreatureModelData')[native['CreatureDisplayInfo'][0]['ModelID']]]
assert all(v['FileDataID']>0 for v in native['GameObjectDisplayInfo'])
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
 'quests':[{k:v[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','RewardNextQuest','Flags','FlagsEx')} for v in records['quest_template']],
 'objectives':records['quest_objectives'],'objects':records['gameobject_template'],'go_starters':records.get('gameobject_queststarter',[]),'go_enders':records.get('gameobject_questender',[]),
 'native':native,'runtime_hashes':data.hashes,
 'limits':'Original fragment objectives/physical-repair story/next quest text, native chest lock and model/bounds, and supported isolated Phase 6665 with no PhaseXPhaseGroup or native phase aura are data evidence. ItemSparse uses a separate production DB2Storage probe. Client castbar, pose, model/rendering, collection UI, assembly visual and hand-in/phase visibility remain unverified.'}
path=ROOT/'docs/audits/stormheim-watcher-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: four original item IDs/amounts/storage slots, native fragment models/lock, physical-restoration story and supported isolated phase with no native group/aura collisions.')
