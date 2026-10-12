import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={42088};npcs={106115,106153,106191}
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
assert {(r['ID'],r['Type'],r['ObjectID'],r['Amount'],r['StorageIndex']) for r in objectives}=={(283606,0,106115,1,0),(283617,0,106153,1,1),(283622,0,106191,1,2)}
assert not data.get('creature_queststarter') # The original archive has no starter relation for this quest.
assert {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}=={(93846,42088)}
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/worm-support-no-overlays')
assert not d.sql.exists()
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (210246,210304)]
assert len(effects)==2 and all(r['Effect']==6 and r['EffectAura']==4 and r['ImplicitTarget1']==1 for r in effects)
visuals=[r for r in d.rows('SpellXSpellVisual').values() if r['SpellID'] in (210246,210304)]
assert {r['SpellID']:r['SpellVisualID'] for r in visuals}=={210246:55273,210304:55311}
assert all(r['SpellVisualID'] in d.rows('SpellVisual') for r in visuals)
t=Table(d.dbc/'BroadcastText.db2',-1,0x51BF0C33)
missing_broadcasts=[id for id in (109069,109076,109086,109107) if id not in t.all_ids]
assert missing_broadcasts==[109069,109076,109086,109107] # Served by the 26124 SQL hotfix records; tested by the data regression.
d.hashes['BroadcastText']=t.sha256
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'starters':data.get('creature_queststarter',[]),'enders':data['creature_questender'],'pickup_effects':effects,'pickup_visuals':visuals,'broadcasts_absent_from_base_db2':missing_broadcasts,'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-evacuation-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: original TDB735 three independent evacuation credits and Mayla turn-in; native pickup aura effects/visuals; dialogue is supplied separately by SQL hotfixes.')
