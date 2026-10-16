"""39855 native evidence, with an explicit boundary between data and playback."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={192470,192471,212916,212917,212918,216818,192670,192671,192672,241140}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
for spell,conversation in ((212916,2257),(212917,2258),(212918,2259)):
    assert any(r['SpellID']==spell and r['Effect']==219 and r['EffectMiscValue1']==conversation and r['ImplicitTarget1']==18 for r in native['SpellEffect'])
assert {r['EffectTriggerSpell'] for r in native['SpellEffect'] if r['SpellID']==192670 and r['Effect']==64}=={192671,192672}
assert {r['EffectAura'] for r in native['SpellEffect'] if r['SpellID']==192671}=={12,26}
assert any(r['SpellID']==192471 and r['EffectTriggerSpell']==81040 for r in native['SpellEffect'])
assert any(r['SpellID']==192470 and r['EffectMiscValue1']==97558 and r['EffectMiscValue2']==3281 for r in native['SpellEffect'])
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
visual_ids={r['SpellVisualID'] for r in native['SpellXSpellVisual'] if r['SpellID']==216818}
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent'] in visual_ids]
kits={r['kit'] for r in events}
effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attached=[attachments[r['effect']] for r in effects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attached]
animations=[d.rows('SpellVisualAnim')[i] for i in sorted({r['effect'] for r in effects if r['type']==1})]
kit_ids={r['AnimKitID'] for r in animations if r['AnimKitID']}
assert visual_ids and {r['parent'] for r in events}==visual_ids
assert all(r['model_file'] or r['texture_file'] for r in models)
native.update(visual_events=events,kits=[d.rows('SpellVisualKit')[i] for i in sorted(kits)],kit_effects=effects,attachments=attached,effect_models=models,visual_animations=animations,animation_kits=[d.rows('AnimKit')[i] for i in sorted(kit_ids)])
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected={'quest_template','quest_template_addon','quest_objectives','creature_queststarter','creature_questender'}
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
        key=int(next((r[k] for k in ('QuestID','quest','ID') if k in r),'0'))
        if key==39855:original.setdefault(n,[]).append(r)
objective=original['quest_objectives'][0]
assert all(objective[k]==v for k,v in {'ID':'280035','Type':'0','StorageIndex':'0','ObjectID':'97487','Amount':'1','Flags':'0'}.items())
original['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in original['quest_template']]
out=ROOT/'build-extractors/runtime-test/stormheim-lifeblood'
record={'quest':39855,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,'native':native,'runtime_hashes':d.hashes,'native_cpp_catalog':(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'limits':'The final conversations2257/2258/2259, their native durations8823/4978/6396 and Ashildir group3/trap216818 are composed into an authored server sequence. Their exact retail order, Ashildir/Helya movement and trap/voice/camera playback are not client verified. The regular26972 parser traces trap visual->event->kit->attachment/model/animation references only; the native C++ reader checks sparse ConversationLine/model/sound/package records independently. The old installed direct TeleportTo destination is preserved instead of casting192670/192671/192672 scene/root/stun without a proven scene completion handler. Credit is written only after the acknowledged actual arrival of the current in-memory request. Reconnect in Helheim restarts the optional sequence. An incomplete login at the exact saved Stormheim exit restarts at the altar, without inferring credit or restoring an exact story stage; completed/rewarded quests, other locations and later phases stay put. Client cold arrival, pet portrait, movement, story and Havi package reward remain pending.'}
path=ROOT/'docs/audits/stormheim-lifeblood-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2)+'\n')
else:assert record==json.loads(path.read_text())
print('PASS: original39855 story/objective, native conversation and guardian/scene chains, trap visual/kit/attachment/model references. Retail choreography and client playback remain unverified.')
