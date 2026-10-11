import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39588};npcs={95290,95707,96307,96423,96615,100055,104323,104328,105065,105066}
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
assert objectives==[dict(ID=279704,QuestID=39588,Type=0,StorageIndex=0,ObjectID=95290,Amount=10,Flags=0,Flags2=0)]
credits={int(r['entry']):[int(r['KillCredit1']),int(r['KillCredit2'])] for r in data['creature_template']}
assert credits=={entry:([0,0] if entry in (95290,96307) else [95290,0]) for entry in npcs}
assert any(int(r['id'])==95421 for r in data['creature_questender'])
assert int(data['quest_template'][0]['RewardNextQuest'])==39426
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'credits':credits,'names':{int(r['entry']):r['name'] for r in data['creature_template']},'starters':data.get('creature_queststarter',[]),'enders':data['creature_questender'],'quests':{r['ID']:{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest','Flags','FlagsEx')} for r in data['quest_template']}}
evidence=ROOT/'docs/audits/highmountain-pay-blood-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: TDB735 ten-kill objective, all alternate credits, excluded cave template, Navarrogg turn-in and next quest.')
