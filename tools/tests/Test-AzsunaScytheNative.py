"""Verify Scythe recovery identities against 7.3.5 DB2 and the 2018 TDB archive.

Pass --write-evidence to refresh the checked-in evidence. The archive is read
only; it is never imported as a database or treated as a source of instructions.
"""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

dump = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
rows = []
for line in dump.open(encoding='utf-8'):
    if line.startswith('INSERT INTO `quest_objectives`'):
        for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
            if re.match(r'\(\d+,37660,', match[0]):
                values = match[0][1:].split(',')
                rows.append(dict(zip(('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2'), map(int, values[:9]))))
assert len(rows) == 10
sql = (ROOT / 'sql/updates/world/2026_10_09_08_azsuna_scythe.sql').read_text(encoding='utf-8')
for row in rows:
    prefix = 'SELECT ' + ','.join(str(row[k]) for k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')) + ',0,'
    assert prefix in sql, prefix
d = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/progression-audit')
effects = [r for r in d.rows('SpellEffect').values() if r['SpellID'] in (178711,178939,179183)]
assert any(r['SpellID']==178711 and r['Effect']==28 and r['EffectMiscValue1']==90401 for r in effects)
assert any(r['SpellID']==178939 and r['Effect']==6 and r['EffectAura']==4 for r in effects)
assert any(r['SpellID']==179183 and r['EffectAura']==260 and r['EffectMiscValue1']==1093 for r in effects)
line = [r for r in d.rows('QuestLineXQuest').values() if r['QuestLineID']==82]
assert any(r['QuestID']==37660 and r['OrderIndex']==4 for r in line)
objectives = [r for r in d.rows('QuestObjective').values() if r['QuestID']==37660]
assert not objectives, 'Reassess if a different runtime client supplies objectives'
evidence = dict(quest=37660, archive=dump.name, archive_sha256=hashlib.sha256(dump.read_bytes()).hexdigest(),
    archive_objectives=sorted(rows,key=lambda r:r['Order']), runtime_db2_objectives=objectives,
    questline=line, effects=effects, runtime_sha256=d.hashes,
    limitations='Objective records come from 7.3.5 TDB build 25549, not the incomplete QuestObjective.db2. Recovery entry uses an offset from the existing exit; personal melee encounter and native screen effect do not reconstruct every original combat ability or cinematic.')
if '--write-evidence' in sys.argv:
    (ROOT / 'docs/audits/azsuna-scythe-native-2026-10-09.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
print('PASS: all ten migration objectives match 7.3.5 TDB; native Allari summon, compel, screen and questline identities verified.')
