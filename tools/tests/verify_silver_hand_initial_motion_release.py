"""Check canonical default-motion selection and distinct native waypoint stores."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body)
            if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


selector = '?SelectMovementGenerator@FactorySelector@@'
body = instructions(selector)
idle = next(index for index, entry in enumerate(body) if entry.mnemonic == 'xor' and entry.op_str == 'bl, bl')
kind = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rcx + 0x16], 3')
controlled = next(index for index, entry in enumerate(body) if entry.op_str == 'qword ptr [rax + 0x2f40], rsi')
movement = next(index for index, entry in enumerate(body) if entry.op_str == 'ebx, byte ptr [rax + 0x3a80]')
assert idle < kind < controlled < movement
assert body[kind + 1].mnemonic == 'cmovne'
assert body[controlled + 1].mnemonic == 'jne'
assert body[controlled + 1].operands[0].imm == body[movement + 1].address
assert any(entry.op_str == 'rax, rax' and body[index + 1].mnemonic == 'je'
           and body[index + 1].operands[0].imm == body[movement + 1].address
           for index, entry in enumerate(body[:controlled]))
assert any(entry.mnemonic == 'jmp' and entry.op_str == 'qword ptr [rax + 8]' for entry in body[movement:])
body = instructions('?InitDefault@MotionMaster@@')
select, = sites(body, selector)
mutate, = sites(body, '?Mutate@MotionMaster@@')
assert select < mutate and any(entry.op_str == 'r8d, r8d' and entry.mnemonic == 'xor' for entry in body[select:mutate])

for prefix in ('?LoadPath@?$WaypointMovementGenerator@VCreature', '?DoInitialize@?$WaypointMovementGenerator@VCreature'):
    body = instructions(prefix)
    enabled = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rcx + 0x31], 0')
    path = next(index for index, entry in enumerate(body) if entry.op_str == 'dword ptr [rcx + 0x2c], 0')
    inherited = next(index for index, entry in enumerate(body) if entry.op_str == 'eax, dword ptr [rdx + 0x3b20]')
    scripted = next(index for index, entry in enumerate(body) if entry.op_str == 'rcx, [rax + 0x40]')
    assert enabled < path < inherited < scripted
    assert body[enabled + 1].mnemonic == 'je' and body[path + 1].mnemonic == 'jne'
    alternate = next(index for index, entry in enumerate(body) if entry.address == body[path + 1].operands[0].imm)
    assert inherited < alternate < scripted
    ordinary = next(index for index, entry in enumerate(body[inherited:alternate], inherited) if entry.op_str == 'rcx, rax')
    assert body[ordinary + 1].mnemonic == 'jmp' and body[ordinary + 1].operands[0].imm == body[scripted + 1].address
    missing = next(index for index, entry in enumerate(body) if entry.op_str == 'qword ptr [rbx + 0x10], 0')
    assert body[missing + 1].mnemonic == 'jne'
    delay = next(entry for entry in body if entry.address == body[missing + 1].operands[0].imm)
    assert delay.op_str == 'dword ptr [rbx + 0x20], 0xbb8'
    messages = []
    for entry in body[missing:]:
        if entry.mnemonic != 'lea' or len(entry.operands) != 2 or entry.operands[1].type != CS_OP_MEM or entry.operands[1].mem.base != X86_REG_RIP:
            continue
        offset = native['offset'](entry.address + entry.size + entry.operands[1].mem.disp)
        messages.append(native['binary'][offset:offset + 160].split(b'\0')[0])
    assert any(message.startswith(b'WaypointMovementGenerator::LoadPath: creature ') and b"doesn't have waypoint path id" in message for message in messages)
    assert any(entry.op_str == 'r8d, 4' for entry in body[missing:])
print('PASS canonical InitDefault selects actual spawn motion unless player-controlled; creature waypoint initialization distinguishes ordinary/script stores, retains missing-path warning and native 3000ms delay. Data-only repair, not client movement proof.')
