"""Verify action87's nonempty guard and initialized-pool RNG bound in linked PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP, X86_REG_RSP

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n = p['native']
body = p['instructions']('?ProcessAction@SmartScript@@')
refs = []
for i, x in enumerate(body):
    for o in x.operands:
        if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP:
            try:
                at = n['offset'](x.address + x.size + o.mem.disp)
                if b' has no nonzero timed action list entries.' in n['binary'][at:at+240].split(b'\0')[0]:
                    refs.append(i)
            except ValueError:
                pass
assert len(refs) == 1, 'Missing or ambiguous empty action87 pool diagnostic in actual dispatcher'
ref = refs[0]
guards = [i for i in range(max(0, ref-90), ref)
          if body[i].mnemonic == 'test' and body[i].op_str == 'edx, edx'
          and body[i+1].mnemonic == 'jne' and body[i+1].operands[0].imm > body[ref].address]
assert len(guards) == 1, 'Missing count guard around diagnostic and RNG'
guard = guards[0]
target = body[guard+1].operands[0].imm
random = next(i for i,x in enumerate(body) if x.address == target)
assert body[random].mnemonic == 'dec' and body[random].op_str == 'edx', 'Nonempty path must decrement inclusive RNG maximum'
assert body[random+1].mnemonic == 'xor' and body[random+1].op_str == 'ecx, ecx', 'RNG lower bound changed'
call = body[random+2]
assert call.mnemonic == 'call' and call.operands[0].type == CS_OP_IMM
assert n['resolve'](call.operands[0].imm) == n['symbol']('?urand@@'), 'Bounded RNG is not the native inclusive urand'
load = body[random+4]
assert load.mnemonic == 'mov' and load.operands[1].type == CS_OP_MEM
pool = load.operands[1].mem
assert pool.base == X86_REG_RSP and pool.scale == 4 and load.reg_name(pool.index) == 'rax'
assert body[random+3].op_str == 'eax, eax', 'Unexpected RNG index conversion'
stores = [x for x in body[max(0,guard-24):guard]
          if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
          and x.operands[0].mem.base == pool.base and x.operands[0].mem.disp == pool.disp
          and x.operands[0].mem.scale == 4]
assert len(stores) == 1, 'RNG selection no longer reads the populated compact pool'
assert any(x.mnemonic == 'inc' and x.op_str == 'edx' for x in body[guard-12:guard]), 'Missing populated entry counter'
assert any(x.mnemonic == 'jmp' and x.operands[0].imm > body[random+4].address
           for x in body[ref:random]), 'Empty-pool diagnostic path must leave selection'
print('PASS linked SmartScript::ProcessAction action87: populated pool counter, empty-pool diagnostic/exit, count-1 inclusive native urand and indexed read from that same initialized pool. Native ASan regression separately covers all 192 nonempty mask/choice pairs and dispatch.')
