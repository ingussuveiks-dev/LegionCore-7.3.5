"""Read-only 7.3.5 Highmountain opening evidence."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
IDS={39661,39488,39489,39487,39498}
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};data={};table=None
names=('quest_template','quest_objectives','creature_questender','creature_template','gameobject_template')
for line in p.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in names else None;columns[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:columns[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in names:continue
 name=m[1]
 for m in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  parts=re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",m[0][1:-1]);r=dict(zip(columns[name],parts))
  keep=int(r.get('entry',0)) in (95916,95881,95882,243639) if name in ('creature_template','gameobject_template') else int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in IDS
  if keep:data.setdefault(name,[]).append(r)
q={int(r['ID']):r for r in data['quest_template']}
assert set(q)==IDS and len(data['quest_objectives'])==3
obj={int(r['ID']):r for r in data['quest_objectives']}
assert obj[279636]['Flags2']=='1' and obj[279636]['ObjectID']=='128393' and obj[279636]['Amount']=='10'
assert obj[279791]['ObjectID']=='95866' and obj[279791]['Amount']=='7'
assert obj[279635]['ObjectID']=='95881' and obj[279635]['Amount']=='1'
creatures={int(r['entry']):r for r in data['creature_template']}
assert creatures[95916]['KillCredit1']=='95866'
assert q[39487]['QuestPackageID']=='9699' and q[39498]['RewardNextQuest']=='42104'
assert {int(r['id']) for r in data['creature_questender'] if r['quest']=='39661'}=={96520}
assert {int(r['id']) for r in data['creature_questender'] if r['quest']=='39498'}=={97662}
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spellIds={195320,195344,195346,195596,195597,197769,197773,197798,219913}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spellIds]
assert {r['SpellID'] for r in fx}==spellIds
assert any(r['SpellID']==195596 and r['EffectTriggerSpell']==195597 for r in fx)
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in spellIds]
stun=next(r for r in misc if r['SpellID']==195597)
assert d.rows('SpellDuration')[stun['DurationIndex']]['Duration']==16000
out=dict(scope=sorted(IDS),quests={i:{k:v for k,v in r.items() if k in('ID','LogTitle','LogDescription','QuestPackageID','StartItem','Flags','RewardNextQuest')} for i,r in q.items()},
 objectives=[{k:int(v) for k,v in r.items() if k in('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']],
 enders=data['creature_questender'],creatures={i:{k:r[k] for k in('entry','name','KillCredit1','KillCredit2')} for i,r in creatures.items()},
 crystal_template=data['gameobject_template'],spell_effects=fx,spell_misc=misc,runtime_hashes=d.hashes,archive=p.name,archive_sha256=hashlib.sha256(p.read_bytes()).hexdigest())
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/highmountain-cavern-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: five native quests, three objectives, alternate Crageater kill credit, delivery enders, reward package and native Gelmogg combat/petrification spells.')
