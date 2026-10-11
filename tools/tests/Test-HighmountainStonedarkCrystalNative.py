import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39425};npcs={95290,96307,96423,96615,104323,104328}
p=ROOT/Path('.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql');names={'quest_template','quest_objectives','creature_template','creature_queststarter','creature_questender','quest_template_addon','creature_loot_template'};cols={};data={};table=None
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
  keep=int(r.get('Item',0))==128343 if name=='creature_loot_template' else int(r.get('entry',0)) in npcs if name=='creature_template' else int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in ids
  if keep:data.setdefault(name,[]).append(r)
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']]
assert objectives==[dict(ID=279391,QuestID=39425,Type=1,StorageIndex=0,ObjectID=128343,Amount=1,Flags=0,Flags2=1)]
assert any(int(r['id'])==95421 for r in data['creature_questender'])
assert int(data['quest_template'][0]['RewardNextQuest'])==39426
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'starters':data.get('creature_queststarter',[]),'enders':data['creature_questender'],'quests':data['quest_template'],'loot':data.get('creature_loot_template',[]),'templates':data.get('creature_template',[]),'ItemSparse_sha256':hashlib.sha256((ROOT/'build-extractors/bin/Release/dbc/enUS/ItemSparse.db2').read_bytes()).hexdigest()}
evidence=ROOT/'docs/audits/highmountain-stonedark-crystal-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: TDB735 Stonedark Focus objective, Navarrogg turn-in, reward link and native evidence hashes.')
