"""Read the original Faljar confrontation objective and native effect/route records."""
import hashlib
import json
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/research'))
from audit_spell_mechanics import Data

QUESTS = {38818,39837}
archive = ROOT / '.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
selected = {'quest_template','quest_objectives','creature_questender','creature_queststarter'}
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
assert quests[38818]['RewardNextQuest']=='0'
objective=[r for r in records['quest_objectives'] if r['QuestID']=='38818']
assert len(objective)==1 and all(objective[0][k]==v for k,v in {'ID':'278476','Type':'0','StorageIndex':'0','ObjectID':'93442','Amount':'1','Flags':'0'}.items())
data=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/stormheim-no-overlays')
ids={89319,100044,209164,209363,208544,208547,209177,209310,209241}
keys=('ID','SpellID','EffectIndex','Effect','EffectAura','EffectBasePoints','EffectMiscValue1','EffectMiscValue2','EffectTriggerSpell','ImplicitTarget1','ImplicitTarget2')
effects=[{k:r[k] for k in keys} for r in data.rows('SpellEffect').values() if r['SpellID'] in ids]
assert any(r['SpellID']==89319 and r['EffectAura']==18 and r['EffectMiscValue1']==30 for r in effects)
assert any(r['SpellID']==100044 and r['EffectAura']==19 and r['EffectMiscValue1']==30 for r in effects)
assert any(r['SpellID']==209363 and r['EffectAura']==395 and r['EffectMiscValue1']==4870 for r in effects)
assert any(r['SpellID']==208544 and r['EffectAura']==40 for r in effects)
assert any(r['SpellID']==208544 and r['EffectAura']==23 and r['EffectTriggerSpell']==208547 for r in effects)
assert any(r['SpellID']==209310 and r['Effect']==219 and r['EffectMiscValue1']==1867 for r in effects)
assert {56,201,40,60,67}<={r['EffectAura'] for r in effects if r['SpellID']==209241}
helheim=data.rows('Map')[1463];assert helheim['InstanceType']==0
out={
 'archive':archive.name,'archive_sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),
 'quests':[{k:r[k] for k in ('ID','LogTitle','LogDescription','QuestDescription','Flags','FlagsEx','RewardNextQuest')} for r in records['quest_template']],
 'objectives':records['quest_objectives'],'starters':records.get('creature_queststarter',[]),'enders':records.get('creature_questender',[]),
 'native_effects':effects,'map1463':helheim,'runtime_hashes':data.hashes,
 'limits':'Original38818 is a93442 confrontation objective with no automatic RewardNextQuest;39837 is offered by the live97319 menu after reward. Native Quest Invisibility22 matches detection100044;209164 and209177 copied Spell names are resolved by the separate C++ DB2Storage probe. Runic Bulwark208544 has immunity/periodic damage and is not a safe player banishment.209310 is conversation1867, not the banishment spell. Native209241 has transform/flying/immunity/pacify/disarm effects: the new ending does not apply it to the player. The implementation provides an authored private melee confrontation, native text groups and delayed standard teleport using existing coordinates, with no new cinematic/vehicle/player aura. Native runestone/Bulwark combat and the full retail banishment cinematic are not reconstructed or client-verified by this patch. These are explicit presentation/combat fidelity limits, separate from the fixed progression/ownership/one-shot/retry path.'
}
path=ROOT/'docs/audits/stormheim-final-judgment-native-2026-10-15.json'
if '--write-evidence' in sys.argv:path.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(path.read_text())
print('PASS: original93442/storage0 confrontation and native next quest; invisibility/detection pair, actual Bulwark/conversation/banishment effect types and outdoor Helheim. No retail cinematic/combat certification.')
