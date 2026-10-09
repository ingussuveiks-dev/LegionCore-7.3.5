"""Read-only native 26972 evidence for Save Yourself and The Head of the Snake."""
import hashlib,json,math,struct,sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/research'))
from audit_spell_mechanics import Data
d=Data(ROOT/'build-extractors/bin/Release/dbc/enUS',ROOT/'.codex/progression-audit')
spells={178283,178284,178285,178288,178347,178348,178784,179215,179217,225948,197936,181404,45204,15497}
fx=[r for r in d.rows('SpellEffect').values() if r['SpellID'] in spells]
def effect(s,i=0):return next(r for r in fx if r['SpellID']==s and r['EffectIndex']==i)
assert spells<=d.rows('Spell').keys()
assert effect(178283)['EffectMiscValue1']==89089 and effect(178283)['EffectBasePoints']==178284
assert effect(178284)['EffectAura']==236 and effect(178284,1)['EffectMiscValue2']==4321
assert effect(178288)['EffectMiscValue1']==89090
assert effect(178347)['EffectMiscValue1']==89116 and effect(178348)['EffectMiscValue1']==89117
assert effect(179215)['Effect']==3 and effect(179217)['Effect']==2
assert effect(179217)['ImplicitTarget1']==87 and effect(179217)['ImplicitTarget2']==16
assert effect(225948)['Effect']==29
assert effect(197936)['EffectAura']==430 and effect(197936)['EffectMiscValue1']==1148
assert effect(181404)['Effect']==219 and effect(181404)['EffectMiscValue1']==346
v=d.rows('Vehicle')[3972];seat=d.rows('VehicleSeat')[v['SeatID1']]
assert seat['ID']==15240 and seat['Flags']&0x800 # generic charm/control and cleanup
# The old-looking base damage is not the final level-100 damage. Both native
# spells have LEVEL_DAMAGE_CALCULATION and no SpellLevels row (base level 0).
misc=[r for r in d.rows('SpellMisc').values() if r['SpellID'] in spells]
for s in (178784,179217):
 assert next(r for r in misc if r['SpellID']==s)['Attributes1']&0x80000
 assert not any(r['SpellID']==s for r in d.rows('SpellLevels').values())
 assert effect(s)['EffectBasePoints']*.25*math.exp(102*60/1000)>190000
members=[r for r in d.rows('SceneScriptPackageMember').values() if r['SceneScriptPackageID']==1520]
assert {13760,13761,13762}<={r['SceneScriptID'] for r in members}
nodes=[r for r in d.rows('PathNode').values() if r['PathID']==15600]
assert len(nodes)==1
cave=d.rows('Location')[nodes[0]['LocationID']]
assert abs(cave['Pos1']+19.482639)<.001 and abs(cave['Pos3']-.6981)<.001
b=(d.dbc/'SceneScriptText.db2').read_bytes();lo,hi=struct.unpack_from('<II',b,28);catalog=struct.unpack_from('<I',b,60)[0]
texts={}
for id in (13760,13761,13762,13763):
 offset,size=struct.unpack_from('<IH',b,catalog+6*(id-lo));assert size
 vals=b[offset:offset+size].split(b'\0');texts[id]=vals[1].decode('utf8')
assert 'FINALE_DELAY = 14' in texts[13760] and 'RISE_TIME = 6' in texts[13760]
assert 'Wait(2)' in texts[13762] and 'scene:EndScene()' in texts[13762]
assert 'azshara = SpawnSceneActor( 88740, 15600, true )' in texts[13762]
objectives=[r for r in json.loads((ROOT/'docs/audits/narthalas-native-2026-10-09.json').read_text())['objectives'] if r['QuestID']==37530]
# Current scope was deliberately excluded from the earlier ten-quest report.
# Parse the immutable native 7.3.5 SQL archive directly.
import re
archive=ROOT/'.codex/TDB735/TDB_full_735.00_2018_02_19/TDB_world_735.00_2018_02_19.sql'
objectives=[]
for line in archive.open(encoding='utf8'):
 if line.startswith('INSERT INTO `quest_objectives`'):
  for match in re.finditer(r"\((?:'(?:\\.|''|[^'\\])*'|[^'()])*\)",line):
   parts=match[0][1:].split(',')
   if int(parts[1]) in (37530,37470):objectives.append(dict(zip(('ID','QuestID','Type','Order','StorageIndex','ObjectID','Amount','Flags','Flags2'),map(int,parts[:9]))))
assert [r['ObjectID'] for r in sorted(objectives,key=lambda r:r['Order']) if r['QuestID']==37530]==[89325,91395,91396,91397,91399,91400,89323]
assert [r['ObjectID'] for r in objectives if r['QuestID']==37470]==[88855]
evidence=dict(objectives=objectives,effects=fx,misc=misc,vehicle=v,seat=seat,scene_members=members,cave=cave,
 native_hashes=d.hashes,scene_text_sha256=hashlib.sha256(b).hexdigest(),archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),
 scene_defeat_seconds=20,scene_end_seconds=22,
 limitations='Read-only native data validation, not an in-game replay. Server implementation uses actual combat deaths for the two naga, a spell-triggered native Azshara scene, and POI-based travel. Dragging actors, naga escape choreography, and introductory dialogue are not reconstructed. Meteor dummy-to-damage link is inferred from native spell names, target layouts and the original vehicle spell list.')
if '--write-evidence' in sys.argv:(ROOT/'docs/audits/azsuna-rescue-native-2026-10-09.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PASS: eight native objectives, original control seat, boarding aura, Farondis abilities and level damage, 22-second native scene/package and true cave position.')
