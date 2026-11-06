"""Select the linked manual pickup/return state and native acknowledgement paths."""
from pathlib import Path
import runpy
import struct
import re
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
a,ins=n['native'],n['instructions']

def sites(prefix, dependency):
    targets={a['resolve'](address) for name,address in a['symbols'].items() if name.startswith(dependency)}
    return [i for i,x in enumerate(ins(prefix)) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and a['resolve'](x.operands[0].imm) in targets]

tick='?Tick@State@WarriorPickupReturn@@'
ack='?Complete@WarriorPickupReturn@@'
owner='?Owner@WarriorPickupReturn@@'
ai='?UpdateAI@ValkyrAI@WarriorPickupReturn@@'
stored='?StoredOffSpec@WarriorPickupReturn@@'
for prefix,dependencies in ((tick,('?Valid@State@WarriorPickupReturn@@','?Earned@WarriorPickupReturn@@','?Equipped@WarriorPickupReturn@@','?HasSceneStatus@Player@@','?CastSpell@Unit@@','?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?SummonCreature@WorldObject@@')),
                            (ack,('?Context@WarriorPickupReturn@@','?HasSceneStatus@Player@@','?Earned@WarriorPickupReturn@@','?Current@WarriorPickupReturn@@')),
                            (owner,('?GetQuestCancellationReceipt@Player@@','?InSamePhase@WorldObject@@','?Earned@WarriorPickupReturn@@')),
                            (ai,(owner,'?CreateAreaTrigger@AreaTrigger@@','?InheritQuestObjectScope@WorldObject@@','?AffectUnit@AreaTrigger@@','?IsUnitAffected@AreaTrigger@@','?IsInArea@AreaTrigger@@','?SetDuration@AreaTrigger@@')),
                            ('?OnUpdate@player_warrior_pickup_return@@',(tick,'?Context@WarriorPickupReturn@@'))):
    for dependency in dependencies:
        assert sites(prefix,dependency), 'Missing actual native dependency:'+prefix+' -> '+dependency
assert sites(tick,stored) and sites(ack,stored)
for dependency in ('?CanUseItem@Player@@QEBA?AW4InventoryResult@@PEBUItemTemplate@@','?GetChildItemByGuid@Player@@','?GetItemByEntry@Player@@'):
    assert sites(stored,dependency), 'Off-spec storage bypasses native owned-item/spec admission:'+dependency
for forbidden in ('?SetUInt32Value@Object@@','?SetActiveSpec@Player@@','?ActivateSpec@Player@@','?CreateItem@Item@@','?StoreNewItem@Player@@'):
    assert not sites(stored,forbidden), 'Off-spec storage changes specialization or creates items'
for prefix in ('?Complete@WarswordsPresentation@@','?Complete@EarthwarderPresentation@@'):
    assert sites(prefix,'?HasSceneStatus@Player@@')[0] < sites(prefix,ack)[0] < sites(prefix,'?CastSpell@Unit@@')[0]
publication='?Update@PickupConversation@WarriorPickupReturn@@'
assert sites(publication,'?getMSTime@@')[0] < sites(publication,'?Update@Conversation@@')[0]
assert sites(publication,'?GetUInt32Value@Object@@') and sites(publication,'?IsInWorld@Object@@')
for values in ((1511,944,40043,128908,134553,15,16,1218,205443,194201,98033,849),
               (1495,909,39191,128289,128288,16,15,1215,205340,208562,94851,0)):
    assert struct.pack('<12I',*values) in a['binary'], 'Native mode IDs/parent-child slots changed'
factory=ins('?GetAI@npc_stromkar_valkyr@@')
tables=[]
for x in factory:
    for value in x.operands:
        if x.mnemonic=='lea' and value.type==CS_OP_MEM and value.mem.base==X86_REG_RIP:
            offset=a['offset'](x.address+x.size+value.mem.disp)
            table=a['binary'][offset:offset+0x400]
            if struct.unpack_from('<Q',table,0x18)[0] == a['symbol'](ai):
                tables.append(table)
assert len(tables)==1 and struct.unpack_from('<Q',tables[0],0x28)[0]==a['symbol']('?Reset@ValkyrAI@WarriorPickupReturn@@'), '93819 actual factory omits the scoped manual exit AI'
assert sites('?GetAI@npc_stromkar_valkyr@@','?Owner@StromkarReturn@@'), 'Shared Stromkar source dispatch lost'
condition='?OnConditionCheck@condition_warrior_pickup_return@@'
assert sites(condition,owner) and sites(condition,'?GetSummoner@TempSummon@@')
values={v.imm for x in ins(condition) for v in x.operands if v.type==CS_OP_IMM}
assert {11869,1511,1495,0x575250} <= values
for prefix in (tick,ai,ack,publication):
    for forbidden in ('?TeleportTo@Player@@','?KilledMonsterCredit@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@','?RewardQuest@Player@@'):
        assert not sites(prefix,forbidden), 'Manual recovery grants progress/items or forces transfer'
    assert 192085 not in {v.imm for x in ins(prefix) for v in x.operands if v.type==CS_OP_IMM}
for prefix in ('?OnLootStateChanged@go_art_warswords@@','?OnLootStateChanged@go_art_earthwarder@@'):
    assert not sites(prefix,'?AddDelayedEvent@FunctionProcessor@@'), 'Legacy automatic departure timer remains'
assert b'player_warrior_pickup_return' in a['binary'] and b'condition_warrior_pickup_return' in a['binary']
maptext=(a['args'].release/'worldserver.map').read_text(errors='replace')
matches=re.findall(r'\s+\w+:\w+\s+\?\?_7player_warrior_pickup_return@@6B@\s+([0-9a-fA-F]{16})',maptext)
assert len(matches)==1
events=a['offset'](int(matches[0],16))
assert a['resolve'](struct.unpack_from('<Q',a['binary'],events+44*8)[0])==a['symbol']('?OnBeforeLogout@player_warrior_pickup_return@@')
entries=struct.unpack_from('<45Q',a['binary'],events)
assert a['symbol']('?OnUpdate@player_warrior_pickup_return@@') in entries
for dependency in ('?ClearArea@ValkyrAI@WarriorPickupReturn@@','?Remove@Conversation@@','?RemoveAurasDueToSpell@Unit@@','?DespawnOrUnsummon@Creature@@'):
    assert sites('?OnBeforeLogout@player_warrior_pickup_return@@',dependency), 'Pre-save cleanup lost native endpoint:'+dependency
print('PASS linked native scene bridges acknowledge manual pickup state before equip; actual modes/native pair slots, original receipt/quest admission, viewer-publication clock, selected93819 scoped AI/custom11869/manual215997 and cleanup are present. No automatic192085 or recovery item/quest/arrival grants. Separate ASAN/TEMP/native inventory/client checks remain necessary.')
