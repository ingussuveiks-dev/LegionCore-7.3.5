import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39781,42454};npcs={97304,91007}
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


objectives={int(r['ID']):r for r in data['quest_objectives']}
for id,kind,storage,obj,flags,flags2 in [(279938,1,0,128765,1,1),(279973,3,1,97304,4,0)]:
 assert [int(objectives[id][k]) for k in ('Type','StorageIndex','ObjectID','Amount','Flags','Flags2')]==[kind,storage,obj,1,flags,flags2]
assert {'id':'250548','quest':'39781'} in data['gameobject_questender']
assert {'id':'250548','quest':'42454'} in data['gameobject_queststarter']
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':list(objectives.values()),'ender':data['gameobject_questender'],'starter':data['gameobject_queststarter'],'item_probe':'Test-BloodtotemItemNative.ps1 uses the production sparse DB2 reader for item 128765: MaxCount=0, Stackable=50, Bonding=4.', 'db2_hashes':{'ItemSparse':hashlib.sha256((ROOT/'build-extractors/bin/Release/dbc/enUS/ItemSparse.db2').read_bytes()).hexdigest()}}
path=ROOT/'docs/audits/highmountain-lair-native-2026-10-13.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: TDB735 mandatory hammer item and optional Navarrogg objective, native quest-giver/next quest; installed ItemSparse fingerprint.')
