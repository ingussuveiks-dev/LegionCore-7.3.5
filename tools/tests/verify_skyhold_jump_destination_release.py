"""Inspect the actual Skyhold destination write and native movement consumers."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_landing_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?EffectJumpDest@Spell@@')
gate = [index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and
        entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 192085]
assert len(gate) == 1, 'Reinspect native Skyhold branch'
position = gate[0]
assert body[position - 3].op_str == 'rcx, qword ptr [rdi + 0x180]', 'Destination base changed'
assert body[position - 2].op_str == 'rax, qword ptr [rdi]', 'Spell identity base changed'
assert body[position + 2].op_str == 'xmm6, dword ptr [rcx + 8]', 'Original destination Z not loaded'
assert body[position + 3].mnemonic == 'jne', 'Other spells enter the lift branch'
lift = body[position + 4]
assert lift.mnemonic == 'addss' and lift.op_str.startswith('xmm6, ')
constant = lift.operands[1]
assert constant.type == CS_OP_MEM and constant.mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](lift.address + lift.size + constant.mem.disp))[0] == 20.0
assert body[position + 5].op_str == 'dword ptr [rcx + 4], xmm7'
assert body[position + 6].mnemonic == 'movss' and body[position + 6].op_str == 'dword ptr [rcx + 8], xmm6', 'Lift never reaches the actual destination'
assert body[position + 3].operands[0].imm == body[position + 7].address, 'Other spells do not skip the destination write'
movement = '?MoveJump@MotionMaster@@QEAAXMMMMMM'
target = native['symbol'](movement)
calls = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and
         entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]
assert len(calls) == 1 and calls[0] > position + 6
arguments = body[calls[0] - 25:calls[0]]
for expected in ('r8, qword ptr [rdi + 0x180]', 'xmm3, dword ptr [r8 + 8]',
                 'xmm1, dword ptr [r8]', 'xmm2, dword ptr [r8 + 4]'):
    assert any(entry.mnemonic in ('mov', 'movss') and entry.op_str == expected for entry in arguments), f'Movement does not consume destination: {expected}'
assert not any(entry.mnemonic.startswith('mov') and entry.operands[0].type == CS_OP_MEM and
               entry.op_str.startswith('dword ptr [rcx + 8]') for entry in body[position + 7:calls[0]]), 'Destination Z overwritten before movement'
motion = instructions(movement)
for expected in ('xmm9, xmm1', 'xmm10, xmm2', 'xmm11, xmm3'):
    assert any(entry.mnemonic == 'movaps' and entry.op_str == expected for entry in motion), 'Native movement XYZ inputs changed'
move_to = native['symbol']('?MoveTo@MoveSplineInit@Movement@@')
launch = [index for index, entry in enumerate(motion) if entry.mnemonic == 'call' and
          entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == move_to]
assert len(launch) == 1
for expected in ('dword ptr [rsp + 0x48], xmm9', 'dword ptr [rsp + 0x4c], xmm10',
                 'dword ptr [rsp + 0x50], xmm11', 'rdx, [rsp + 0x48]'):
    assert any(entry.op_str == expected for entry in motion[launch[0] - 8:launch[0]]), 'Native MoveTo vector no longer contains destination XYZ'
print('PASS: canonical192085-only branch writes originalZ+20 into the actual destination before native MoveJump reloads it; its Z reaches native spline MoveTo vector. Other spells skip the write. Not client ascent, teleport or return-actor proof')
