"""Bind the ambush provenance field across constructor and both native hooks."""
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RCX, X86_REG_R9, X86_REG_RDI, X86_REG_RBX, X86_REG_EAX, X86_REG_ECX, X86_REG_RAX


def check_provenance_layout(factory, post_hook, pre_hook):
    fields = [ins.operands[1].mem.disp for ins in post_hook
              if ins.mnemonic == 'lea' and len(ins.operands) == 2
              and ins.operands[0].type == CS_OP_REG and ins.operands[0].reg == X86_REG_R9
              and ins.operands[1].type == CS_OP_MEM and ins.operands[1].mem.base == X86_REG_RCX
              and not ins.operands[1].mem.index]
    assert len(fields) == 1, 'Post hook must pass one actual this-relative provenance field'
    field = fields[0]
    assert field >= 0x10 and field % 4 == 0, 'Invalid provenance field alignment/location'

    def memory(ins, base, size):
        return (ins.operands[0].type == CS_OP_MEM and ins.operands[0].size == size
                and ins.operands[0].mem.base == base and not ins.operands[0].mem.index
                and ins.operands[0].mem.disp == field)

    assert any(ins.mnemonic == 'mov' and len(ins.operands) == 2 and memory(ins, X86_REG_RDI, 4)
               and ins.operands[1].type == CS_OP_IMM and ins.operands[1].imm == 0
               for ins in factory), 'New actor provenance field is not initialized to zero'
    allocations = []
    for i in range(len(factory) - 2):
        ins, call, alias = factory[i:i + 3]
        if (ins.mnemonic == 'mov' and len(ins.operands) == 2
                and ins.operands[0].type == CS_OP_REG and ins.operands[0].reg == X86_REG_ECX
                and ins.operands[1].type == CS_OP_IMM and call.mnemonic == 'call'
                and alias.mnemonic == 'mov' and len(alias.operands) == 2
                and alias.operands[0].type == CS_OP_REG and alias.operands[0].reg == X86_REG_RDI
                and alias.operands[1].type == CS_OP_REG and alias.operands[1].reg == X86_REG_RAX):
            allocations.append(ins.operands[1].imm)
    assert len(allocations) == 1 and allocations[0] >= field + 4, 'Provenance lies outside the actual allocated actor'
    assert any(ins.mnemonic == 'cmp' and len(ins.operands) == 2 and memory(ins, X86_REG_RCX, 4)
               and ins.operands[1].type == CS_OP_IMM and ins.operands[1].imm == 0
               for ins in pre_hook), 'Pre hook reads a different or uninitialized provenance field'
    assert any(ins.mnemonic == 'mov' and len(ins.operands) == 2 and memory(ins, X86_REG_RBX, 4)
               and ins.operands[1].type == CS_OP_REG and ins.operands[1].reg == X86_REG_EAX
               for ins in pre_hook), 'Pre hook does not retain the native scenario step in the same field'
    return field
