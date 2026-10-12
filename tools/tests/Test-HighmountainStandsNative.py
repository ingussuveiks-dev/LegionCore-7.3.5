import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={42512,41246};npcs=set()
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

objective=data['quest_objectives'][0]
assert {k:int(objective[k]) for k in ('ID','QuestID','ObjectID','Amount','Type','StorageIndex','Flags')}==dict(ID=284367,QuestID=42512,ObjectID=107704,Amount=1,Type=0,StorageIndex=0,Flags=1)
assert any(r['ID']=='41246' and 'Terrain Tracking' in r['LogTitle'] for r in data['quest_template'])
assert data['creature_questender']==[{'id':'97407','quest':'42512'}]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
import struct
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/worm-support-no-overlays')
assert not d.sql.exists()
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (213725,205349,197461)]
assert any(r['SpellID']==213725 and r['EffectAura']==430 and r['EffectMiscValue1']==1287 for r in effects)
assert any(r['SpellID']==213725 and r['Effect']==64 and r['EffectTriggerSpell']==205349 for r in effects)
assert any(r['SpellID']==205349 and r['Effect']==16 and r['EffectMiscValue1']==41246 for r in effects)
assert any(r['SpellID']==197461 and r['Effect']==252 and r['ImplicitTarget2']==17 for r in effects)
assert 1638 in d.rows('SceneScriptPackage') and 9180 in d.rows('Phase')
members=sorted([r for r in d.rows('SceneScriptPackageMember').values() if r['SceneScriptPackageID']==1638],key=lambda r:r['OrderIndex'])
script_rows=d.rows('SceneScript');b=(d.dbc/'SceneScriptText.db2').read_bytes();u=lambda p:struct.unpack_from('<I',b,p)[0]
# Sparse WDC1 catalog is the same offset/size lookup used by DB2FileLoaderSparseImpl.
assert b[:4]==b'WDC1' and struct.unpack_from('<H',b,44)[0]&1
scripts={};metadata=[]
for member in members:
 id=member['SceneScriptID'];seen=set()
 while id:
  assert id not in seen;seen.add(id)
  off,size=struct.unpack_from('<IH',b,u(60)+(id-u(28))*6)
  assert off and off+size<=u(60)
  name,script,tail=b[off:off+size].decode().split('\0',2);assert not tail.strip('\0')
  scripts[id]=script.replace('\r','')
  metadata.append(dict(id=id,name=name,next=script_rows[id]['NextSceneScriptID'],sha256=hashlib.sha256(b[off:off+size]).hexdigest()))
  id=script_rows[id]['NextSceneScriptID']
assert 'SendTrigger( "START" )' in scripts[14151] # Continuation of init 14150 must be read.
assert 'dargrulCreatureID = 107704' in scripts[14150]
assert all('TriggerServerEvent( "'+name+'" )' in scripts[14152] for name in ('IMPACT','EXPLOSION'))
assert all('TriggerServerEvent( "'+name+'" )' in scripts[14153] for name in ('TELEPORT05','SLEEP'))
assert 'scene:EndScene()' in scripts[14152]
d.hashes['SceneScriptText']=hashlib.sha256(b).hexdigest()
# Existing spell-data teleports use map 1220; geometry is checked separately on both swaps.
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objective':objective,'ender':data['creature_questender'],'tracking_quest':[r for r in data['quest_template'] if r['ID']=='41246'][0]['LogTitle'],'effects':effects,'package_members':members,'script_chunks':metadata,'native_events':['IMPACT','EXPLOSION','TELEPORT05','SLEEP'],'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-stands-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: TDB735 required confrontation/ender, scene 1287/package 1638 including continuation scripts, native callbacks, terrain tracking quest and teleport effect.')
