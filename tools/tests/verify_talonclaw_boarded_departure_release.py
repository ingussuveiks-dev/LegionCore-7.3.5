"""Follow installed Talonclaw factories, native boarding and accepted transfer receipt."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')),
                   init_globals={'PE_HELPERS_ONLY': True})
ins = n['instructions']
resolves_to = n['resolves_to']
callback_slot = runpy.run_path(str(Path(__file__).with_name('pe_creature_ai_slots.py')))['callback_slot']


def references(method):
    return {x.address + x.size + op.mem.disp for x in ins(method) for op in x.operands
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}


def table(address, count=80):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))


def contains_callback(address, count, method):
    target = n['symbol'](method)
    return any(resolves_to(value, target) for value in table(address, count))


def registration_references(loader):
    root = '?AddSC_artifact_acquisition@@'
    constructors = [name for name in n['symbols'] if name.startswith('??0' + loader)]
    return references(root) | {
        address for name in constructors if n['symbol'](name) in n['calls'](root)
        for address in references(name)
    }


def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)


def values(method):
    return {op.imm for x in ins(method) for op in x.operands if op.type == CS_OP_IMM}


def call_index(body, target):
    return next(i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
                and n['resolve'](x.operands[0].imm) == n['symbol'](target))


for name, callbacks in (
        ('npc_talonclaw_boarding_source', ('OnSpellClick',)),
        ('npc_talonclaw_boarded_ride', ('IsSummonedBy', 'PassengerBoarded', 'UpdateAI'))):
    factory = '?GetAI@?$GenericCreatureScript@U' + name + '@@'
    registration = registration_references('?$GenericCreatureScript@U' + name + '@@')
    assert any(contains_callback(a, 16, factory) for a in registration), factory
    for callback in callbacks:
        slot = callback_slot(n, '?' + callback + '@SmartAI@@')
        assert any(resolves_to(table(a)[slot], n['symbol']('?' + callback + '@' + name + '@@'))
                   for a in references(factory)), (name, callback, slot)

for name, callbacks in (('spell_talonclaw_boarding_summon', ('Check', 'GuardSummon')),
                        ('spell_talonclaw_boarded_transfer', ('Check', 'Guard', 'After'))):
    factory = '?GetSpellScript@?$GenericSpellScriptLoader@V' + name + '@@'
    register = '?Register@' + name + '@@'
    registration = registration_references('?$GenericSpellScriptLoader@V' + name + '@@')
    assert any(contains_callback(a, 8, factory) for a in registration), factory
    assert any(contains_callback(a, 10, register) and contains_callback(a, 10, '?Load@' + name + '@@')
               for a in references(factory)), name
    for callback in callbacks:
        assert any(resolves_to(a, n['symbol']('?' + callback + '@' + name + '@@'))
                   for a in references(register)), (name, callback)

ride = '@npc_talonclaw_boarded_ride@@'
needs('?UpdateAI' + ride, '?EnterVehicle@Unit@@', '?HasEmptySeat@Vehicle@@')
needs('?Begin' + ride, '?AddQuestDelayedEvent@Player@@', '?BindQuestObject@Player@@')
needs('?IsSummonedBy' + ride, '?Begin' + ride)
needs('?Accepted' + ride, '?Lease' + ride, '?GetSpellTargetPosition@SpellMgr@@',
      '?ProcessEventsFor@SmartScript@@', '?GetQuestObjectiveData@Player@@QEBAHII@Z')
needs('?GuardSummon@spell_talonclaw_boarding_summon@@', '?Owned@TalonclawBoardedDeparture@@', '?PreventHitDefaultEffect@SpellScript@@')
needs('?Guard@spell_talonclaw_boarded_transfer@@', '?ExitReady' + ride, '?PreventHitDefaultEffect@SpellScript@@')
needs('?After@spell_talonclaw_boarded_transfer@@', '?Accepted' + ride)
assert {40385, 1220, 3, 100965, 366115} <= values('?Contact@TalonclawBoardedDeparture@@')
assert {100111, 198950, 1511} <= values('?Accepted' + ride)
assert {100967, 198941, 1220, 200} <= values('?_ExitVehicle@Unit@@')
assert {3, 5000, 10000, 30000} <= values('?UpdateAI' + ride)
assert 227 in values('?Register@spell_talonclaw_boarded_transfer@@')
needs('?PassengerBoarded' + ride, '?Current' + ride, '?GetVehicleBase@Unit@@')
receipt = ins('?Accepted' + ride)
assert call_index(receipt, '?Lease' + ride) < call_index(receipt, '?GetSpellTargetPosition@SpellMgr@@')
distance_test = next(x for x in receipt if x.mnemonic in ('comiss', 'ucomiss') and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.base == X86_REG_RIP)
distance_at = distance_test.address + distance_test.size + distance_test.operands[1].mem.disp
assert struct.unpack_from('<f', n['binary'], n['offset'](distance_at))[0] == struct.unpack('<f', struct.pack('<f', 0.1))[0]
credit = call_index(receipt, '?ProcessEventsFor@SmartScript@@')
assert call_index(receipt, '?GetExactDist@Position@@QEBAMMMM@Z') < call_index(receipt, '?GetQuestObjectiveData@Player@@QEBAHII@Z') < credit
assert any(op.type == CS_OP_IMM and op.imm == 100111 for x in receipt[credit - 12:credit] for op in x.operands)
needs('?UpdateAI' + ride, '?ExitVehicle@Unit@@', '?Restore' + ride, '?ExitReady' + ride, '?OnSpellClick@SmartAI@@')
needs('?Restore' + ride, '?SetClientControl@Player@@', '?SetViewpoint@Player@@')
for callback in ('?Accepted' + ride, '?After@spell_talonclaw_boarded_transfer@@', '?UpdateAI' + ride):
    calls = n['calls'](callback)
    for forbidden in ('?KilledMonsterCredit@Player@@', '?RewardPlayerAndGroupAtEvent@Player@@',
                      '?AddItem@Player@@', '?TeleportTo@Player@@'):
        assert not any(n['resolve'](a) in calls for name, a in n['symbols'].items() if name.startswith(forbidden)), (callback, forbidden)
print('PASS installed Talonclaw source/ride and both spell factories/hooks: native46598 join, quest lease, actual passenger acknowledgment, native exit, guarded native198950 and exact accepted map1511 receipt through original action33. No direct item/quest credit or teleport API. Client travel/camera/pet remains separate.')
