"""Selected Blood entry and two native, acknowledged barrier checkpoints."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_graveyard_release.py')))
n, body, sites, at = (p[k] for k in ('n', 'body', 'sites', 'at'))
holy = p['p']
blood = holy['blood']
assert blood is not None, 'Release lacks Blood barrier cemetery recovery'
step = next(i for i in sites('?GetCurrentStep@Scenario@@') if blood < i <= blood + 3)
assert body[step + 1].op_str == 'eax, 3' and body[step + 1].mnemonic == 'cmp' and body[step + 2].mnemonic == 'jae'
later = body[step + 2].operands[0].imm
entry, = sites('?GetSpellTargetPosition@SpellMgr@@')
assert step + 2 < entry and body[entry - 2].op_str == 'edx, 0x313ce'
entry_failure = body[entry + 3].operands[0].imm
assert body[entry + 3].mnemonic == 'je'
mapcheck = at('dword ptr [rax], edi', 'cmp')
assert body[mapcheck - 1].op_str == 'edi, 0x5d1' and body[mapcheck + 1].operands[0].imm == entry_failure
early_coord = holy['coordinates'][2]
assert entry < mapcheck < early_coord and body[early_coord + 2].operands[0].imm == entry_failure
assert any(o.mnemonic == 'je' and o.operands[0].imm == entry_failure for o in body[mapcheck + 2:early_coord])
expected = [('movss', 'xmm0, dword ptr [rsi + 0x10]'), ('movss', 'xmm3, dword ptr [rsi + 0xc]'), ('movss', 'xmm2, dword ptr [rsi + 8]'), ('movss', 'xmm1, dword ptr [rsi + 4]')]
assert [(o.mnemonic, o.op_str) for o in body[early_coord + 3:early_coord + 7]] == expected
stores = at('rbx, [r14 + 0x2e0]', 'lea')
assert body[early_coord + 7].mnemonic == 'jmp' and body[early_coord + 7].operands[0].imm == body[stores].address, 'Entry must use native radians without a second degree conversion'
late_at = next(i for i, o in enumerate(body) if o.address == later)
assert [(o.mnemonic, o.op_str) for o in body[late_at:late_at + 3]] == [('cmp', 'eax, 7'), ('sbb', 'edx, edx'), ('add', 'edx, 0x1541')]
lookup = next(i for i, o in enumerate(body) if i > late_at and o.mnemonic == 'mov' and o.op_str == 'rsi, qword ptr [rax + rdx*8]')
identity = next(i for i, o in enumerate(body) if i > lookup and o.mnemonic == 'cmp' and o.op_str == 'dword ptr [rsi], edx')
failure = body[identity + 1].operands[0].imm
assert body[lookup - 2].mnemonic == 'jae' and body[lookup - 2].operands[0].imm == failure and body[lookup + 2].operands[0].imm == failure
mapcheck = next(i for i, o in enumerate(body) if i > identity and o.mnemonic == 'cmp' and o.op_str == 'word ptr [rsi + 0x1c], di')
assert body[mapcheck - 1].op_str == 'edi, 0x5d1' and body[mapcheck + 1].operands[0].imm == failure
coord = holy['coordinates'][3]
assert late_at < lookup < identity < mapcheck < coord and any(o.mnemonic == 'je' and o.operands[0].imm == failure for o in body[mapcheck + 2:coord])
assert body[coord + 2].mnemonic == 'jne' and body[coord + 2].operands[0].imm == body[holy['coord'] + 3].address and body[coord + 3].address == failure
for register, expected_value in (('xmm6', 3.141592653589793), ('xmm7', 180.0)):
    o = next(o for o in body[mapcheck:coord] if o.mnemonic == 'movss' and o.op_str.startswith(register + ',') and 'rip' in o.op_str)
    a = next(a for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)
    value, = struct.unpack_from('<f', n['binary'], n['offset'](o.address + o.size + a.mem.disp))
    assert abs(value - expected_value) < 1e-5
for diagnostic in (b'Blood graveyard: invalid native entry201678 for instance %u', b'Blood graveyard: invalid native WorldSafeLocs %u for instance %u'):
    assert diagnostic in n['binary']
refused = body[holy['refused']].address
assert any(o.mnemonic in ('je', 'jmp') and o.operands[0].imm == refused for o in body[early_coord:late_at])
assert any(o.mnemonic in ('je', 'jmp') and o.operands[0].imm == refused for o in body[coord:])
print('PASS linked Blood cemetery:registered940 branch,step<3 uses guarded201678 native-radian entry,3..6 selects5440 and>=7 selects5441,indexed native ID/map/zero/finite guards,degree conversion and proven shared stores,diagnostic/null failures;Holy and MM branches/dispatch/progression exclusions retained. Native barrier acknowledgement/release tested separately;full client remains separate.')
