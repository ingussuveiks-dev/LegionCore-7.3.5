import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={38910,39321};npcs={93841,94991,95410}
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
assert next(r for r in objectives if r['QuestID']==38910)==dict(ID=279198,QuestID=38910,Type=0,StorageIndex=0,ObjectID=94991,Amount=7,Flags=0,Flags2=0)
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[38910]['RewardNextQuest'])==39321
assert (95410,38910) in {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={188432,188434}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
assert any(r['SpellID']==188432 and r['Effect']==3 for r in fx)
assert any(r['SpellID']==188434 and r['Effect']==6 and r['EffectAura']==4 for r in fx)
line=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in ids],key=lambda r:r['OrderIndex'])
assert [(r['QuestLineID'],r['QuestID'],r['OrderIndex']) for r in line]==[(93,38910,2),(93,39321,3)]
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
spellnames=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:spellnames[dest]=spellnames[source]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'enders':data['creature_questender'],'quests':quests,'quest_line':line,'effects':fx,'spell_names':{n:spellnames[n] for n in sorted(spells)},'hashes':dict(d.hashes,Spell=t.sha256)}
evidence=ROOT/'docs/audits/highmountain-rocs-eagles-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native seven-eagle objective, Lasan ender, next quest and release/captive aura effects. Names:',out['spell_names'])
