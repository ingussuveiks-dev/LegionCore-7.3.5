import sys,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
dbc=ROOT/'build-extractors/bin/Release/dbc/enUS'
d=Data(dbc,ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID']==190370 and r['DifficultyID']==0]
assert any(r['Effect']==28 and r['EffectMiscValue1']==96038 and r['EffectMiscValue2']==3679 for r in fx)
p=d.rows('SummonProperties')[3679]
assert (p['Control'],p['Faction'],p['Title'],p['Slot'])==(1,1665,0,0)
assert p['Flags'] & 512  # This core creates an uncontrolled Guardian, not a primary pet.
lock=d.rows('Lock')[1691]
assert (lock['Type2'],lock['Index2'],lock['Skill2'])==(2,5,0)  # ordinary Open, no profession requirement
result={'build':'7.3.5.26972','spell':190370,'effects':fx,'summonProperties':p,'lock':lock,'hashes':{name:hashlib.sha256((dbc/(name+'.db2')).read_bytes()).hexdigest() for name in ('SpellEffect','SummonProperties','Lock')}}
if '--write-evidence' in sys.argv:
 (ROOT/'docs/audits/highmountain-lifespring-native-2026-10-10.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8')
else:
 assert result==json.loads((ROOT/'docs/audits/highmountain-lifespring-native-2026-10-10.json').read_text())
print('PASS: installed 7.3.5 DB2 summon spell, ally properties and native crystal lock; no substitute IDs.')
