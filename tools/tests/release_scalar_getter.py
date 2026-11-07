"""Evaluate bounded, side-effect-free integer GetData leaves from selected PEs.

MSVC can rebalance the switch when a new key is appended. Decode its actual
branches and loads instead of assuming the previous subtract-chain layout.
Unsupported instructions, memory writes, calls and out-of-leaf branches fail.
"""
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_REG, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RCX


def decode(native, prefix):
    start = native['symbol'](prefix)
    end = min(a for a in native['symbols'].values() if a > start)
    assert 0 < end-start <= 192, 'Unbounded scalar getter'
    decoder = Cs(CS_ARCH_X86, CS_MODE_64); decoder.detail = True
    body = list(decoder.disasm(native['binary'][native['offset'](start):native['offset'](end)], start))
    for op in body:
        assert op.mnemonic in {'cmp','sub','mov','movzx','xor','je','jne','ja','jbe','jmp','ret','int3','nop'}, op
        if op.mnemonic in {'mov','movzx','sub','xor'}:
            assert op.operands[0].type == CS_OP_REG, 'Scalar getter writes memory'
    return body


def evaluate(body, key, fields):
    regs={'eax':0,'edx':key & 0xffffffff}; pc=0; zero=False; below=False
    addresses={op.address:i for i,op in enumerate(body)}
    def value(op):
        if op.type == CS_OP_IMM: return op.imm & 0xffffffff
        if op.type == CS_OP_REG: return regs[body[pc].reg_name(op.reg)]
        assert op.type==CS_OP_MEM and op.mem.base==X86_REG_RCX and not op.mem.index
        return fields[op.mem.disp] & ((1 << (8*op.size))-1)
    for _ in range(100):
        op=body[pc]; m=op.mnemonic; args=op.operands
        if m=='ret': return regs['eax']
        if m in ('cmp','sub'):
            left,right=value(args[0]),value(args[1]); zero=left==right; below=left<right
            if m=='sub': regs[op.reg_name(args[0].reg)]=(left-right)&0xffffffff
        elif m in ('mov','movzx'): regs[op.reg_name(args[0].reg)]=value(args[1])
        elif m=='xor':
            regs[op.reg_name(args[0].reg)]=value(args[0])^value(args[1]); zero=regs[op.reg_name(args[0].reg)]==0;below=False
        elif m in ('je','jne','ja','jbe','jmp'):
            take={'je':zero,'jne':not zero,'ja':not zero and not below,'jbe':zero or below,'jmp':True}[m]
            if take: pc=addresses[args[0].imm];continue
        else: assert m=='nop', 'Reached padding/unsupported leaf instruction'
        pc+=1
    raise AssertionError('Scalar getter did not terminate')
