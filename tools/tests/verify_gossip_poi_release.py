"""Verify valid gossip POIs bypass clearing in the selected linked loader."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, instructions = p['native'], p['instructions']
body = instructions('?LoadGossipMenuItems@GossipDataStoreMgr@@')
lookup = n['symbol']('?GetPointOfInterest@QuestDataStoreMgr@@')
calls = [i for i, x in enumerate(body) if x.mnemonic == 'call'
         and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) == lookup]
assert len(calls) == 1
i = calls[0]
argument = body[i - 2]
assert argument.mnemonic == 'mov' and argument.op_str.startswith('edx, dword ptr ')
poi = argument.operands[1].mem
assert body[i + 1].mnemonic == 'test' and body[i + 1].op_str == 'rax, rax'
branch = body[i + 2]
assert branch.mnemonic == 'jne', 'Existing POI must take success branch'
target = next(j for j, x in enumerate(body) if x.address == branch.operands[0].imm)
clear = body[target - 1]
assert clear.mnemonic == 'mov' and clear.operands[0].type == CS_OP_MEM
assert (clear.operands[0].mem.base, clear.operands[0].mem.disp) == (poi.base, poi.disp)
assert clear.operands[1].type == CS_OP_REG
zero = body[target - 2]
assert zero.mnemonic == 'xor' and zero.operands[0].reg == clear.operands[1].reg == zero.operands[1].reg
assert branch.operands[0].imm == clear.address + clear.size, 'Valid POI still passes through clearing'
send = '?SendPointOfInterest@PlayerMenu@@'
assert n['symbol'](send) in n['calls']('?OnGossipSelect@Player@@')
assert n['symbol']('?Write@GossipPOI@NPC@WorldPackets@@') in n['calls'](send)
print('PASS linked loader preserves valid POIs and skips the invalid-only zero store; native selection/send/packet consumers retained. Client rendering pending.')
