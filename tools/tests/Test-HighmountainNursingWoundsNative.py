import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={38913,39318,38910};npcs={93841,95261,95403}
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
assert {r['ID']:(r['Type'],r['StorageIndex'],r['ObjectID'],r['Amount'],r['Flags'],r['Flags2']) for r in objectives}=={279202:(0,0,95261,8,2,0),280335:(0,1,95403,1,0,0),279198:(0,0,94991,7,0,0)}
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert [int(quests[n]['RewardNextQuest']) for n in (38913,39318,38910)]==[39318,38910,39321]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={188911,189168,189182}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
assert any(r['SpellID']==189182 and r['EffectAura']==293 and r['EffectMiscValue1']==603 for r in fx)
assert any(r['SpellID']==189168 and r['Effect']==3 for r in fx)
bar=d.rows('OverrideSpellData')[603]
assert bar['Spells1']==189168
line=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in ids],key=lambda r:r['OrderIndex'])
assert [(r['QuestLineID'],r['QuestID'],r['OrderIndex']) for r in line]==[(93,38913,0),(93,39318,1),(93,38910,2)]
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
spellnames=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:spellnames[dest]=spellnames[source]
assert [spellnames[n] for n in (188911,189168,189182)]==['Skyhorn Fire','Suppressing Wind','Totem of Winds']
assert (93841,39318) in {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'enders':data['creature_questender'],'quests':quests,'quest_line':line,'effects':fx,'override_bar':bar,'spell_names':{n:spellnames[n] for n in sorted(spells)},'hashes':dict(d.hashes,Spell=t.sha256)}
evidence=ROOT/'docs/audits/highmountain-nursing-wounds-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Skyhorn branch order/links, Julan and eight-fire objectives, Totem of Winds override bar and Suppressing Wind effect.')
