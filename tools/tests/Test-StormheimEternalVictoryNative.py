"""Native Victory is Eternal spells and visual relationships, without client playback claims."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={182866,182868,182877}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
assert any(r['SpellID']==182866 and r['EffectAura']==23 and r['EffectTriggerSpell']==182868 for r in native['SpellEffect'])
assert any(r['SpellID']==182877 and r['Effect']==2 and r['ImplicitTarget1']==47 for r in native['SpellEffect'])
native['cast_times']=[d.rows('SpellCastTimes')[i] for i in sorted({r['CastingTimeIndex'] for r in native['SpellMisc']})]
native['durations']=[d.rows('SpellDuration')[i] for i in sorted({r['DurationIndex'] for r in native['SpellMisc'] if r['DurationIndex']})]
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
visual_ids={r['SpellVisualID'] for r in native['SpellXSpellVisual']}
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent'] in visual_ids]
kits={r['kit'] for r in events}
effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attached=[attachments[r['effect']] for r in effects if r['type']==2]
# The core's disabled visual-store LoadInfo omits parent fields and differs
# from the actual effect-name layout. Use the previously audited26972 columns.
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attached]
anim_ids={r['effect'] for r in effects if r['type']==1}
animations=[d.rows('SpellVisualAnim')[i] for i in sorted(anim_ids)]
kit_ids={r['AnimKitID'] for r in animations if r['AnimKitID']}
segments=[r for r in d.rows('AnimKitSegment').values() if r['ParentAnimKitID'] in kit_ids]
assert {r['parent'] for r in events}==visual_ids
assert all(r['model_file'] or r['texture_file'] for r in models)
native.update(visual_events=events,kits=[d.rows('SpellVisualKit')[i] for i in sorted(kits)],kit_effects=effects,attachments=attached,effect_models=models,visual_animations=animations,animation_kits=[d.rows('AnimKit')[i] for i in sorted(kit_ids)],segments=segments)
previous=json.loads((ROOT/'docs/audits/stormheim-drowning-valkyra-native-2026-10-16.json').read_text())
original={k:[r for r in previous['original'][k] if r.get('QuestID',r.get('ID'))=='39853'] for k in ('quest_template','quest_objectives')}
out=ROOT/'build-extractors/runtime-test/stormheim-eternal-victory'
record={'quest':39853,'archive_sha256':previous['archive_sha256'],'original':original,'native':native,'runtime_hashes':d.hashes,'native_cpp_catalog':(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'limits':'Original39853 is a normal Geir91948 kill objective, with existing source0 SmartAI combat182866/182877 and optional death conversation2256. Native DB2 and regular26972 visual relationships establish spell/kit/model references, not timed combat or M2 playback. The supported C++ reader independently checks sparse line4752/broadcast98495, hostile faction1814, model24301 and female sound53006; the World/Hotfix overlay is checked separately. The repair changes only the eligible pet/guardian death-event invoker on the original spawn, using its home position to allow combat movement. Native kill selection/50ms credit/group reward and later Warrior spawns retain their original paths. No full client fight, tapping, loot distribution, voice or39855 exit choreography is verified.'}
path=ROOT/'docs/audits/stormheim-eternal-victory-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2)+'\n')
else:assert record==json.loads(path.read_text())
print('PASS: original39853 Geir kill objective and native Crushing Stomp182866 ->182868, Smash182877, actual visual/kit/effect/model references. Conversation/model/voice records checked by native C++ reader; client checks pending.')
