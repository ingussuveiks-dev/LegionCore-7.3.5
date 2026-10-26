"""Check native SetMinion's ordered full-GUID formation and incoming-object path."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
body = instructions('?SetMinion@Unit@@')


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


primary = max(index for index, entry in enumerate(body) if entry.mnemonic == 'movups' and entry.op_str == 'xmm0, xmmword ptr [rsi + 0x4d8]')
branch = next(index for index in range(primary, len(body)) if body[index].mnemonic == 'cmp' and body[index].op_str == 'rcx, 5')
rotation = body[primary:branch]
assert sum(entry.mnemonic == 'movups' and entry.op_str.startswith('xmmword ptr [') for entry in rotation) >= 7
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
           and native['resolve'](entry.operands[0].imm) == native['symbol']('??8ObjectGuid@@') for entry in rotation)
assert any(entry.mnemonic == 'movups' and entry.op_str == 'xmmword ptr [r14], xmm0' for entry in body[:primary])
lookups = [index for index in sites('?GetCreatureOrPetOrVehicle@ObjectAccessor@@') if index > branch]
assert len(lookups) == 2, 'Both formation branches must retain native fallback lookup'
for position in lookups:
    equality, = [index for index in sites('??8ObjectGuid@@') if position - 14 < index < position]
    assert body[equality + 1].op_str == 'al, al' and body[equality + 2].mnemonic == 'je'
    assert body[equality + 3].op_str == 'rdi, r13' and body[equality + 4].mnemonic == 'jmp'
    join = next(index for index, entry in enumerate(body) if entry.address == body[equality + 4].operands[0].imm)
    assert join == position + 2 and body[join].op_str == 'rdi, rdi'
    assert any(entry.op_str == 'r12b, r12b' and entry.mnemonic == 'test' for entry in body[equality - 8:equality])
    assert any(entry.op_str == 'dword ptr [rdi + 0x3b14], xmm6' for entry in body[join:join + 30])
    assert any(entry.op_str == 'dword ptr [rdi + 0x3b18], xmm0' for entry in body[join:join + 35])
assert not sites('?MoveFollow@MotionMaster@@'), 'Formation must not restart an existing movement command'
for caller in ('?InitStats@Minion@@', '?LoadPetFromDB@Pet@@'):
    assert native['symbol']('?SetMinion@Unit@@') in native['calls'](caller), 'Actual summon/load must reach formation'
constants = set()
for entry in body[branch:]:
    if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        constants.add(struct.unpack_from('<f', native['binary'], native['offset'](address))[0])
assert {struct.unpack('<f', struct.pack('<f', value))[0] for value in (1.5707963267948966, 4.71238898038469)} <= constants
print('PASS supplied Release actual Minion init/Pet load -> SetMinion: copied128-bit controlled GUID order rotates primary first; both <5/>=5 branches select live incoming minion before registration and preserve native lookup fallback/angle-distance stores. Existing follow commands are not restarted. No actual client path/collision/Hati combat or visual certification.')
