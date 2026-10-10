"""Read-only native evidence for the 46-quest Val'sharah main-story audit."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
IDS={39731,39861,40122,38384,38382,39383,39384,40573,38142,38455,38922,38246,38143,38145,38144,38147,38146,38381,38235,38225,38323,38322,38148,38377,38641,38662,38663,38595,38582,38753,38675,38684,38687,38743,40567,40890,41724,41890,41893,41054,41056,41708,41749,41763,43576,43702}
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};data={};table=None
names=('quest_template','quest_objectives','creature_questender','gameobject_queststarter','gameobject_questender')
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
assert set(q)==IDS
assert q[43576]['QuestSortID']=='7558' and q[43576]['StartItem']=='0'
assert (q[38743]['QuestPackageID'],q[38743]['RewardItem1'])==('0','141383')
assert q[38377]['RewardItem1']=='141387' and q[38753]['RewardItem1']=='141390'
assert q[40890]['QuestPackageID']=='665' and q[40890]['StartItem']==q[40890]['ItemDrop1']=='139043'
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
lines=[r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in IDS]
for line,ids in ((189,[41054,41890,43576,38675,41749,38684,43702,41763,38743]),(190,[41056,41708,43576,41724,41893,38684,43702,38687,38743])):
 assert [r['QuestID'] for r in sorted(lines,key=lambda r:r['OrderIndex']) if r['QuestLineID']==line]==ids
assert {141383,141387,141390,139043}<=d.rows('Item').keys()
rewards=[r for r in d.rows('QuestPackageItem').values() if r['PackageID']==665]
assert len(rewards)==4
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(182117,180935,192293,192295,181865,81040)]
assert not any(r['SpellID'] in(181865,81040) for r in fx) # do not revive deleted rewards
assert any(r['SpellID']==192293 and r['Effect']==252 for r in fx)
out=dict(scope=sorted(IDS),quests={i:{k:v for k,v in r.items() if k in('ID','LogTitle','QuestSortID','QuestPackageID','StartItem','ItemDrop1','ItemDropQuantity1','RewardItem1','RewardAmount1','RewardSpell','Flags')} for i,r in q.items()},
 objectives=[{k:int(v) for k,v in r.items() if k in('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']],
 quest_lines=lines,chest_package=rewards,spell_effects=fx,runtime_hashes=d.hashes,archive=p.name,archive_sha256=hashlib.sha256(p.read_bytes()).hexdigest())
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/valsharah-chain-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'PASS: {len(IDS)} native quests, {len(out["objectives"])} objectives, both faction quest lines, real rewards and absent legacy spells.')
