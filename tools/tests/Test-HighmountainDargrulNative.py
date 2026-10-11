import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39027};npcs={97552,97553,94610,97827,97781,97871,97901,93805}
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
assert len(objectives)==4
assert {r['ObjectID']:(r['Type'],r['Amount'],r['Flags']) for r in objectives}=={94610:(0,1,1),97871:(0,1,12),97781:(3,1,12),93805:(0,1,2)}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (205051,192467,192936,193100,193017)]
assert any(r['SpellID']==205051 and r['EffectAura']==428 and r['EffectTriggerSpell']==192467 for r in fx)
assert any(r['SpellID']==192467 and r['Effect']==28 and r['EffectMiscValue1']==97552 and r['EffectMiscValue2']==3054 for r in fx)
assert any(r['SpellID']==192936 and r['Effect']==28 and r['EffectMiscValue1']==97781 and r['EffectMiscValue2']==3035 for r in fx)
assert any(r['SpellID']==193017 and r['Effect']==165 and r['EffectBasePoints']==500 for r in fx)
assert any(r['SpellID']==193100 and r['Effect']==3 for r in fx)
v=d.rows('Vehicle')[4404];assert v['SeatID1']==15989 and not v['SeatID2']
props=[d.rows('SummonProperties')[i] for i in (3035,3054)];assert props[0]['Control']==4
out={'quests':data['quest_template'],'objectives':objectives,'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[]),'spell_effects':fx,'summon_properties':props,'vehicle':v,'seat':d.rows('VehicleSeat')[15989],'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'runtime_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-dargrul-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: native 39027 objectives, controller/escort, grasp vehicle/seat, hammer and 500-percent escape spell.')
