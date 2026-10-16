"""Native Allies in Death spells and visual relationships, without client playback claims."""
import hashlib,json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={192387,192388}
native={t:[r for r in d.rows(t).values() if r.get('SpellID') in ids] for t in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
assert any(r['SpellID']==192387 and r['Effect']==3 and r['ImplicitTarget1']==25 for r in native['SpellEffect'])
native['cast_time']=d.rows('SpellCastTimes')[16]
assert native['cast_time']['Base']==1500
def raw(name,fields):
    meta=re.split('struct '+name+'Meta\n',d.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
    index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
    t=Table(d.dbc/(name+'.db2'),int(index),int(layout,16));d.hashes[name]=t.sha256
    return [dict(ID=t.ids[i],parent=t.parents.get(i,0),**{k:t.value(i,f) for k,f in fields.items()}) for i in range(t.count)]
visual_ids={49840,49841}
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
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
columns={};original={};table=None
for line in archive.open(encoding='utf8'):
    m=re.match(r'CREATE TABLE `([^`]+)`',line)
    if m:table=m[1] if m[1] in {'quest_template','quest_objectives','quest_template_addon'} else None;columns[table]=[]
    elif table:
        if line.startswith(')'):table=None
        else:
            m=re.match(r'  `([^`]+)`',line)
            if m:columns[table].append(m[1])
    m=re.match(r'INSERT INTO `([^`]+)`',line)
    if not m or m[1] not in columns or m[1] is None:continue
    name=m[1]
    for mt in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
        row=dict(zip(columns[name],re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+",mt[0][1:-1])))
        if row.get('QuestID',row.get('ID')) in ('39851','39853'):original.setdefault(name,[]).append(row)
original['quest_template']=[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in original['quest_template']]
assert any(r['QuestID']=='39851' and r['ObjectID']=='97472' and r['Amount']=='8' for r in original['quest_objectives'])
out=ROOT/'build-extractors/runtime-test/stormheim-drowning-valkyra'
record={'quest':39851,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'original':original,'native':native,'runtime_hashes':d.hashes,'native_cpp_catalog':(out/'native-lines.txt').read_text(encoding='utf-8-sig').splitlines(),'catalog_hashes':json.loads((out/'native-inputs.json').read_text(encoding='utf-8-sig')),'limits':'Native regular WDC1 visual/kit/effect/attachment/model/animation relationships are reference evidence, not M2 playback. The supported native C++ catalog checks spell names/effects/1500ms/range500/visual links/display68954. Disabled visual-store LoadInfo is incomplete; regular26972 relationship columns follow previous audits. Native base spawns, aura192388 and SAY group0 remain unchanged; personal hiding at4500ms and restoration18500ms retain the installed repeat cycle. This is an8-count repeatable rescue objective, not8 persisted unique actor IDs. Transient source cooldowns do not persist through logout/map/grid loss; the saved count does. Full client cast/pose/visibility/LOS/dialogue verification remains pending.'}
path=ROOT/'docs/audits/stormheim-drowning-valkyra-native-2026-10-16.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(record,indent=2)+'\n')
else:assert record==json.loads(path.read_text())
print('PASS: native39851 narrative/objective8, Releasing192387 ANY25/1500ms and Curse192388 visual49841; native visual kits/attachments/effect models/animation references. Client checks pending.')
