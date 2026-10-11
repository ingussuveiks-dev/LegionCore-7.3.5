import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={42104,39025,39026,39043};npcs={100520,93805,97662,97553,94255,95051,94286,102274,94261,98653}
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
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']]
assert len(objectives)==7
assert {r['ObjectID']:r['Amount'] for r in objectives if r['QuestID']==39043}=={94261:3,98653:3}
assert {r['ObjectID']:r['Amount'] for r in objectives if r['QuestID']==42104}=={100520:10}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (210983,210891,192268)]
assert any(r['SpellID']==210983 and r['EffectAura']==293 and r['EffectMiscValue1']==702 for r in fx)
assert d.rows('OverrideSpellData')[702]['Spells1']==210891
assert d.rows('SpellRadius')[32]['Radius']==12
assert any(r['SpellID']==210891 and r['Effect']==3 and r['ImplicitTarget2']==8 and r['EffectRadiusIndex1']==32 for r in fx)
vehicle=d.rows('Vehicle')[4397]
assert vehicle['SeatID1']==15974 and not vehicle['SeatID2']
assert any(r['SpellID']==192268 and r['EffectMiscValue1']==1092 for r in fx)
out={'quests':{r['ID']:{k:r[k] for k in ('LogTitle','LogDescription','RewardNextQuest','StartItem')} for r in data['quest_template']},'objectives':objectives,
     'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[]),'spell_effects':fx,'override':d.rows('OverrideSpellData')[702],'radius':d.rows('SpellRadius')[32],'vehicle':vehicle,
     'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'runtime_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-riverbend-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: four native quests/seven objectives, 10 evacuations, 8 raiders, 3+3 cave kills, horn/action bar/radius, grasp vehicle seat and reward scene.')
