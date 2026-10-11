import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={40229,39456};npcs={95392,99619,99624,93826,108434}
p=ROOT/Path('.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql');names={'quest_template','quest_objectives','creature_template','creature_queststarter','creature_questender','quest_template_addon'};cols={};data={};table=None
for line in p.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in names else None;cols[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:cols[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in names:continue
 name=m[1]
 for m in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  r=dict(zip(cols[name],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",m[0][1:-1])))
  keep=int(r.get('entry',0)) in npcs if name=='creature_template' else int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in ids
  if keep:data.setdefault(name,[]).append(r)
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']]
assert {r['ID']:(r['Type'],r['StorageIndex'],r['ObjectID'],r['Amount'],r['Flags'],r['Flags2']) for r in objectives}=={280678:(3,0,99619,1,0,0),280843:(0,1,99619,1,3,0),280680:(0,2,99624,1,12,0)}
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[40229]['RewardNextQuest'])==39456 and int(quests[39456]['RewardNextQuest'])==0
models={int(r['entry']):int(r['modelid1']) for r in data['creature_template']}
assert models[99619]==models[99624]==69863
assert {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}=={(95392,40229),(93826,39456),(108434,39456)}
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'models':models,'enders':data['creature_questender'],'quests':quests}
evidence=ROOT/'docs/audits/highmountain-unexpected-allies-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native distinct meet/witness/optional wait objectives, both Navarrogg models, Mayla enders and end-of-branch reward links.')
