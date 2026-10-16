"""Native Sundered spells and visual relationships, without client playback claims."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={192493,192495,192500,192214,192485,192486,192195,192196,192198,192200,212902,212903,212904}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
assert any(r['SpellID']==192493 and r['Effect']==3 and r['ImplicitTarget1']==25 for r in native['SpellEffect'])
assert {r['EffectTriggerSpell'] for r in native['SpellEffect'] if r['SpellID']==192495 and r['Effect']==203}=={192195,192196,192198,192200}
native['cast_time']=d.rows('SpellCastTimes')[16]
assert native['cast_time']['Base']==1500
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
visual_ids={49786,49866,49867,49840,49868}
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
assert {r['ID'] for r in animations}=={8256,8284,8285}
assert {r['attachment'] for r in attached if r['parent']==59145}=={1,2,47,48}
assert all(r['model_file'] or r['texture_file'] for r in models)
native.update(visual_events=events,kits=[d.rows('SpellVisualKit')[i] for i in sorted(kits)],kit_effects=effects,attachments=attached,effect_models=models,visual_animations=animations,animation_kits=[d.rows('AnimKit')[i] for i in sorted(kit_ids)],segments=segments)
previous=json.loads((ROOT/'docs/audits/stormheim-light-way-native-2026-10-16.json').read_text())
original={k:[r for r in previous[k] if r.get('QuestID',r.get('ID'))=='39850'] for k in ('quest_template','quest_objectives')}
out=ROOT/'build-extractors/runtime-test/stormheim-sundered'
record={'quest':39850,'archive_sha256':previous['archive_sha256'],'original':original,'native':native,'runtime_hashes':d.hashes,'native_cpp_catalog':(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'limits':'Regular WDC1 visual relationships/attachments/effect model file IDs and pose animation kits are evidence of native references, not M2 playback. The standalone native C++ visual-store attempt failed (disabled/incomplete visual LoadInfo); the supported native C++ catalog checks spells, effects, range, links and creature model, while the established regular26972 reader handles visual relationships. No runtime DB2 files or shared metadata were changed. Native lantern range is500; the authored interaction restricts actual nearby clicked actors to10yd and current quest acceptance. Private five-second release copies and falling are authored; actual rendering/LOS/motion and full retail choreography are pending client checks.'}
path=ROOT/'docs/audits/stormheim-sundered-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2)+'\n')
else:assert record==json.loads(path.read_text())
print('PASS: native Sundered lantern1500ms/ANY25, release removes four chains, pose visual animations8256/8284/8285 and kit11590, break-chain attachments1/2/47/48 and model references. Client checks pending.')
