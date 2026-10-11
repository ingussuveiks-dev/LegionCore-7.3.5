import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39381,39391,39425,39588};npcs={95421,96286}
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
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (190967,190982,190983)]
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives'] if int(r['QuestID'])==39381]
assert objectives==[{'ID':280082,'QuestID':39381,'Type':0,'StorageIndex':0,'ObjectID':96286,'Amount':1,'Flags':0,'Flags2':0}]
assert any(r['SpellID']==190967 and r['Effect']==90 and r['EffectMiscValue1']==96286 for r in fx)
assert any(r['SpellID']==190967 and r['EffectAura']==12 for r in fx)
assert any(r['SpellID']==190967 and r['EffectTriggerSpell']==190983 for r in fx)
assert any(r['SpellID']==190983 and r['Effect']==3 for r in fx)
assert any(r['SpellID']==190967 and r['EffectTriggerSpell']==190982 for r in fx)
assert not any(r['SpellID']==190982 for r in fx) # Native dangling trigger, no replacement guessed.
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in (190967,190983)]
duration=d.rows('SpellDuration')[next(r for r in misc if r['SpellID']==190967)['DurationIndex']]
assert duration['Duration']==1750
for quest in (39391,39425,39588):
 assert next(r for r in data['quest_template'] if int(r['ID'])==quest)['RewardNextQuest']=='39426'
assert any(int(r['id'])==95421 and int(r['quest'])==39381 for r in data['creature_questender'])
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'effects':fx,'duration':duration,'misc':misc,'hashes':d.hashes,'quests':data['quest_template'],'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[])}
evidence=ROOT/'docs/audits/highmountain-rock-escape-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: TDB735 talk objective/ender/three next links and native personal credit, stun, duration, child spell; missing 190982 explicitly recorded.')
