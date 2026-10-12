import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={38915,39776};npcs={93846,97013,99217,99216}
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
assert all(r['QuestID']==39776 for r in objectives), '38915 must have no numeric objective'
assert {r['ID']:(r['Type'],r['StorageIndex'],r['ObjectID'],r['Amount'],r['Flags'],r['Flags2']) for r in objectives}=={279939:(0,0,97013,1,1,0),280504:(0,1,99217,1,1,0),280505:(0,2,99216,1,1,0)}
assert {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}=={(93846,38915),(93846,39776)}
quests={int(r['ID']):{k:r[k] for k in ('LogTitle','LogDescription','QuestDescription','RewardNextQuest')} for r in data['quest_template']}
assert int(quests[38915]['RewardNextQuest'])==39776 and int(quests[39776]['RewardNextQuest'])==42088
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/snowblind-no-overlays')
assert not d.sql.exists()
ids={197168,197173,197232,197238,197252,197253,223971,213433,213439,213440,195144,195145,195146,195148,195149,195150}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in ids]
fx={(r['SpellID'],r['EffectIndex']):r for r in effects}
assert fx[197232,0]['ImplicitTarget1']==25 and fx[197232,0]['EffectAura']==87
assert fx[197232,1]['ImplicitTarget1']==25 and fx[197232,1]['EffectAura']==89
assert fx[197252,0]['Effect']==77 and fx[197252,0]['EffectBasePoints']==197253 and fx[197252,0]['ImplicitTarget1']==6
assert fx[197253,0]['Effect']==42 and fx[197253,0]['ImplicitTarget1']==63 and fx[197253,0]['ImplicitTarget2']==64
assert fx[197253,0]['EffectMiscValue2']==150
for spell in (197168,197173):assert fx[spell,0]['ImplicitTarget1']==6
assert fx[223971,0]['EffectAura']==129
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID']==197232]
assert len(misc)==1 and misc[0]['DurationIndex']==21
duration=d.rows('SpellDuration')[21]
assert duration['Duration']==-1
# AggressorAI handles this permanent aura in EnterCombat(who), not DoCast(id).
overrides=[d.rows('OverrideSpellData')[i] for i in (634,635,636)]
assert [r['Spells1'] for r in overrides]==[195144,195146,195149]
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'quests':quests,'objectives':objectives,'enders':data['creature_questender'],'effects':effects,'naxa_misc':misc,'naxa_duration':duration,'support_override_bars':overrides,'hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-battle-worms-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native journey without numeric objectives; three distinct worm kills; Mayla turn-ins; next quest; explicit Choking Stream target and Hooked Tongue jump child.')