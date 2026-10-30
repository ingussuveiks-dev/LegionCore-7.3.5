"""Follow installed Talonclaw factories, native boarding and accepted transfer receipt."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
callback_slot = runpy.run_path(str(Path(__file__).with_name('pe_creature_ai_slots.py')))['callback_slot']


def references(method):
    return {x.address + x.size + op.mem.disp for x in ins(method) for op in x.operands
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}


def table(address, count=80):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))


def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)


def values(method):
    return {op.imm for x in ins(method) for op in x.operands if op.type == CS_OP_IMM}


def call_index(body, target):
    return next(i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
                and n['resolve'](x.operands[0].imm) == n['symbol'](target))


def exact(body, mnemonic, operand):
    return next(i for i, x in enumerate(body) if x.mnemonic == mnemonic and x.op_str == operand)


registration = references('?AddSC_artifact_acquisition@@')
for name, callbacks in (
        ('npc_talonclaw_boarding_source', ('OnSpellClick',)),
        ('npc_talonclaw_boarded_ride', ('PassengerBoarded', 'UpdateAI'))):
    factory = '?GetAI@?$GenericCreatureScript@U' + name + '@@'
    assert any(n['symbol'](factory) in table(a, 16) for a in registration), factory
    for callback in callbacks:
        slot = callback_slot(n, '?' + callback + '@SmartAI@@')
        assert any(table(a)[slot] == n['symbol']('?' + callback + '@' + name + '@@')
                   for a in references(factory)), (name, callback, slot)

for name, callbacks in (('spell_talonclaw_boarding_summon', ('Check', 'After')),
                        ('spell_talonclaw_boarded_transfer', ('Check', 'Guard', 'After'))):
    factory = '?GetSpellScript@?$GenericSpellScriptLoader@V' + name + '@@'
    register = '?Register@' + name + '@@'
    assert any(n['symbol'](factory) in table(a, 8) for a in registration), factory
    assert any(n['symbol'](register) in table(a, 10) and n['symbol']('?Load@' + name + '@@') in table(a, 10)
               for a in references(factory)), name
    for callback in callbacks:
        assert n['symbol']('?' + callback + '@' + name + '@@') in references(register), (name, callback)

ride = '@npc_talonclaw_boarded_ride@@'
needs('?Join' + ride, '?EnterVehicle@Unit@@', '?HasEmptySeat@Vehicle@@')
needs('?Begin' + ride, '?AddQuestDelayedEvent@Player@@', '?BindQuestObject@Player@@', '?Join' + ride)
needs('?Accepted' + ride, '?Lease' + ride, '?GetSpellTargetPosition@SpellMgr@@',
      '?ProcessEventsFor@SmartScript@@', '?GetQuestObjectiveData@Player@@QEBAHII@Z')
needs('?After@spell_talonclaw_boarding_summon@@', '?Owned@TalonclawBoardedDeparture@@', '?Begin' + ride)
needs('?Guard@spell_talonclaw_boarded_transfer@@', '?ExitReady' + ride, '?PreventHitDefaultEffect@SpellScript@@')
needs('?After@spell_talonclaw_boarded_transfer@@', '?Accepted' + ride)
assert {40385, 1220, 3, 100965, 366115} <= values('?Contact@TalonclawBoardedDeparture@@')
assert {100111, 198950, 1511} <= values('?Accepted' + ride)
assert {3, 5000, 10000, 30000} <= values('?UpdateAI' + ride)
assert 227 in values('?Register@spell_talonclaw_boarded_transfer@@')
board = ins('?PassengerBoarded' + ride)
assert call_index(board, '?Current' + ride) < call_index(board, '?GetVehicleBase@Unit@@') < exact(board, 'mov', 'byte ptr [rdi + 0x80], 1')
assert exact(board, 'mov', 'byte ptr [rdi + 0x80], 1') < exact(board, 'mov', 'dword ptr [rdi + 0x38], 2')
guard = ins('?Guard@spell_talonclaw_boarded_transfer@@')
assert exact(guard, 'cmp', 'dword ptr [rdi + 0x38], 4') < call_index(guard, '?ExitReady' + ride) < exact(guard, 'mov', 'byte ptr [rbx + 0x208], 1')
receipt = ins('?Accepted' + ride)
latch = exact(receipt, 'mov', 'dword ptr [rsi + 0x38], 5')
assert call_index(receipt, '?Lease' + ride) < call_index(receipt, '?GetSpellTargetPosition@SpellMgr@@') < latch
assert exact(receipt, 'cmp', 'dword ptr [rcx + 0x10], 0x5e7') < call_index(receipt, '?GetExactDist@Position@@QEBAMMMM@Z') < latch
distance_test = next(x for x in receipt if x.mnemonic == 'comiss' and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.base == X86_REG_RIP)
distance_at = distance_test.address + distance_test.size + distance_test.operands[1].mem.disp
assert struct.unpack_from('<f', n['binary'], n['offset'](distance_at))[0] == struct.unpack('<f', struct.pack('<f', 0.1))[0]
credit = call_index(receipt, '?ProcessEventsFor@SmartScript@@')
assert latch < call_index(receipt, '?GetQuestObjectiveData@Player@@QEBAHII@Z') < credit
for operand in ('edx, 0x26', 'r9d, 0x1870f', 'r8, rbx', 'dword ptr [rsp + 0x20], 1'):
    assert any(x.op_str == operand for x in receipt[credit - 12:credit]), operand
needs('?UpdateAI' + ride, '?ExitVehicle@Unit@@', '?Restore' + ride, '?ExitReady' + ride, '?OnSpellClick@SmartAI@@')
needs('?Restore' + ride, '?SetClientControl@Player@@', '?SetViewpoint@Player@@')
for callback in ('?Accepted' + ride, '?After@spell_talonclaw_boarded_transfer@@', '?UpdateAI' + ride):
    calls = n['calls'](callback)
    for forbidden in ('?KilledMonsterCredit@Player@@', '?RewardPlayerAndGroupAtEvent@Player@@',
                      '?AddItem@Player@@', '?TeleportTo@Player@@'):
        assert not any(n['resolve'](a) in calls for name, a in n['symbols'].items() if name.startswith(forbidden)), (callback, forbidden)
print('PASS installed Talonclaw source/ride and both spell factories/hooks: native46598 join, quest lease, actual passenger acknowledgment, native exit, guarded native198950 and exact accepted map1511 receipt through original action33. No direct item/quest credit or teleport API. Client travel/camera/pet remains separate.')
