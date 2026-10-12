import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={38910,39321,39429,39322};npcs={95194,95195,95196,95410,95265,95266,95277,105068}
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
assert {r['ID']:(r['StorageIndex'],r['ObjectID'],r['Amount'],r['Flags']) for r in objectives if r['QuestID']==39321}=={279205:(0,95194,1,1),279216:(1,95195,1,1),279217:(2,95196,1,1)}
credits={int(r['entry']):[int(r['KillCredit1']),int(r['KillCredit2'])] for r in data['creature_template']}
assert all(credits[n]==[95266,0] for n in (95265,105068))
assert all(credits[n]==[0,0] for n in (95194,95195,95196,95266,95277))
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[38910]['RewardNextQuest'])==39321
assert all((95410,q) in {(int(r['id']),int(r['quest'])) for r in data['creature_questender']} for q in (39321,39429))
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={192111,213273,194228,194245,191955,215591,194241,215583,215588,215592,215593}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
assert any(r['SpellID']==192111 and r['EffectAura']==56 and r['ImplicitTarget2']==15 for r in fx)
assert any(r['SpellID']==194228 and r['EffectAura']==23 and r['EffectTriggerSpell']==194241 for r in fx)
assert any(r['SpellID']==191955 and r['EffectAura']==23 and r['EffectTriggerSpell']==215591 for r in fx)
assert all(any(r['SpellID']==n for r in fx) for n in spells)
assert any(r['SpellID']==194241 and r['Effect']==3 and r['EffectBasePoints']==215588 for r in fx)
assert any(r['SpellID']==215588 and r['Effect']==2 for r in fx)
assert any(r['SpellID']==215591 and r['Effect']==3 and r['EffectBasePoints']==215592 for r in fx)
assert any(r['SpellID']==215592 and r['Effect']==32 and r['EffectTriggerSpell']==215593 for r in fx)
assert {r['Effect'] for r in fx if r['SpellID']==215593}=={2,98}
line=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in ids],key=lambda r:r['OrderIndex'])
t=Table(ROOT/'build-extractors/bin/Release/dbc/enUS/Spell.db2',layout=0x2273DFFF)
spellnames=dict(zip(t.ids,(t.name(i) for i in range(t.count))))
for dest,source in t.copy_pairs:spellnames[dest]=spellnames[source]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'credits':credits,'enders':data['creature_questender'],'quests':quests,'quest_line':line,'effects':fx,'spell_names':{n:spellnames[n] for n in sorted(spells)},'hashes':dict(d.hashes,Spell=t.sha256)}
evidence=ROOT/'docs/audits/highmountain-the-three-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native three kill objectives, Haglands enders, combat/aura effects and existing triggered spell references. Names:',out['spell_names'])
