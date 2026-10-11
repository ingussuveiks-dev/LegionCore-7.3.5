import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39455,39860};npcs={97539,97544,93836}
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
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (192443,195789)]
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives'] if int(r['QuestID'])==39455]
assert len(objectives)==2
assert {r['ObjectID']:(r['Type'],r['StorageIndex'],r['Amount'],r['Flags']) for r in objectives}=={244564:(2,0,1,0),97544:(0,1,1,3)}
altar=data['gameobject_template'][0];assert int(altar['Data0'])==1691 and int(altar['Data20'])==1
assert any(r['SpellID']==192443 and r['Effect']==28 and r['EffectMiscValue1']==97544 and r['EffectMiscValue2']==3640 and r['ImplicitTarget1']==142 for r in fx)
props=d.rows('SummonProperties')[3640];assert (props['Control'],props['Title'],props['Slot'])==(0,0,0)
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID']==192443]
duration=d.rows('SpellDuration')[misc[0]['DurationIndex']];assert duration['Duration']==120000
assert any(r['SpellID']==195789 and r['Effect']==2 for r in fx) and any(r['SpellID']==195789 and r['EffectAura']==3 for r in fx)
witch=next(r for r in data['creature_template'] if r['entry']=='97539');assert int(witch['modelid1'])==64303
challenger=next(r for r in data['creature_template'] if r['entry']=='97544');assert int(challenger['HealthScalingExpansion'])==6
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'altar':altar,'witch':witch,'challenger':challenger,'effects':fx,'summon_properties':props,'misc':misc,'duration':duration,'hashes':d.hashes,'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[])}
evidence=ROOT/'docs/audits/highmountain-blood-trial-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: TDB735 objectives/altar/model/scaling, native 192443 -> 97544 wild summon (no pet slot), 120-second duration and Burning Slash effects.')
