"""Shared proof of the actual native numeric bound before encounter callbacks."""
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

def check(native, instructions):
    body = instructions('?DealDamage@Unit@@')
    candidates = [i for i, x in enumerate(body) if x.mnemonic == 'divss' and
                  any(y.mnemonic == 'call' and y.operands[0].type == CS_OP_MEM and
                      y.operands[0].mem.disp == 0xa8 for y in body[i + 1:i + 90])]
    assert len(candidates) == 1, 'Expected one final normalization before native content callbacks'
    division, = candidates
    assert body[division - 1].mnemonic == 'call' and body[division - 1].op_str == 'qword ptr [rax + 0x180]'
    assert body[division].op_str == 'xmm6, xmm0'
    assert body[division + 1].mnemonic == 'cvtps2pd' and body[division + 1].op_str == 'xmm1, xmm6'
    limit = body[division + 2]
    assert limit.mnemonic == 'movsd' and limit.reg_name(limit.operands[0].reg) == 'xmm2' and limit.operands[1].type == CS_OP_MEM and limit.operands[1].mem.base == X86_REG_RIP
    address = limit.address + limit.size + limit.operands[1].mem.disp
    assert struct.unpack_from('<d', native['binary'], native['offset'](address))[0] == 4294967295.0
    clamp, convert, store = body[division + 3:division + 6]
    assert (clamp.mnemonic, clamp.op_str) == ('minsd', 'xmm2, xmm1')
    assert convert.mnemonic == 'cvttsd2si' and convert.operands[0].size == 8 and convert.op_str.endswith(', xmm2')
    register = convert.reg_name(convert.operands[0].reg)
    low = {'rax': 'eax', 'rcx': 'ecx'}[register]
    assert (store.mnemonic, store.op_str) == ('mov', 'dword ptr [rsp + 0x60], ' + low), 'Clamped conversion must feed the actual mutable damage argument'
    hooks = [i for i in range(division + 6, division + 90) if body[i].mnemonic == 'call' and
             body[i].operands[0].type == CS_OP_MEM and body[i].operands[0].mem.disp == 0xa8]
    assert len(hooks) == 2, 'Both primary/suspended final-damage policies must follow saturation'
    for index in hooks:
        assert any(x.mnemonic == 'lea' and x.op_str == 'r8, [rsp + 0x60]' for x in body[index - 5:index])
        assert any(x.mnemonic == 'mov' and x.op_str == 'rdx, r14' for x in body[index - 4:index])
        post = body[index + 1:index + 6]
        if post[0].mnemonic == 'cmp':
            assert post[0].op_str == 'dword ptr [rsp + 0x60], 0' and post[1].mnemonic == 'je'
            assert post[2].mnemonic == 'cmp' and post[2].op_str == 'dword ptr [rbx + 0xa18], 0' and post[3].mnemonic == 'jne', 'Zero damage and dead target must leave the final policy path'
            assert post[1].operands[0].imm == post[3].operands[0].imm > body[hooks[-1]].address
        else:
            assert post[0].op_str == 'ecx, dword ptr [rsp + 0x60]' and post[1].op_str == 'ecx, ecx'
    assert any(x.mnemonic == 'mov' and x.op_str == 'r10, qword ptr [rcx + 0x120]' for x in body[division:hooks[0]])
    comparisons = [x for x in body[hooks[0]:hooks[1]] if x.mnemonic == 'cmp' and x.op_str in ('r10, qword ptr [rbx + 0x9c0]', 'rcx, qword ptr [rbx + 0x9c0]')]
    assert len(comparisons) == 1
    disabled = comparisons[0].op_str.split(',')[0]
    assert any(x.mnemonic == 'mov' and x.op_str == disabled + ', qword ptr [rbx + 0x9c8]' for x in body[hooks[0]:hooks[1]]), 'Compared suspended AI comes from the real disabled-AI field'
    print('PASS actual Unit::DealDamage float normalization -> exact double UINT32_MAX clamp -> integer store -> native instance/primary/suspended guards; in-range rounding retained, no forced encounter credit')
    return division
