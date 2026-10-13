import re,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
ids={39780};npcs={97266,97352}
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
for id,storage,obj,flags in [(279965,0,97266,3),(280018,1,97352,1)]:
 r=objectives[id]
 assert [int(r[k]) for k in ('Type','StorageIndex','ObjectID','Amount','Flags')]==[0,storage,obj,1,flags]
assert data['creature_questender']==[{'id':'93846','quest':'39780'}]
sys.dont_write_bytecode=True
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/underking-no-overlays')
assert not d.sql.exists()
ids={192269,192270,192322,216101,213577}
effects=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in ids]
def effect(id,kind,**values):return any(r['SpellID']==id and r['Effect']==kind and all(r[k]==v for k,v in values.items()) for r in effects)
assert effect(192269,6,EffectAura=23,EffectAuraPeriod=1000,EffectTriggerSpell=192270)
assert effect(192270,6,EffectAura=33,EffectBasePoints=5)
assert effect(192270,160,EffectTriggerSpell=192322,EffectIndex=1)
assert effect(192270,160,EffectTriggerSpell=216101,EffectIndex=2)
assert effect(192270,2,EffectBasePoints=110)
for id in (192322,216101):
 assert effect(id,6,EffectAura=12) and effect(id,164,EffectTriggerSpell=192270)
assert effect(213577,2,EffectBasePoints=300) and effect(213577,98,EffectBasePoints=150)
options=[r for r in d.rows('SpellAuraOptions').values() if r['SpellID'] in ids]
assert next(r for r in options if r['SpellID']==192270)['CumulativeAura']==10
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in ids]
durations={id:d.rows('SpellDuration')[id] for id in {r['DurationIndex'] for r in misc if r['DurationIndex']}}
radii={id:d.rows('SpellRadius')[id] for id in {r[k] for r in effects for k in ('EffectRadiusIndex1','EffectRadiusIndex2') if r[k]}}
# Core dispatch must still execute trigger effects before hit/aura stack handling.
core=(ROOT/'src/server/game/Spells/SpellEffects.cpp').read_text()
assert '&Spell::EffectTriggerSpell,                             //160' in core
hook=core.split('void Spell::EffectTriggerSpell(SpellEffIndex effIndex)')[1].split('// todo:')[0]
assert 'SPELL_EFFECT_HANDLE_LAUNCH_TARGET' in hook and 'SPELL_EFFECT_HANDLE_LAUNCH' in hook
source=(ROOT/'src/server/scripts/Legion/highmountain_underking.cpp').read_text()
assert source.count('OnEffectLaunchTarget +=')==2 and 'AfterHit += SpellHitFn(spell_highmountain_underking_breath::After)' in source
out={'archive':p.name,'archive_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'objectives':list(objectives.values()),'ender':data['creature_questender'],'effects':effects,'misc':misc,'aura_options':options,'durations':durations,'radii':radii,'db2_hashes':d.hashes}
evidence=ROOT/'docs/audits/highmountain-underking-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: native Underking objectives/ender, breath stack cap and launch hooks, petrification effects/durations and Stone Smash.')
