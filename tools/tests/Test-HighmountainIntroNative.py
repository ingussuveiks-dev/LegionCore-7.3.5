"""Read-only 7.3.5 Highmountain opening evidence."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
IDS={39733,38907,38911,39491,39272,39490,39496}
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};data={};table=None
names=('quest_template','quest_objectives','creature_questender','gameobject_queststarter','gameobject_questender','quest_poi','quest_poi_points')
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
  if int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in IDS:data.setdefault(name,[]).append(r)
q={int(r['ID']):r for r in data['quest_template']}
assert set(q)==IDS and len(data['quest_objectives'])==11
assert q[39272]['QuestPackageID']=='0' and q[39491]['QuestPackageID']=='679'
obj={int(r['ID']):r for r in data['quest_objectives']}
assert obj[279996]['Flags']=='28' and obj[279644]['Flags2']=='1'
assert {int(r['id']) for r in data['creature_questender'] if r['quest']=='38907'}=={93826}
pois={int(r['QuestObjectID']):r for r in data['quest_poi'] if r['QuestID']=='39272' and int(r['QuestObjectID']) in range(99433,99437)}
points={int(r['Idx1']):r for r in data['quest_poi_points'] if r['QuestID']=='39272'}
for entry,xy in zip(range(99433,99437),((4177,4642),(4213,4616),(4261,4635),(4314,4635))):
 point=points[int(pois[entry]['Idx1'])];assert (int(point['X']),int(point['Y']))==xy
assert not any(r['QuestID']=='38907' and r['BlobIndex']=='0' and r['Idx1']=='5' for r in data['quest_poi'])
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
lines=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestLineID']==144],key=lambda r:r['OrderIndex'])
assert [r['QuestID'] for r in lines[:6]]==[38907,38911,39491,39272,39490,39496]
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(195481,198160)]
assert any(r['SpellID']==195481 and r['Effect']==3 for r in fx)
assert any(r['SpellID']==198160 and r['Effect']==154 and r['EffectMiscValue1']==1719 for r in fx)
assert not any(r['Effect']==16 and r['EffectMiscValue1']==39733 for r in d.rows('SpellEffect').values())
node=d.rows('TaxiNodes')[1719];assert node['ContinentID']==1220 and abs(node['PosZ']-768.16)<0.01
paths=[r for r in d.rows('TaxiPath').values() if r['ToTaxiNode']==1719];assert len(paths)==9
out=dict(scope=sorted(IDS),quests={i:{k:v for k,v in r.items() if k in('ID','LogTitle','QuestPackageID','RewardNextQuest','Flags')} for i,r in q.items()},
 objectives=[{k:int(v) for k,v in r.items() if k in('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']],
 quest_lines=lines,quest_poi=data['quest_poi'],quest_poi_points=data['quest_poi_points'],spell_effects=fx,taxi_node=node,taxi_paths=paths,runtime_hashes=d.hashes,archive=p.name,archive_sha256=hashlib.sha256(p.read_bytes()).hexdigest())
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/highmountain-intro-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: 7 native quests, 11 objectives, Rivermane order, four distinct idol POIs, native dummy click, taxi discovery/node/routes, reward package and valid arrival POIs.')
