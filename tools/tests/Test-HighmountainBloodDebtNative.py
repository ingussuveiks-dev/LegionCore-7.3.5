import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39426,40229,39456};npcs={95707,96453,95392,95421}
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
assert next(r for r in objectives if r['QuestID']==39426)==dict(ID=279393,QuestID=39426,Type=0,StorageIndex=1,ObjectID=95707,Amount=1,Flags=1,Flags2=0)
assert not any(r['QuestID']==40229 for r in objectives)
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[39426]['RewardNextQuest'])==40229 and int(quests[40229]['RewardNextQuest'])==39456
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={191109,197427,197450,197500,197520,197515}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
assert any(r['SpellID']==191109 and r['Effect']==28 and r['EffectMiscValue1']==96453 and r['EffectMiscValue2']==3839 for r in fx)
assert any(r['SpellID']==197450 and r['Effect']==98 for r in fx)
assert any(r['SpellID']==197520 and r['EffectTriggerSpell']==197515 for r in fx)
assert any(r['SpellID']==197515 for r in fx)
properties=d.rows('SummonProperties')[3839]
assert properties['Control']==1 and properties['Slot']==0 and properties['Flags']==8522506
from audit_spell_visuals import Table
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
spellnames=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:spellnames[dest]=spellnames[source]
assert {spellnames[n] for n in (197427,197450,197500,197520)}=={'Fel Slash','Demonic Cleave','Raging Blood','Fel Rush'}
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'enders':data['creature_questender'],'quests':quests,'effects':fx,'summon_properties':properties,'spell_names':{n:spellnames[n] for n in spells},'hashes':dict(d.hashes,Spell=t.sha256)}
evidence=ROOT/'docs/audits/highmountain-blood-debt-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Torok objective, handoff quest, Unexpected Allies link, combat spell effects/names and companion summon.')
