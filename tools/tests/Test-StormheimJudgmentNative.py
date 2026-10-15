"""Read original TDB735 quest/chest records and native 7.3.5 Open mechanics."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table

QUESTS = {38811,38817}
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
        if (name == 'gameobject_template' and key in (241864,241868,241869,241877)) or (name == 'gameobject_loot_template' and key in (241864,241868,241869,241877)) or (name not in ('gameobject_template','gameobject_loot_template') and key in QUESTS):
            records.setdefault(name, []).append(row)

quests={int(row['ID']):row for row in records['quest_template']}
assert quests[38811]['RewardNextQuest']=='38817'
objectives=[r for r in records['quest_objectives'] if r['QuestID']=='38811']
assert {(r['ObjectID'],r['StorageIndex'],r['Amount'],r['Flags']) for r in objectives}=={('92889','0','1','0'),('92918','1','1','0'),('92920','2','1','0')}
assert all(r['Type']=='0' for r in objectives)
bowls=[r for r in records['gameobject_template'] if r['entry'] in ('241864','241868','241869')]
assert len(bowls)==3 and all(r['type']=='10' and r['Data0']=='1691' and r['Data1']=='38811' and r['Data10']=='0' for r in bowls)
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
native={name:[r for r in data.rows(name).values() if r.get('SpellID') in (3365,184831)] for name in ('SpellEffect','SpellMisc','SpellXSpellVisual')}
assert any(r['SpellID']==3365 and r['Effect']==33 and r['EffectMiscValue1']==5 and r['ImplicitTarget1']==23 for r in native['SpellEffect'])
assert any(r['SpellID']==184831 and r['Effect']==6 and r['EffectAura']==4 and r['ImplicitTarget1']==1 for r in native['SpellEffect'])
assert any(r['SpellID']==184831 and r['SpellVisualID']==47364 for r in native['SpellXSpellVisual'])
native['SpellCastTimes']=[data.rows('SpellCastTimes')[i] for i in sorted({r['CastingTimeIndex'] for r in native['SpellMisc']})]
native['SpellDuration']=[data.rows('SpellDuration')[r['DurationIndex']] for r in native['SpellMisc'] if r['DurationIndex']]
# Use the regular table's actual relationship data for visual -> kit -> effect.
def raw(name, fields):
 meta=re.split('struct '+name+'Meta\n',data.meta,flags=re.IGNORECASE)[1].split('\n};')[0]
 index,layout=re.search(r'DB2Meta instance\((-?\d+), \d+, (0x[\dA-Fa-f]+)',meta).groups()
 table=Table(data.dbc/(name+'.db2'),int(index),int(layout,16));data.hashes[name]=table.sha256
 return [dict(ID=table.ids[i],parent=table.parents.get(i,0),**{k:table.value(i,f) for k,f in fields.items()}) for i in range(table.count)]
events=[r for r in raw('SpellVisualEvent',{'start':0,'end':3,'target':6,'kit':7}) if r['parent']==47364]
assert events
kits={r['kit'] for r in events if r['kit']}
kitRows=data.rows('SpellVisualKit');effects=raw('SpellVisualKitEffect',{'type':0,'effect':1})
# Include fallback kits as well as directly referenced kits.
# ModelAttach has an extra relationship field absent from the runtime LoadInfo
# (its core store is disabled). Use the same exact regular26972 field mapping
# as audit_character_spell_chains.py, not guessed packed PlayerCondition offsets.
for _ in range(10):
 added={kitRows[k]['FallbackSpellVisualKitID'] for k in kits if kitRows[k]['FallbackSpellVisualKitID']}
 if added<=kits:break
 kits|=added
kitEffects=[r for r in effects if r['parent'] in kits]
attachments={r['ID']:r for r in raw('SpellVisualKitModelAttach',{'effect_name':3,'attachment':4,'start_anim':15,'anim':16,'end_anim':17,'anim_kit':18,'low_def':19})}
attached=[attachments[r['effect']] for r in kitEffects if r['type']==2]
names={r['ID']:r for r in raw('SpellVisualEffectName',{'model_file':10,'texture_file':8})}
effectNames=[names[r['effect_name']] for r in attached]
native['incense_visual_events']=events
native['incense_kits']=[kitRows[k] for k in sorted(kits)]
native['incense_kit_effects']=kitEffects
native['incense_attachments']=attached
native['incense_effect_names']=effectNames
out={'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']],'objectives':objectives,'bowls':bowls,'original_enders':records['creature_questender'],'native':native,'runtime_hashes':data.hashes,'limits':'Original required three rulers and bowl lock/quest settings. Current installed SAI delays/summon locations, source bindings and Hotfix dialogue are checked separately. Native regular relationship tables trace184831 -> visual47364 -> kits/effects/attachments/models; C++ DB2Storage checks lock1691/type5 and ruler/helper/bowl displays. All five selected BroadcastText IDs require existing Hotfix overlays. Dialogue pauses/private ritual behavior are authored. No M2 playback or full retail combat repertoire/client certification.'}
path=ROOT/'docs/audits/stormheim-judgment-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original38811 three rulers/bowls/38817 handoff; native Open3365/type5 and incense184831 visual47364/kit/attachment/model chain.')
