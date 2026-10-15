"""Native Helheim disguise/effect/visual/model evidence; no client playback claim."""
import re,json,hashlib,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_template_addon','quest_objectives','gameobject_template','gameobject_loot_template','creature_template','creature_loot_template','creature_queststarter','creature_questender'}
cols={};records={};table=None
for line in archive.open(encoding='utf8'):
 m=re.match(r'CREATE TABLE `([^`]+)`',line)
 if m:table=m[1] if m[1] in selected else None;cols[table]=[]
 elif table:
  if line.startswith(')'):table=None
  else:
   m=re.match(r'  `([^`]+)`',line)
   if m:cols[table].append(m[1])
 m=re.match(r'INSERT INTO `([^`]+)`',line)
 if not m or m[1] not in selected:continue
 n=m[1]
 for mt in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
  r=dict(zip(cols[n],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
  key=int(next((r[k] for k in ('QuestID','quest','ID','entry','Entry') if k in r),'0'))
  if (n.startswith('quest_') and key in (38347,39848)) or (n in ('creature_queststarter','creature_questender') and key in(38347,39848)) or (n=='creature_template' and key in(91386,97419,97480)):records.setdefault(n,[]).append(r)
obj=next(r for r in records['quest_objectives'] if r['QuestID']=='38347')
assert all(obj[k]==v for k,v in {'ID':'277732','Type':'0','StorageIndex':'1','ObjectID':'91386','Amount':'1','Flags':'0'}.items())
records['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']]
records['creature_template']=[{k:r[k] for k in ('entry','name','modelid1','faction','npcflag','AIName','ScriptName')} for r in records['creature_template']]
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={181705,181709,192299,192298,81040}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual','SpellTargetRestrictions')}
assert any(r['SpellID']==181705 and r['Effect']==64 and r['EffectTriggerSpell']==181709 for r in native['SpellEffect'])
assert any(r['SpellID']==181709 and r['EffectAura']==139 and r['EffectMiscValue1']==14 and r['EffectBasePoints']==3 for r in native['SpellEffect'])
assert any(r['SpellID']==192299 and r['EffectAura']==428 and r['EffectTriggerSpell']==192298 for r in native['SpellEffect'])
assert any(r['SpellID']==192299 and r['EffectTriggerSpell']==81040 for r in native['SpellEffect'])
assert any(r['SpellID']==192298 and r['Effect']==28 and r['EffectMiscValue1']==97419 and r['EffectMiscValue2']==3281 for r in native['SpellEffect'])
assert not any(r['SpellID']==81040 for r in native['SpellEffect'])
native['SummonProperties3281']=d.rows('SummonProperties')[3281]
native['durations']=[d.rows('SpellDuration')[i] for i in (560,21,42)]
assert native['SummonProperties3281']['Slot']==6 and native['SummonProperties3281']['Flags']&512
assert [r['Duration'] for r in native['durations']]==[299000,-1,3600000]
from audit_spell_visuals import Table
def raw(name,fields):
 meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
 index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
 t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
 return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent']==46255]
kits={r['kit'] for r in events}
effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attached=[attachments[r['effect']] for r in effects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attached]
assert events and effects
native.update(visual_events=events,visual_kits=[d.rows('SpellVisualKit')[k] for k in sorted(kits)],kit_effects=effects,attachments=attached,effect_models=models)
records.update(archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),native=native,runtime_hashes=d.hashes,limits='Native objective is a91386 passage, not a kill;181705 is a dummy aura/visual and181709 supplies neutral reaction to faction14.192299 controller invokes192298 guardian-slot6 plus absent native81040. The implementation deliberately uses a private wild97419 follow actor, not that native guardian/controller; actual follow spacing and40yd approach-side sampling are authored. Source/native kit records prove a linked visual chain, not model playback or full retail choreography. Native C++ DB2Storage separately checks copied spell names,81040 absence and actor models/faction.')
path=ROOT/'docs/audits/stormheim-helheim-disguise-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(records,indent=2)+'\n')
else:assert json.loads(json.dumps(records))==json.loads(path.read_text())
print('PASS: native38347 objective,181705 ->181709 faction14/rank3,299s/infinite durations, controller192299 -> guardian192298/slot6/deleted81040; visual46255 kit effects:',effects,'models:',models)
