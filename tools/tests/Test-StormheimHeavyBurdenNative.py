"""Original40078 and actual26972 spell/carry references; no client playback claim."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={193563,193576,193577,227837}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellInterrupts','SpellAuraRestrictions','SpellTargetRestrictions','SpellXSpellVisual')}
assert any(r['SpellID']==193563 and r['EffectAura']==33 and r['EffectBasePoints']==75 for r in native['SpellEffect'])
assert any(r['SpellID']==193563 and r['EffectAura']==312 and r['EffectMiscValue1']==175 for r in native['SpellEffect'])
assert any(r['SpellID']==193576 and r['Effect']==134 and r['EffectMiscValue1']==98239 for r in native['SpellEffect'])
assert any(r['SpellID']==193577 and r['Effect']==160 and r['EffectTriggerSpell']==193576 for r in native['SpellEffect'])
assert any(r['SpellID']==227837 and r['Effect']==203 and r['EffectTriggerSpell']==193563 for r in native['SpellEffect'])
assert not native['SpellAuraRestrictions']
assert next(r for r in native['SpellInterrupts'] if r['SpellID']==193563)['AuraInterruptFlags1']==0x20000 # mount, not use/cast/movement
native['cast_times']=[d.rows('SpellCastTimes')[i] for i in sorted({r['CastingTimeIndex'] for r in native['SpellMisc']})]
assert d.rows('SpellCastTimes')[16]['Base']==1500
native['durations']=[d.rows('SpellDuration')[i] for i in sorted({r['DurationIndex'] for r in native['SpellMisc'] if r['DurationIndex']})]
native['carry_replacement_set']=d.rows('AnimReplacementSet')[175]
assert 175 not in d.rows('AnimKit')
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
native['carry_replacements']=[r for r in raw('AnimReplacement',{'src':0,'dst':1,'flags':2}) if r['parent']==175]
visuals={r['SpellVisualID'] for r in native['SpellXSpellVisual'] if r['SpellID']==193563}
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent'] in visuals]
kits={r['kit'] for r in events}
effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attached={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attachments=[attached[r['effect']] for r in effects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attachments]
assert visuals=={50154} and {r['parent'] for r in events}==visuals
assert all(r['model_file'] or r['texture_file'] for r in models)
native.update(visual_events=events,kits=[d.rows('SpellVisualKit')[i] for i in sorted(kits)],kit_effects=effects,attachments=attachments,effect_models=models)
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_objectives','creature_questender','gameobject_template','gameobject'}
cols={};original={};table=None
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
        key=int(next((r[k] for k in ('QuestID','quest','ID','entry') if k in r),'0'))
        if n=='gameobject':key=int(r['id'])
        if (n in ('quest_template','quest_objectives','creature_questender') and key==40078) or (n.startswith('gameobject') and key in(244867,244872)):original.setdefault(n,[]).append(r)
assert original['quest_objectives'][0]['ObjectID']=='98239'
original['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardSpell','RewardNextQuest')} for r in original['quest_template']]
original['gameobject_template']=[{k:r[k] for k in ('entry','type','displayId','name','size','Data1','Data10')} for r in original['gameobject_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-heavy-burden'
record={'quest':40078,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,'native':native,'runtime_hashes':d.hashes,'native_cpp_catalog':(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'limits':'Original TDB narrative and native QuestLine135 order10/11 establish39855 ->40078; installed Cooking185/1 is removed and PrevQuest39855 added for fresh acceptance. The missing barrel is an authored spawn at the native barrel POI3220,1555 and measured ground180.773, not a recovered retail spawn coordinate. Original spear position and all native spell data remain. The authored GO handler starts native193576 directly, retaining its1500ms cast and avoiding193577 delayed wrapper. Its own collection/acceptance/cast ticket controls completion; effects203/134 are suppressed only for that ticket, then the personal objective and self-owned aura cleanup run once. The native client-owned replacement set175 and actual visual/kit/attachment/model references are evidence only, not fish rendering, speed, pose or cast interruption in a client. AnimKit175 is absent, so the old shared312 server handler calling SetAnimKitId175 is ignored by native Unit validation; no generic aura/mover/animation code is changed. Havi reward227837 and next40001 are preserved. Client collection, carry, skewer, cancellation/reconnect and NPC handoff remain pending.'}
path=ROOT/'docs/audits/stormheim-heavy-burden-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2)+'\n')
else:assert record==json.loads(path.read_text())
print('PASS: original40078 story/objective/templates and native fish75% slow/312 replacement175, visual50154 references,1500ms193576/203/134,193577 wrapper and227837 cleanup. Actual client movement/visuals/casts remain pending.')
