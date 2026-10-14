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

QUESTS = {38459,39597,38473,38611,38312,38612}
OBJECTS = set()

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

quests={int(r['ID']):r for r in records['quest_template']}
assert quests[38473]['AllowableRaces']=='18875469' and quests[38611]['AllowableRaces']=='33555378'
assert quests[39597]['RewardNextQuest']=='38473' and quests[38473]['RewardNextQuest']=='38312' and quests[38611]['RewardNextQuest']=='38612'
assert not [r for r in records['quest_objectives'] if int(r['QuestID']) in (38473,38611)]
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
lines=[r for r in data.rows('QuestLineXQuest').values() if r['QuestLineID'] in (129,132,133)]
for line,expected in ((129,[38331,39590,39591,39592,39593,39594,39597]),(132,[38473,38312]),(133,[38611,38612])):
 actual=[r['QuestID'] for r in sorted(lines,key=lambda r:r['OrderIndex']) if r['QuestLineID']==line]
 assert actual[:len(expected)]==expected
# Old TDB RewardSpell81040 is absent from installed26972 native effects.
# Preserve the live zero reward, rather than resurrecting a missing phase spell.
assert not any(r['SpellID']==81040 for r in data.rows('SpellEffect').values())
models=[data.rows('CreatureDisplayInfo')[i] for i in (34520,65132,76630)]
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
 'quests':[{k:r[k] for k in ('ID','LogTitle','AllowableRaces','LogDescription','QuestDescription','RewardNextQuest','RewardSpell','Flags','FlagsEx')} for r in records['quest_template']],
 'objectives':records.get('quest_objectives',[]),'native_enders':records.get('creature_questender',[]),
 'quest_lines':sorted(lines,key=lambda r:(r['QuestLineID'],r['OrderIndex'])),'models':models,'runtime_hashes':data.hashes,
 'inference':'The shared chapter ends in blessing39597 and the two race-specific Thorignir chapters begin38473/38611. The Horde predecessor repair follows the shared blessing and existing Alliance gate; native QuestLineXQuest alone does not encode a cross-chapter addon prerequisite.',
 'limits':'Native quest/line/race/model and missing legacy spell data; actual NPC/quest UI, travel, phase transitions and faction client handoffs remain unverified. No new visual spell or synthetic arrival objective is introduced.'}
path=ROOT/'docs/audits/stormheim-thorignir-native-2026-10-14.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: native shared chapter and both race-specific Thorignir branches, empty breadcrumbs, next relations/stories/models, absent legacy81040 and source hashes.')
