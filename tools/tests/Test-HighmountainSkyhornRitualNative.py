import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39387};npcs={98773,99225,98777,98778}
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
assert {r['ID']:(r['Type'],r['StorageIndex'],r['ObjectID'],r['Amount'],r['Flags'],r['Flags2']) for r in objectives}=={280358:(3,0,98773,1,0,0),280508:(0,1,98773,1,3,0),280509:(0,2,99225,1,12,0)}
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[39387]['RewardNextQuest'])==0
assert {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}=={(93826,39387),(108434,39387)}
sys.dont_write_bytecode=True;sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (196066,194359)]
assert any(r['SpellID']==196066 and r['Effect']==61 and r['EffectMiscValue1']==47401 for r in fx)
assert any(r['SpellID']==194359 and r['Effect']==28 and r['EffectMiscValue1']==98778 and r['EffectMiscValue2']==3054 for r in fx)
vehicle=d.rows('Vehicle')[4428]
seats={r: d.rows('VehicleSeat')[r] for k,r in vehicle.items() if k.startswith('SeatID') and r}
props=d.rows('SummonProperties')[3054]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'enders':data['creature_questender'],'quests':quests,'effects':fx,'aviash_vehicle':vehicle,'aviash_seats':seats,'aviash_properties':props,'hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-skyhorn-ritual-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Skyhorn meet/witness/optional wait, Mayla enders, ritual event and Aviash summon/vehicle data.')
