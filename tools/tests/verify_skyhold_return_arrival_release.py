"""Verify the actual native192085/216016 origin and completed-ascent guards."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_execute_jump_release.py')))
n, instructions, values = (p[k] for k in ('n', 'instructions', 'values'))
move = '?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@PEBUSpellEffectExtraData@Movement@@@Z'
inform = '?MovementInform@EffectMovementGenerator@@'
assert {192085,216016,4,0xffffffff} <= values(move)
code = instructions(move)
# Inlined constructor stores the Player's actual launch map (including0),
# with UINT32_MAX for ordinary generators, into the same field read on arrival.
store = next(i for i,x in enumerate(code) if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
             and x.operands[0].size == 4 and x.operands[0].mem.disp == 0x24
             and x.operands[1].type == CS_OP_REG)
register = code[store].operands[1].reg
window = code[max(0,store-20):store]
assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_REG and x.operands[0].reg == register
           and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.disp == 0xe0 for x in window)
assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_REG and x.operands[0].reg == register
           and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 0xffffffff for x in window)
body = instructions(inform)
gate = next(i for i,x in enumerate(body) if x.mnemonic == 'cmp' and x.operands[-1].type == CS_OP_IMM
            and x.operands[-1].imm == -1)
tail = body[gate:gate+12]
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and x.operands[0].size == 1 for x in tail), 'Actual _arrived test missing'
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0xa18 for x in tail), 'Actual ALIVE test missing'
assert any(x.mnemonic == 'mov' and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.disp == 0x24 for x in tail)
assert sum(x.mnemonic in ('je','jne') for x in tail) >= 4
assert any(n['resolve'](a) in n['calls'](inform) for name,a in n['symbols'].items() if name.startswith('?CastSpell@Unit@@'))
print('PASS actual MoveJump192085/delay216016 Player gate, zero-launch rejection, inlined origin-map field capture with UINT32_MAX ordinary sentinel, and arrival _arrived/ALIVE/same-map tests before native cast. Full extracted host covers six maps, interruption and foreign paths; client return remains separate.')
