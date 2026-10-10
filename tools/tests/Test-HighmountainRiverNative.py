"""Read-only 7.3.5 Highmountain opening evidence."""
import sys,re,json,hashlib
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
IDS={39316,39277,39614,39661}
p=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};data={};table=None
names=('quest_template','quest_objectives','creature_questender','gameobject_queststarter','gameobject_questender','quest_poi','quest_poi_points')
for line in p.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in names else None;columns[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:columns[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in names:continue
 name=m[1]
 for m in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  parts=re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",m[0][1:-1]);r=dict(zip(columns[name],parts))
  if int(r.get('QuestID',r.get('quest',r.get('ID',0)))) in IDS:data.setdefault(name,[]).append(r)
q={int(r['ID']):r for r in data['quest_template']}
assert set(q)==IDS and len(data['quest_objectives'])==3
assert q[39277]['QuestPackageID']=='18473' and q[39277]['StartItem']=='127988'
expected={(39316,2,1,243368,1),(39277,0,0,95017,6),(39614,0,0,95148,8)}
assert {(int(r['QuestID']),int(r['Type']),int(r['StorageIndex']),int(r['ObjectID']),int(r['Amount'])) for r in data['quest_objectives']}==expected
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
items=[r for r in d.rows('ItemEffect').values() if r['ItemID']==127988]
assert len(items)==1 and items[0]['SpellID']==188466 and items[0]['TriggerType']==0 and items[0]['Charges']==0
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in(188447,188465,188466,190421)]
assert any(r['SpellID']==188447 and r['Effect']==98 and r['ImplicitTarget1']==25 for r in fx)
assert any(r['SpellID']==188466 and r['EffectAura']==23 and r['EffectTriggerSpell']==188465 for r in fx)
assert any(r['SpellID']==190421 and r['Effect']==28 and r['EffectMiscValue1']==94688 and r['EffectMiscValue2']==3359 and r['ImplicitTarget1']==72 for r in fx)
summoners=[r for r in d.rows('SpellEffect').values() if r['Effect']==28 and r['EffectMiscValue1']==94688]
assert {r['SpellID'] for r in summoners}=={190421},summoners
props=d.rows('SummonProperties')[3359];assert props['Control']==1 and props['Slot']==0
visuals=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID'] in(188447,188465,188466)]
assert {r['SpellID'] for r in visuals}=={188447,188465,188466}
assert all(r['SpellVisualID'] in d.rows('SpellVisual') for r in visuals)
lines=sorted([r for r in d.rows('QuestLineXQuest').values() if r['QuestLineID']==144],key=lambda r:r['OrderIndex'])
out=dict(scope=sorted(IDS),quests={i:{k:v for k,v in r.items() if k in('ID','LogTitle','QuestPackageID','StartItem','Flags','RewardNextQuest')} for i,r in q.items()},
 objectives=[{k:int(v) for k,v in r.items() if k in('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives']],
 enders=data['creature_questender'],quest_lines=lines,item_effects=items,spell_effects=fx,summon_properties=props,visuals=visuals,runtime_hashes=d.hashes,archive=p.name,archive_sha256=hashlib.sha256(p.read_bytes()).hexdigest())
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/highmountain-river-native-2026-10-10.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS: native river objectives, enders, nonconsumable Bug Sprayer / periodic spray / allied targeted summon, only native grub summon spell, carp knockback and all three visual references.')
