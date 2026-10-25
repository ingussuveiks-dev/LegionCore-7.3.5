"""Verify installed prepublication join abort/finish and native queue return semantics."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_vehicle_pending_joins_release.py')))
native, instructions = previous['native'], previous['instructions']
body = instructions('?Execute@VehicleJoinEvent@@')

def locate(entries, operand, mnemonic=None):
    return next(index for index, entry in enumerate(entries) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))

def call(entries, prefix):
    target = native['symbol'](prefix)
    return next(index for index, entry in enumerate(entries) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == target)

save_time = locate(body, 'rbx, rdx', 'mov')
save_this = locate(body, 'rdi, rcx', 'mov')
passenger_world = call(body, '?IsInWorld@Object@@')
lookup = call(body, '?GetUnit@ObjectAccessor@@')
target_world = next(index for index in range(lookup + 1, len(body)) if body[index].mnemonic == 'call'
                    and body[index].operands[0].type == CS_OP_IMM
                    and native['resolve'](body[index].operands[0].imm) == native['symbol']('?IsInWorld@Object@@'))
kit = locate(body, 'rax, qword ptr [rsi + 0x30e0]', 'mov')
rec_aura = locate(body, 'dword ptr [rax + 0x58], 0', 'cmp')
aura = call(body, '?HasAuraTypeWithCaster@Unit@@')
success = locate(body, 'r14, qword ptr [rdi + 0x50]', 'mov')
publication = locate(body, 'qword ptr [rcx + 0x30d8], rax', 'mov')
assert save_time < save_this < passenger_world < lookup < target_world < kit < rec_aura < aura < success < publication
assert body[passenger_world - 1].op_str == 'rcx, qword ptr [rcx + 0x50]'
assert body[lookup + 1].op_str == 'qword ptr [rdi + 0x48], rax'
assert body[aura - 2].op_str == 'edx, 0xec' and body[aura - 1].op_str == 'rcx, rsi'
assert body[rec_aura + 1].mnemonic == 'jne' and body[rec_aura + 1].operands[0].imm == body[success].address
failure = body[passenger_world + 2].operands[0].imm
edges = [entry for entry in body[:success] if entry.mnemonic == 'je' and entry.operands[0].imm == failure]
assert len(edges) == 5, 'All five passenger/target/kit/control-aura rejection edges must finish through Abort'
for gate in (passenger_world + 2, lookup + 3, target_world + 2, kit + 3, aura + 2):
    assert body[gate] in edges
cleanup = next(index for index, entry in enumerate(body) if entry.address == failure)
assert [entry.op_str for entry in body[cleanup:cleanup + 5]] == [
    'rax, qword ptr [rdi]', 'rdx, rbx', 'rcx, rdi', 'qword ptr [rax + 0x18]', 'al, 1']
assert body[cleanup + 3].mnemonic == 'call' and body[cleanup + 4].mnemonic == 'mov'
assert previous['slots'][1] == native['symbol']('?Execute@VehicleJoinEvent@@')
assert previous['slots'][3] == native['symbol']('?Abort@VehicleJoinEvent@@')

abort = instructions('?Abort@VehicleJoinEvent@@')
remove = call(abort, '?RemoveAurasByType@Unit@@QEAAXW4AuraType@@VObjectGuid@@')
assert abort[remove - 2].op_str == 'edx, 0xec' and abort[remove - 1].op_str == 'rcx, rbx'
assert locate(abort, 'rcx, qword ptr [rsi]', 'mov') < remove
accessory = locate(abort, 'dword ptr [rcx + 0x30e8], 0x200', 'test')
despawn = call(abort, '?DespawnOrUnsummon@Creature@@')
assert remove < accessory < despawn and abort[despawn - 1].op_str == 'edx, edx'
saved_guid = locate(abort, 'rcx, [r14 + 0x60]', 'lea')
assert abort[saved_guid + 1].mnemonic == 'call'
assert native['resolve'](abort[saved_guid + 1].operands[0].imm) == native['symbol']('?GetGUIDLow@ObjectGuid@@')
assert not any('ptr [r14 + 0x58]' in entry.op_str for entry in abort), 'Unavailable-target logging still reads saved Seat iterator'
assert b'Passenger GuidLow: %u, Entry: %u, board on unavailable vehicle GuidLow: %u cancelled' in native['binary']

queue = instructions('?Update@EventProcessor@@')
event = locate(queue, 'rbx, qword ptr [rax + 0x28]', 'mov')
abort_flag = locate(queue, 'byte ptr [rbx + 8], 0', 'cmp')
dispatch = locate(queue, 'qword ptr [rax + 8]', 'call')
abort_dispatch = locate(queue, 'qword ptr [rax + 0x18]', 'call')
destroy = locate(queue, 'qword ptr [rax]', 'call')
assert event < abort_flag < dispatch < abort_dispatch < destroy
assert sum(entry.mnemonic == 'call' for entry in queue[event:abort_flag]) == 2, 'Due node must be erased/freed before Execute'
assert queue[dispatch + 1].op_str == 'al, al' and queue[dispatch + 2].mnemonic == 'je'
assert queue[dispatch + 2].operands[0].imm == queue[destroy + 1].address, 'False must still skip destruction for explicitly requeued events'
assert queue[destroy - 2].op_str == 'edx, 1' and queue[destroy - 1].op_str == 'rcx, rbx'
print('PASS: selected native Execute sends all five prepublication rejection edges through existing virtual Abort(eventTime), returns true, and native EventProcessor destroys once; caster-scoped aura/accessory cleanup, saved-GUID diagnostic without Seat dereference and generic false/requeue semantics retained. Late postpublication failures and actual world/client boarding remain OPEN.')
