"""Read-only native evidence for the 19-quest Azurewing Repose progression."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

quests = {38443,37853,37991,42271,37855,37856,37858,37957,37859,37857,
          37959,37960,37860,37861,37862,38014,38015,42567,42756}
dump = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
objectives = []
for line in dump.open(encoding='utf-8'):
    if not line.startswith('INSERT INTO `quest_objectives`'):
        continue
    for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        parts = match[0][1:].split(',')
        if int(parts[1]) in quests:
            objectives.append(dict(zip(('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2'),map(int,parts[:9]))))
assert len(objectives) == 31
by_id = {r['ID']:r for r in objectives}
assert all(by_id[i]['Flags2']==1 for i in (277011,277144,277268,284014,284016))
assert by_id[284778]['Flags']==28 and by_id[284778]['ObjectID']==107995
assert by_id[280835]['Amount']==1
assert {r['ObjectID'] for r in objectives if r['QuestID']==42756} == {91155,108721}
assert not any(r['QuestID'] in (38443,37855,37858,42567) for r in objectives)

d = Data(ROOT/'build-extractors/bin/Release/dbc/enUS', ROOT/'.codex/progression-audit')
spells = {179915,179913,180713,180463,179825,180515,77901,214482,180019,180066}
effects = [r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
def effect(spell, kind, target=0):
    return any(r['SpellID']==spell and r['Effect']==kind and (not target or r['EffectMiscValue1']==target) for r in effects)
assert d.rows('OverrideSpellData')[573]['Spells1']==179915
assert d.rows('OverrideSpellData')[572]['Spells1']==179825
assert effect(179915,164) and effect(179913,3)
assert effect(180713,90,90880) and effect(180463,134,90167)
assert any(r['SpellID']==180515 and r['Effect']==24 and r['EffectItemType']==122292 for r in effects)
assert effect(180019,28,90406) and effect(180066,28,90476)
assert any(r['SpellID']==77901 and r['EffectAura']==236 for r in effects)
assert all(i not in d.rows('Spell') for i in (179917,179858,180731))
item_effects = [r for r in d.rows('ItemEffect').values() if r['ItemID'] in (122306,138146)]
assert any(r['ItemID']==138146 and r['SpellID']==214482 for r in item_effects)
line = [r for r in d.rows('QuestLineXQuest').values() if r['QuestLineID'] in (82,225) and r['QuestID'] in quests]
assert len([r for r in line if r['QuestLineID']==225])==13
vehicle = d.rows('Vehicle')[1920]
seats = {i:d.rows('VehicleSeat')[i] for k,i in vehicle.items() if k.startswith('SeatID') and i}
evidence = dict(quests=sorted(quests),archive=dump.name,archive_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),
    objectives=sorted(objectives,key=lambda r:(r['QuestID'],r['Order'])),questline=line,effects=effects,
    item_effects=item_effects,vehicle=vehicle,seats=seats,runtime_sha256=d.hashes,
    limitations='Native identities and optional flags, not proof of an in-game playthrough. Saved SQL storage indices are retained. Original SQL waypoints are exercised separately. Recovery combat does not recreate all retail boss abilities or dialogue.')
if '--write-evidence' in sys.argv:
    (ROOT/'docs/audits/azurewing-native-2026-10-09.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
print('PASS: 19 quests / 31 original objectives, real pool/pylon buttons, native whelp/items, optional flight and separate final enemies; obsolete trigger IDs absent.')
