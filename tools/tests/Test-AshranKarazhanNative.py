"""Verify the extracted 7.3.5 identities used by Ashran and Karazhan repairs."""
import json
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
D = Data(ROOT / 'build-extractors/bin/Release/dbc/enUS', ROOT / '.codex/progression-audit')
checks = {}
for table, identity in [('Map',1191),('MapDifficulty',3518),('LFGDungeons',1127),('PlayerCondition',43731),('SpellRadius',13)]:
    checks[table] = D.rows(table)[identity]
assert checks['Map']['InstanceType'] == 5
assert checks['MapDifficulty']['MapID'] == 1191 and checks['MapDifficulty']['DifficultyID'] == 25
assert checks['LFGDungeons']['MapID'] == 1191 and checks['LFGDungeons']['MinLevel'] == 110
assert checks['PlayerCondition']['MinLevel'] == 110
assert checks['SpellRadius']['Radius'] == 10.0
areas = D.rows('AreaTable')
assert areas[8485]['ContinentID'] == 1191 and areas[6941]['ContinentID'] == 1116
checks['areas'] = {i: areas[i] for i in (8485,6941,7279,7332,7333)}
graves = D.rows('WorldSafeLocs')
checks['graveyards'] = {i: graves[i] for i in (4742,4743,4822,4825,4821,4824)}
assert all(r['MapID'] == 1191 for r in checks['graveyards'].values())
effects = D.rows('SpellEffect')
checks['effects'] = [r for r in effects.values() if r['SpellID'] in (228208,231458,229466,228271,230118,230205)]
for spell, credit in [(228208,114613),(231458,114622),(229466,115415),(230118,115871),(230205,114322)]:
    assert any(r['SpellID']==spell and r['Effect']==134 and r['EffectMiscValue1']==credit for r in checks['effects'])
assert any(r['SpellID']==230118 and r['Effect']==252 for r in checks['effects'])
assert any(r['SpellID']==228271 and r['Effect']==3 and r['EffectRadiusIndex1']==13 and r['ImplicitTarget1']==38 for r in checks['effects'])
checks['itemEffects'] = [r for r in D.rows('ItemEffect').values() if r['ItemID']==141878]
assert len(checks['itemEffects'])==1 and checks['itemEffects'][0]['SpellID']==228271
checks['sha256'] = D.hashes
checks['sql_overlay_sha256'] = D.sql_hashes
if '--write-evidence' in sys.argv:
    (ROOT/'docs/audits/ashran-karazhan-native-2026-10-09.json').write_text(json.dumps(checks,indent=2)+'\n',encoding='utf-8')
print('PASS: 7.3.5 Ashran instance/area/level and Karazhan item, target radius, teleport and native objective credits')
