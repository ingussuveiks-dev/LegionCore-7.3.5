import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={40594};npcs={100711,100712}
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


objectives={int(r['ID']):r for r in data['quest_objectives']}
for id,storage,obj,amount,flags,weight in [(280942,0,100712,1,1,0),(284259,1,97241,100,92,1),(280933,2,97217,20,92,5),(284260,3,97498,20,92,5),(284382,-1,0,1,2,0)]:
 r=objectives[id]
 assert [int(r[k]) for k in ('StorageIndex','ObjectID','Amount','Flags','ProgressBarWeight')]==[storage,obj,amount,flags,weight]
assert data['creature_questender']==[{'id':'97407','quest':'40594'}]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/justice-no-overlays')
assert not d.sql.exists()
t=Table(d.dbc/'Spell.db2');names={id:t.name(i) for i,id in enumerate(t.ids)}
for dest,src in t.copy_pairs:names[dest]=names[src]
ids={198214,198216,213467,213474,198380,198382,213444,213442,195025,195024}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in ids]
def effect(id,kind,**values):return any(r['SpellID']==id and r['Effect']==kind and all(r[k]==v for k,v in values.items()) for r in effects)
assert effect(198216,28,EffectMiscValue1=100711,EffectMiscValue2=3054)
assert effect(198214,64,EffectTriggerSpell=198216)
assert effect(213467,179,EffectMiscValue1=7105) and effect(213474,165,EffectBasePoints=40)
assert effect(198380,32,EffectTriggerSpell=198382) and effect(198382,6,EffectAura=89,EffectBasePoints=5000)
assert effect(213444,32,EffectTriggerSpell=213442) and effect(213442,28,EffectMiscValue1=97508)
assert effect(195025,6,EffectAura=23,EffectAuraPeriod=2000,EffectTriggerSpell=195024)
assert effect(195024,10,EffectBasePoints=40,ImplicitTarget2=31)
vehicle=d.rows('Vehicle')[4512];seat=d.rows('VehicleSeat')[vehicle['SeatID1']]
assert seat['ID']==16161 and seat['Flags']&0x800 and seat['Flags']&0x20000000
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in ids]
cooldowns=[r for r in d.rows('SpellCooldowns').values() if r['SpellID'] in ids]
radii={id:d.rows('SpellRadius')[id] for id in {r[k] for r in effects for k in ('EffectRadiusIndex1','EffectRadiusIndex2') if r[k]}}
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':list(objectives.values()),'ender':data['creature_questender'],'spell_names':{id:names[id] for id in sorted(ids)},'effects':effects,'misc':misc,'cooldowns':cooldowns,'radii':radii,'vehicle':vehicle,'seat':seat,'summon_properties':d.rows('SummonProperties')[3054],'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-justice-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native TDB735 boarding/weighted objectives and ender; DB2 vehicle/seat, summon, bombing and actual Healing Stream chain.')
