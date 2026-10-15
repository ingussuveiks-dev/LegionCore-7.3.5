"""Read original TDB735 awakening objective and native summon/visual records."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

QUESTS = {38815,38818}
archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template','quest_objectives','creature_questender','creature_queststarter','gameobject_template','gameobject_loot_template'}
columns, records, table = {}, {}, None
for line in archive.open(encoding='utf8'):
    match = re.match(r'CREATE TABLE `([^`]+)`', line)
    if match:
        table = match[1] if match[1] in selected else None
        columns[table] = []
    elif table:
        if line.startswith(')'):
            table = None
        else:
            match = re.match(r'  `([^`]+)`', line)
            if match:
                columns[table].append(match[1])
    match = re.match(r'INSERT INTO `([^`]+)`', line)
    if not match or match[1] not in selected:
        continue
    name = match[1]
    for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)", line):
        row = dict(zip(columns[name], re.findall(r"'(?:\\.|''|[^'\\])*'|[^,]+", match[0][1:-1])))
        key = int(next((row[k] for k in ('QuestID','quest','ID','entry','Entry') if k in row),'0'))
        if (name == 'gameobject_template' and key in (244450,241877)) or (name == 'gameobject_loot_template' and key in (244450,241877)) or (name not in ('gameobject_template','gameobject_loot_template') and key in QUESTS):
            records.setdefault(name, []).append(row)

quests={int(r['ID']):r for r in records['quest_template']}
assert quests[38815]['RewardNextQuest']=='38818'
objective=[r for r in records['quest_objectives'] if r['QuestID']=='38815']
assert len(objective)==1 and all(objective[0][k]==v for k,v in {'ID':'278471','Type':'0','StorageIndex':'1','ObjectID':'97281','Amount':'1','Flags':'0'}.items())
rune=next(r for r in records['gameobject_template'] if r['entry']=='244450')
assert all(rune[k]==v for k,v in {'type':'10','displayId':'22781','Data0':'1691','Data1':'38815','Data10':'192069'}.items())
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={n:[r for r in data.rows(n).values() if r.get('SpellID') in (3365,192069,192062)] for n in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
summons=[r for r in data.rows('SpellEffect').values() if r['Effect']==28 and r['EffectMiscValue1']==97281]
assert len(summons)==1 and summons[0]['SpellID']==192069 and summons[0]['EffectMiscValue2']==719 and summons[0]['ImplicitTarget1']==46
assert any(r['SpellID']==192069 and r['Effect']==90 and r['EffectMiscValue1']==97281 and r['ImplicitTarget1']==1 for r in native['SpellEffect'])
assert any(r['SpellID']==192062 and r['Effect']==6 and r['EffectAura']==61 and r['ImplicitTarget1']==1 for r in native['SpellEffect'])
assert any(r['SpellID']==3365 and r['Effect']==33 and r['EffectMiscValue1']==5 for r in native['SpellEffect'])
assert not any(r['SpellID']==192069 for r in native['SpellXSpellVisual'])
assert any(r['SpellID']==192062 and r['SpellVisualID']==49745 for r in native['SpellXSpellVisual'])
native['SummonProperties']=[data.rows('SummonProperties')[719]]
assert native['SummonProperties'][0]['Flags']==16 and native['SummonProperties'][0]['Control']==0 and native['SummonProperties'][0]['Slot']==0
native['SpellCastTimes']=[data.rows('SpellCastTimes')[i] for i in (1,5)]
native['SpellDuration']=[data.rows('SpellDuration')[i] for i in (1,39)]
assert [r['Base'] for r in native['SpellCastTimes']]==[0,2000]
assert [r['Duration'] for r in native['SpellDuration']]==[10000,2000]

# Follow the regular26972 relationships, including the extra ModelAttach
# relationship absent from the disabled core store's LoadInfo.
from audit_spell_visuals import Table
def raw(name, fields):
 meta=re.split('struct '+name+'Meta\n',data.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
 index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
 table=Table(data.dbc/(name+'.db2'),int(index),int(layout,16));data.hashes[name]=table.sha256
 return [dict(ID=table.ids[i],parent=table.parents.get(i,0),**{k:table.value(i,f) for k,f in fields.items()}) for i in range(table.count)]
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent']==49745]
assert {r['kit'] for r in events}=={58979,58980,58981}
kits={r['kit'] for r in events}
native['SpellVisualKit']=[data.rows('SpellVisualKit')[k] for k in sorted(kits)]
effects=[r for r in raw('SpellVisualKitEffect',{'type':0,'effect':1}) if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18})}
attached=[attachments[r['effect']] for r in effects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
models=[names[r['effect_name']] for r in attached]
assert {r['model_file'] for r in models}=={464111,327753,196561,349187}
assert all(r['attachment']==19 for r in attached)
native.update(visual_events=events,kit_effects=effects,attachments=attached,effect_models=models)
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']],'objectives':records['quest_objectives'],'objects':records['gameobject_template'],'starters':records.get('creature_queststarter',[]),'enders':records.get('creature_questender',[]),'native':native,'runtime_hashes':data.hashes,'limits':'Original TDB25549 objective is MONSTER97281/storage1, unlike the installed21737 GO244450 substitute. Restore the native objective; keep ID/slot/amount and all phase conditions. Native192069 has no own SpellXSpellVisual; its effect90 supplies97281 credit after activation. The scoped association with native192062 is authored from the Bone Effect helper/spell and traced49745 kit/attachment/model chain; this is not proof of complete retail awakening choreography or model playback. No dialogue, player scene, new Ashildir or timing-based credit is invented. The10-second personal wild summon uses Slot0 and native cleanup. Phase6776/endgiver rows and current overlapping spawns are checked separately in live SQL; missing original starter/ender records are not inferred. Native catalog checks lock/display/model records.'}
path=ROOT/'docs/audits/stormheim-reformation-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original38815 MONSTER97281/storage1/38818 handoff; native192069 single personal719 summon/10s and separate credit;192062 cast/aura/visual49745 -> kits/attachment19/four models. No client playback claim.')
