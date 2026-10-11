import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39391};npcs={95272,95273,95276}
p=ROOT/Path('.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql');names={'quest_template','quest_objectives','creature_template','creature_queststarter','creature_questender','quest_template_addon','gameobject_template'};cols={};data={};table=None
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
  keep=int(r.get('entry',0))==244564 if name=='gameobject_template' else int(r.get('entry',0)) in npcs if name=='creature_template' else int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in ids
  if keep:data.setdefault(name,[]).append(r)
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (189686,189687)]
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']]
assert {r['ObjectID']:(r['ID'],r['Type'],r['StorageIndex'],r['Amount'],r['Flags'],r['Flags2']) for r in objectives}=={95276:(279326,0,1,1,0,0),95272:(279327,0,2,1,0,0),95273:(279328,0,3,1,0,0)}
for spell in (189686,189687):
 assert any(r['SpellID']==spell and r['Effect']==3 and r['EffectIndex']==0 and r['ImplicitTarget1']==25 for r in fx)
assert any(int(r['id'])==95421 and int(r['quest'])==39391 for r in data['creature_questender'])
models={int(r['entry']):int(r['modelid1']) for r in data['creature_template']}
assert models=={95272:64366,95273:60098,95276:60098}
from audit_spell_visuals import Table
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
spellnames=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:spellnames[dest]=spellnames[source]
assert spellnames[189686]=='Rescuing' and spellnames[189687]=='Recovering'
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'effects':fx,'models':models,'spells':{n:spellnames[n] for n in (189686,189687)},'hashes':dict(d.hashes,Spell=t.sha256),'quests':data['quest_template'],'enders':data.get('creature_questender',[]),'templates':data['creature_template']}
evidence=ROOT/'docs/audits/highmountain-pet-rocks-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native three rescue/inspection objectives, models, spell effects/names and refuge ender.')
