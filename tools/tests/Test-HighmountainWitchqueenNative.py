import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39321,39429,39322,39387};npcs={95153,95410}
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
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives'] if int(r['QuestID'])==39322]
assert objectives==[dict(ID=279206,QuestID=39322,Type=0,StorageIndex=0,ObjectID=95153,Amount=1,Flags=1,Flags2=0)]
queen=next(r for r in data['creature_template'] if int(r['entry'])==95153)
assert int(queen['KillCredit1'])==int(queen['KillCredit2'])==0
quest=next(r for r in data['quest_template'] if int(r['ID'])==39322)
assert int(quest['RewardNextQuest'])==39387
for table in ('creature_questender',):
 assert any(int(r['id'])==95410 and int(r['quest'])==39322 for r in data[table])
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={192173,192177,192179,192180,192181,192182,192183,192184,192287,166312}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
parent=sorted((r for r in fx if r['SpellID']==192177),key=lambda r:r['EffectIndex'])
assert [(r['EffectIndex'],r['Effect'],r['EffectBasePoints'],r['ImplicitTarget1']) for r in parent]==[(i,3,192179+i,1) for i in range(5)]
children=[r for r in fx if 192179<=r['SpellID']<=192183]
assert len(children)==5
assert all(r['Effect']==179 and r['EffectMiscValue1']==(4889 if r['SpellID']==192179 else 2736) for r in children)
assert any(r['SpellID']==192184 and r['Effect']==2 and r['EffectBasePoints']==200 and r['ImplicitTarget1']==25 for r in fx)
assert any(r['SpellID']==192287 and r['EffectAura']==3 and r['ImplicitTarget1']==25 for r in fx)
assert {r['Effect'] for r in fx if r['SpellID']==192173}=={2,98}
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in spells]
duration=d.rows('SpellDuration')[28]
assert duration['Duration']>0
assert all(r['DurationIndex']==28 for r in misc if 192179<=r['SpellID']<=192183)
line=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in ids],key=lambda r:r['OrderIndex'])
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
names=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:names[dest]=names[source]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'quest':quest,'quest_line':line,'effects':fx,'misc':misc,'cyclone_duration':duration,'spell_names':{n:names[n] for n in sorted(spells)},'hashes':dict(d.hashes,Spell=t.sha256)}
evidence=ROOT/'docs/audits/highmountain-witchqueen-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Witchqueen kill objective/turn-in/next quest, five exact cyclone children and finite lifetime, contact damage and offensive targets.')
