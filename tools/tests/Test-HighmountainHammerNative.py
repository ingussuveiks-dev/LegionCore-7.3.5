import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={42454};npcs={107587}
p=ROOT/Path('.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql');names={'quest_template','quest_objectives','creature_template','creature_queststarter','creature_questender','quest_template_addon','gameobject_queststarter','gameobject_questender'};cols={};data={};table=None
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


obj=data['quest_objectives'];assert len(obj)==1
assert [int(obj[0][k]) for k in ('ID','Type','StorageIndex','ObjectID','Amount','Flags')]==[284276,0,3,107587,1,0]
q=data['quest_template'][0]
assert [int(q[k]) for k in ('StartItem','ItemDrop1','ItemDropQuantity1')]==[137649,137649,1]
assert data['gameobject_questender']==[{'id':'246463','quest':'42454'}]
sys.dont_write_bytecode=True;sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/hammer-no-overlays')
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (192293,192295)]
assert all(any(r['SpellID']==id and r['Effect'] in (5,252) for r in effects) for id in (192293,192295))
item_effects=[r for r in d.rows('ItemEffect').values() if r['ItemID']==137649]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':obj,'quest_item_fields':{k:q[k] for k in ('StartItem','ItemDrop1','ItemDropQuantity1','Flags','FlagsEx')},'ender':data['gameobject_questender'],'teleport_effects':effects,'item_effects':item_effects,'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-hammer-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Hammer teleport objective, provision/removal fields, gallery quest ender and installed native teleport effects.')
