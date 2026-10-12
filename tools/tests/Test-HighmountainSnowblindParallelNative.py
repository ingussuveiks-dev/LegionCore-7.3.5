import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39777,39862};npcs={96690,96691,97546}
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
assert {r['QuestID']:(r['Type'],r['ObjectID'],r['Amount'],r['StorageIndex']) for r in objectives}=={39777:(0,96690,10,0),39862:(0,97546,1,0)}
assert {(int(r['id']),int(r['quest'])) for r in data['creature_questender']}=={(97344,39777),(97301,39862)}
credits={int(r['entry']):[int(r['KillCredit1']),int(r['KillCredit2'])] for r in data['creature_template']}
assert credits=={96690:[0,0],96691:[0,96690],97546:[0,0]}
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/worm-support-no-overlays')
assert not d.sql.exists()
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in (194019,196402,197483)]
assert len(effects)==3
assert all(r['ImplicitTarget1']==25 and r['Effect'] in (2,31) for r in effects)
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in (194019,196402,197483)]
assert len(misc)==3 and all(r['DurationIndex']==0 for r in misc)
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':objectives,'kill_aliases':credits,'enders':data['creature_questender'],'attack_effects':effects,'attack_misc':misc,'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-snowblind-parallel-native-2026-10-12.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: original TDB735 ten Mightstone kills with Slinger alias, one Olgrul kill and Jale/Navarrogg turn-ins.')
