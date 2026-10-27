"""Verify selected native pickup/scene/return factories in the linked x64 PE.

The extracted handler host covers failure/reentry branches; this check proves
the actual registered objects dispatch those handlers and native consumers.
"""
from pathlib import Path
import runpy
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM

p = runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_takeover_release.py')))
n, ins, values, references, table, needs, hook = (p[k] for k in
    ('n', 'ins', 'values', 'references', 'table', 'needs', 'hook'))
registration = references('?AddSC_artifact_acquisition@@')
for actor, methods in (
    ('pickup', ('Reset', 'GossipUse', 'GossipHello', 'OnStateChanged', 'UpdateAI')),
    ('return', ('Reset', 'GossipUse', 'GossipHello', 'UpdateAI'))):
    factory = '?GetAI@?$GenericGameObjectScript@Ugo_lights_wrath_' + actor + '@@'
    assert any(n['symbol'](factory) in table(a, 16) for a in registration)
    assert any(all(n['symbol']('?' + method + '@go_lights_wrath_' + actor + '@@') in table(a, 30)
                   for method in methods) for a in references(factory)), actor
for script, kind, callbacks in (
    ('pickup_cast', 'Spell', ('Check', 'Hit', 'After')),
    ('pickup_scene', 'Aura', ('Apply', 'Tick', 'Removed')),
    ('return', 'Spell', ('Check', 'Hit', 'Depart'))):
    suffix = 'spell_lights_wrath_' + script + '@@'
    factory = '?Get' + kind + 'Script@?$Generic' + kind + 'ScriptLoader@V' + suffix
    register = '?Register@' + suffix
    assert any(n['symbol'](factory) in table(a, 8) for a in registration)
    assert any(table(a, 10)[5] == n['symbol'](register) for a in references(factory))
    for callback in callbacks:
        hook(register, '?' + callback + '@' + suffix)

scope = 'Claim@LightsWrathPickup@@'
# Follow the two actual data objects selected by SourceRoute. Merely finding
# immediate IDs in an unused helper would not prove the shared dispatch.
map_text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
for name, data in (
    ('priest', (1065,41631,5,128868,248572,104985,208011,1244,47214,47119,0,0,0.633369)),
    ('mage', (1101,42011,8,127857,249814,106756,211954,1269,47999,47997,55130,55131,2.21201))):
    found = re.findall(r'\?'+name+r'@LightsWrathPickup@@3URoute@1@B\s+([0-9a-fA-F]{16})\s', map_text)
    assert len(found) == 1
    address = int(found[0], 16)
    assert n['binary'][n['offset'](address):n['offset'](address)+52] == struct.pack('<12If', *data)
    assert address in references('?SourceRoute@LightsWrathPickup@@')
begin = '?Begin@go_lights_wrath_pickup@@'
needs(begin, '?CanTake@LightsWrathPickup@@', '?AddQuestDelayedEvent@Player@@',
      '?getQuestStatus@Player@@', '?GetItemByEntry@Player@@', '?UpdateAchievementCriteria@Player@@')
needs(begin, '?SourceRoute@LightsWrathPickup@@')
assert 36 in values(begin)
needs('?OnStateChanged@go_lights_wrath_pickup@@', begin, '?SetLootState@GameObject@@', '?GetGameObject@Map@@')
needs('?UpdateAI@go_lights_wrath_pickup@@', '?Update@' + scope, '?GetAura@Unit@@')
needs('?Current@' + scope, '?CanTake@LightsWrathPickup@@', '?getQuestStatus@Player@@')
needs('?CanTake@LightsWrathPickup@@', '?Eligible@LightsWrathPickup@@', '?Role@LightsWrathPickup@@',
      '?canSeeOrDetect@WorldObject@@', '?IsWithinLOSInMap@WorldObject@@')
needs('?Role@LightsWrathPickup@@', '?isSpawned@GameObject@@')
assert 1583 in values('?Role@LightsWrathPickup@@')
needs('?Eligible@LightsWrathPickup@@', '?SourceRoute@LightsWrathPickup@@', '?Leaf@LightsWrathPickup@@')
assert {6, 7} <= values('?Eligible@LightsWrathPickup@@')
update = '?Update@' + scope
needs(update, '?SpawnPortal@' + scope, '?Current@' + scope,
      '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert {5000, 30000} <= values(update)
spawn = '?SpawnPortal@' + scope
needs(spawn, '?GetGameObjectListWithEntryInGrid@WorldObject@@',
      '?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM', '?SetLootState@GameObject@@')  # inlined Finish
assert {248574, 2000} <= values(spawn)
needs('?Check@spell_lights_wrath_pickup_cast@@', '?Current@' + scope, '?Find@LightsWrathPickup@@')
needs('?After@spell_lights_wrath_pickup_cast@@', '?GetItemByEntry@Player@@',
      '?GetItemByPos@Player@@QEBAPEAVItem@@EE', '?GetQuestObjectiveData@Player@@QEBAHII')  # inlined Equipped

removed = '?Removed@spell_lights_wrath_pickup_scene@@'
needs(removed, '?GetUnitOwner@AuraScript@@', '?GetTargetApplication@AuraScript@@',
      '?GetDuration@AuraScript@@', '?GetMaxDuration@AuraScript@@', '?GetAura@AuraScript@@',
      '?HasSceneStatus@Player@@', '?Current@' + scope, '?GetQuestObjectiveData@Player@@QEBAHII')
assert {9000, 3, 4, 105109} <= values(removed)
b = ins(removed)
# Real elapsed-time field, signed expiry guard and remaining-time clamp; a
# client Complete/Canceled ACK alone cannot create the exit before native9s.
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and
           x.operands[0].mem.disp == 0x118 and b[i+1].mnemonic == 'jl' for i, x in enumerate(b[:-1]))
assert any(x.mnemonic == 'cmovg' for x in b) and any(x.mnemonic == 'cmova' for x in b)
tick = '?Tick@spell_lights_wrath_pickup_scene@@'
needs(tick, '?GetUnitOwner@AuraScript@@', '?Current@' + scope)
assert n['symbol']('?GetTargetApplication@AuraScript@@') not in n['calls'](tick)
for packet in ('Canceled', 'Complete'):
    needs('?HandleScenePlayback' + packet + '@WorldSession@@', '?SceneCompleted@Player@@')
needs('?SceneCompleted@Player@@', '?HasSceneStatus@Player@@', '?RemoveAurasDueToSpell@Unit@@')

# Unrelated contexts retain native SmartGameObjectAI lifecycle/gossip. Both
# actual Nexus routes now use the private accepted-departure receipt.
for method in ('Reset', 'GossipUse', 'GossipHello', 'UpdateAI'):
    needs('?' + method + '@go_lights_wrath_return@@', '?' + method + '@SmartGameObjectAI@@')
needs('?SelectGameObjectAI@FactorySelector@@', '?GetGameObjectAI@ScriptMgr@@')
needs('?GossipUse@go_lights_wrath_return@@', '?ScopedPortal@LightsWrathPickup@@', '?PhysicalExit@LightsWrathPickup@@')
assert {1065, 1101, 1583, 248572, 249814} <= values('?ScopedPortal@LightsWrathPickup@@')
needs('?Check@spell_lights_wrath_return@@', '?Ready@spell_lights_wrath_return@@')
depart = '?Depart@spell_lights_wrath_return@@'
needs(depart, '?Ready@spell_lights_wrath_return@@', '?AwardState@LightsWrathPickup@@',
      '?EffectTeleportUnits@Spell@@', '?SameAttempt@Exit@LightsWrathPickup@@', '?UpdateAchievementCriteria@Player@@')
assert {92, 49875, 1220} <= values(depart)
body = ins(depart)
def sites(method):
    return [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
            and n['resolve'](x.operands[0].imm) == n['symbol'](method)]
assert max(sites('?EffectTeleportUnits@Spell@@')) < min(sites('?UpdateAchievementCriteria@Player@@'))
needs('?AwardState@LightsWrathPickup@@', '?Eligible@LightsWrathPickup@@')
needs('?AwardState@LightsWrathPickup@@', '?SourceRoute@LightsWrathPickup@@')
assert {6, 105109} <= values('?AwardState@LightsWrathPickup@@')
for method in (update, spawn, removed, depart, begin):
    for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?KilledMonsterCredit@Player@@', '?SetHealth@Unit@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method), (method, forbidden)
print('PASS actual five registered factories/vtables/hooks and selected two exact native route data objects, physical storage/equip/original quest receipt,9s/native Complete+Canceled, retryable map portal, unrelated SmartAI delegation and both accepted native departures before eligible49875. No direct item/quest/health grants; dual native host/catalog/data coverage separate; client traversal/VFX pending.')
