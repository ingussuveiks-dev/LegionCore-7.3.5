import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={38909,38912,38913,40515,39579};npcs={93805,93826,94547,95078}
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
objectives=[{k:int(v) for k,v in r.items() if k in ('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2')} for r in data['quest_objectives'] if r['QuestID']=='38909']
assert {r['ObjectID']:(r['Type'],r['StorageIndex'],r['Amount'],r['Flags']) for r in objectives}=={94547:(0,0,1,1),95078:(0,1,1,3)}
assert next(r for r in data['quest_template'] if r['ID']=='38909')['RewardNextQuest']=='38912'
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID']==188583]
assert any(r['Effect']==28 and r['EffectMiscValue1']==95078 and r['EffectMiscValue2']==3679 for r in fx)
props=d.rows('SummonProperties')[3679];assert (props['Control'],props['Faction'],props['Title'],props['Slot'])==(1,1665,0,0) and props['Flags'] & 512
lines=[r for r in d.rows('QuestLineXQuest').values() if r['QuestID'] in ids]
out={'quests':{r['ID']:{k:r[k] for k in ('LogTitle','LogDescription','QuestDescription','RewardNextQuest','Flags')} for r in data['quest_template']},'objectives':objectives,'starters':data.get('creature_queststarter',[]),'enders':data.get('creature_questender',[]),'spell_effects':fx,'summon_properties':props,'quest_lines':lines,'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'runtime_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-highground-native-2026-10-11.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert out==json.loads(evidence.read_text())
print('PASS: native 38909 objectives/slots/flags, 188583 summon/95078 companion, uncontrolled properties and 38912 reward link.')
print('Quest lines:',lines)
