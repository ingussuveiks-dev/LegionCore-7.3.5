"""Read-only 7.3.5 quest, item, objective, teleport and reward evidence."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};data={};table=None
for line in p.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `(quest_template|quest_objectives)`',line)
 if m:table=m[1];columns[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:columns[table].append(m[1])
 for name in ('quest_template','quest_objectives'):
  if line.startswith('INSERT INTO `'+name+'`'):
   for m in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
    parts=re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",m[0][1:-1])
    if (name=='quest_template' and int(parts[0]) in(38286,42213)) or (name=='quest_objectives' and int(parts[1]) in(38286,42213)):
     data.setdefault(name,[]).append(dict(zip(columns[name],parts)))
q={int(r['ID']):r for r in data['quest_template']}
assert q[38286]['QuestPackageID']=='0' and q[38286]['RewardItem1']=='141385' and q[38286]['RewardAmount1']=='1'
assert q[42213]['QuestPackageID']=='18462' and q[42213]['StartItem']=='137206' and q[42213]['ItemDrop1']=='137206' and q[42213]['ItemDropQuantity1']=='1'
assert {(int(r['QuestID']),int(r['ObjectID'])) for r in data['quest_objectives']}=={(38286,91784),(38286,96028),(38286,106847),(42213,106815)}
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(192293,192295)]
assert any(r['Effect']==252 for r in fx if r['SpellID']==192293) and any(r['Effect']==252 for r in fx if r['SpellID']==192295)
assert any(r['Effect']==90 and r['EffectMiscValue1']==97481 for r in fx if r['SpellID']==192293)
assert {137206,141385}<=d.rows('Item').keys()
rewards=[r for r in d.rows('QuestPackageItem').values() if r['PackageID']==18462]
assert {r['ItemID'] for r in rewards}=={140622,140623,140624,140625}
assert {27762,31380}<=d.rows('GameObjectDisplayInfo').keys()
evidence=dict(quests={k:{f:int(v) for f,v in r.items() if f in('ID','QuestPackageID','RewardItem1','RewardAmount1','RewardNextQuest','StartItem','ItemDrop1','ItemDropQuantity1','RewardSpell')} for k,r in q.items()},
 objectives=[{k:int(v) for k,v in r.items() if k in('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']],
 effects=fx,belt_package=rewards,runtime_hashes=d.hashes,archive=p.name,archive_sha256=hashlib.sha256(p.read_bytes()).hexdigest())
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/eye-quest-chain-native-2026-10-10.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS: two native quests/four objectives, Tidestone Sliver reward, provided/removed Tidestone, four belt choices, original two-way teleport spells and existing model IDs.')
