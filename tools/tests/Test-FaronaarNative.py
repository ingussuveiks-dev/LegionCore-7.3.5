"""Read-only 7.3.5 evidence for the Faronaar quest chain recovery."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
quests = {36920, 40815, 44140, 37656, 37450, 37449}
dump = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
objectives = []
for line in dump.open(encoding='utf-8'):
    if not line.startswith('INSERT INTO `quest_objectives`'):
        continue
    for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        parts = match[0][1:].split(',')
        if int(parts[1]) in quests:
            objectives.append(dict(zip(('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2'),map(int,parts[:9]))))
assert len(objectives) == 8
by_id = {r['ID']: r for r in objectives}
assert (by_id[277252]['ObjectID'],by_id[277252]['Amount']) == (90487,6)
assert (by_id[277273]['ObjectID'],by_id[277273]['Amount']) == (239455,3)
assert by_id[277456]['ObjectID'] == 120359 and by_id[277456]['Flags2'] == 1
assert by_id[277505]['ObjectID'] == 90982 and by_id[277505]['Flags'] & 4  # optional ride
assert by_id[286100]['ObjectID'] == 112175 and by_id[286100]['Flags'] == 0
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/progression-audit')
effects = [r for r in d.rows('SpellEffect').values() if r['SpellID'] in (178860,178923,46598,65612,184813)]
assert any(r['SpellID']==178860 and r['Effect']==28 and r['EffectMiscValue1']==90474 and r['ImplicitTarget1']==142 for r in effects)
assert any(r['SpellID']==178923 and r['Effect']==28 and r['EffectMiscValue1']==90982 for r in effects)
assert any(r['SpellID']==46598 and r['EffectAura']==236 for r in effects)
assert any(r['SpellID']==65612 and r['EffectAura']==4 for r in effects)
assert not any(r['SpellID']==184813 for r in effects)
lock = d.rows('Lock')[2358]
assert lock['Type1']==1 and lock['Index1']==120359
properties = {i:d.rows('SummonProperties')[i] for i in (3631,3635)}
assert properties[3635]['Control']==4  # native summon forces boarding
vehicle = d.rows('Vehicle')[4103]
assert vehicle['SeatID1']==15333
seats = {i:d.rows('VehicleSeat')[i] for i in (15333,15334)}
line = [r for r in d.rows('QuestLineXQuest').values() if r['QuestLineID']==82 and r['QuestID'] in quests]
assert {r['QuestID'] for r in line} == quests
evidence = dict(archive=dump.name, archive_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),
    objectives=sorted(objectives,key=lambda r:(r['QuestID'],r['Order'])),questline=line,
    effects=effects,lock=lock,summon_properties=properties,vehicle=vehicle,seats=seats,runtime_sha256=d.hashes,
    limitations='Confirms original identities, required versus optional objectives and key/vehicle mechanics. Does not prove client visual presentation or a completed in-game playthrough.')
if '--write-evidence' in sys.argv:
    (ROOT/'docs/audits/faronaar-native-2026-10-09.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
print('PASS: original 7.3.5 objective IDs/counts, optional ride, key lock, companion summon, chain beam and vehicle/seat identities.')
