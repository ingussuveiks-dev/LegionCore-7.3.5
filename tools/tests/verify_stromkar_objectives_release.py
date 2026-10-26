"""Inspect scoped story-order insertion in the canonical quest loader."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_skyhold_jump_destination_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?LoadQuestObjective@Quest@@')
gate = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and
            entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 41105)
assert body[gate + 1].mnemonic == 'jne'
for offset, identity in enumerate((281862, 286343, 286344, 281863, 282354)):
    entry = body[gate + 2 + offset]
    assert entry.mnemonic == 'mov' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == identity, 'Story order differs from pinned735 objective identities'
branch_end = body[gate + 1].operands[0].imm
branch = [entry for entry in body[gate + 2:] if entry.address < branch_end]
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(branch)
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
assert len(calls) == 3 and calls[0][1] == calls[1][1], 'Reinspect native rank lookup and vector insertion'
assert branch[calls[0][0] + 1].op_str == 'rsi, rax', 'New objective rank not retained'
second = calls[1][0]
assert branch[second - 3].op_str == 'r8d, dword ptr [rbx]', 'Existing objective identity not used'
assert branch[second + 1].mnemonic == 'cmp' and branch[second + 1].op_str == 'rax, rsi'
assert branch[second + 2].mnemonic == 'ja', 'Insertion does not stop at first greater story rank'
assert branch[second + 3].op_str == 'rbx, 0x60', 'Objective vector iteration changed'
insert = calls[2][0]
position = [index for index in range(max(0, insert - 5), insert) if branch[index].op_str == 'r8, rbx']
assert len(position) == 1, 'Native insertion does not use selected position'
assert not any(entry.operands and entry.operands[0].type == CS_OP_REG and entry.reg_name(entry.operands[0].reg) in {'r8', 'r8d'}
               for entry in branch[position[0] + 1:insert]), 'Selected insertion position overwritten'
assert calls[2][1] in native['calls']('?LoadQuestObjective@Quest@@')
assert native['symbol']('?LoadQuestObjective@Quest@@') in native['calls']('?LoadQuests@QuestDataStoreMgr@@'), 'Live database loader bypasses actual quest objective loader'
print('PASS: canonical41105-only loader uses exact five native story identities, retains new rank, compares each existing rank and inserts at the actual selected vector position; live LoadQuests reaches it. Native host verifies save-slot isolation/persistence; client display and actor credit remain separate')
