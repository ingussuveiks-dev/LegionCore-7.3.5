"""Verify that retained occupied-seat admission schedules native abort, not boarding."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_vehicle_foreign_control_release.py')))
native, locate, call = previous['native'], previous['locate'], previous['call']
context = previous
while 'instructions' not in context:
    context = context['previous']
body = context['instructions']('?AddPassenger@Vehicle@@')
queued = call(body, '?AddEvent@EventProcessor@@')
lookup = call(body, '?GetUnit@ObjectAccessor@@')
exit_vehicle = call(body, '?ExitVehicle@Unit@@')
abort = locate(body, 'byte ptr [r12 + 8], 1', 'mov')
assert queued < lookup < exit_vehicle < abort
assert body[queued - 2].op_str == 'rdx, r12' and body[queued - 1].op_str == 'rcx, [r15 + 0x748]'
assert [entry.op_str for entry in body[abort:abort + 2]] == ['byte ptr [r12 + 8], 1', 'al, al']
assert body[abort + 1].mnemonic == 'xor'
assert body[exit_vehicle - 2].op_str == 'edx, edx' and body[exit_vehicle - 1].op_str == 'rcx, rax'
post = exit_vehicle + 1
assert body[post].op_str == 'rcx, [rbx + 0x30]'
assert body[post + 1].mnemonic == 'call'
assert native['resolve'](body[post + 1].operands[0].imm) == native['symbol']('??BObjectGuid@@QEBA_NXZ')
assert body[post + 2].op_str == 'al, al'
assert body[post + 3].mnemonic == 'jne' and body[post + 3].operands[0].imm == body[abort].address
assert body[lookup + 1].op_str == 'rax, rax'
assert body[lookup + 2].mnemonic == 'je' and body[lookup + 2].operands[0].imm == body[post].address
assert body[lookup - 3].op_str == 'xmmword ptr [rsp + 0xc0], xmm0'
assert body[lookup - 4].op_str == 'xmm0, xmmword ptr [rbx + 0x30]'
assert body[post + 4].op_str == 'rax, qword ptr [rbx + 0x28]'
state = locate(body, 'dword ptr [r15 + 0x3140], 0x80000', 'or')
assert post + 4 < state < abort
registration = max(index for index in range(queued + 1, lookup) if body[index].mnemonic == 'call'
                    and body[index].operands[0].type == CS_OP_IMM
                    and native['resolve'](body[index].operands[0].imm) == native['symbol']('?AddPendingEvent@Vehicle@@'))
assert body[registration - 3].op_str == 'qword ptr [r12 + 0x58], rbx'
assert body[registration - 2].op_str == 'rdx, r12' and body[registration - 1].op_str == 'rcx, rsi'
assert body[abort - 2].op_str == 'al, 1' and body[abort - 2].mnemonic == 'mov'
assert body[abort - 1].mnemonic == 'jmp' and body[abort - 1].operands[0].imm == body[abort + 2].address
print('PASS: installed AddPassenger queues the actual r12 event on the requesting Unit, retains selected Seat/registry, and rechecks its full occupant GUID after optional native ExitVehicle. Missing actor and refused exit reach the same to_Abort8/false block before ONVEHICLE/ExtraLook; empty/released seats retain success. Native EventProcessor abort/destruction checked independently; earlier waitOnSeat, actual exit/world/client remain separate.')
