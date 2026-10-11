import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39860,39381};npcs={95259,97418,97440,97675,96286}
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
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (190902,192482,192484,192536,192682,212758,212762,197670,197681)]
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives'] if int(r['QuestID'])==39860]
assert len(objectives)==2
assert {r['ObjectID']:(r['Type'],r['StorageIndex'],r['Amount'],r['Flags']) for r in objectives}=={95259:(0,0,1,0),97418:(0,1,1,3)}
for spell,entry,prop in [(190902,97440,3686),(192536,96286,3699)]:
 assert any(r['SpellID']==spell and r['Effect']==28 and r['EffectMiscValue1']==entry and r['EffectMiscValue2']==prop for r in fx)
 props=d.rows('SummonProperties')[prop];assert (props['Control'],props['Title'],props['Slot'])==(1,0,0);assert props['Flags'] & 512
assert any(r['SpellID']==192484 and r['Effect']==90 and r['EffectMiscValue1']==95259 for r in fx)
assert any(r['SpellID']==192682 and r['EffectAura']==4 and r['EffectMiscValue1']==97675 for r in fx)
assert any(r['SpellID']==212758 and r['Effect']==3 for r in fx)
assert any(r['SpellID']==212762 and r['Effect']==219 and r['EffectMiscValue1']==2244 for r in fx)
assert any(r['SpellID']==197670 and r['Effect']==2 for r in fx)
assert any(r['SpellID']==197681 and r['EffectAura']==12 for r in fx)
assert any(int(r['id'])==96286 and int(r['quest'])==39860 for r in data['creature_questender'])
# TDB735 has no starter row for 39381; retain and verify the live 96286 relation in the SQL test.
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'effects':fx,'summon_properties':[d.rows('SummonProperties')[n] for n in (3686,3699)],'hashes':d.hashes,'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[]),'templates':data.get('creature_template',[])}
evidence=ROOT/'docs/audits/highmountain-blood-rite-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: TDB735 ordered objectives and turn-in/next NPC, DB2 companion/guide ally guardians with slot zero, portal aura, sacrifice, conversation and boss combat effects.')
