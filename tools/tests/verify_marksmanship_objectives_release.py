"""Verify scoped objective-order branches and their actual shared vector insertion."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
body = instructions('?LoadQuestObjective@Quest@@')
by_address = {entry.address: index for index, entry in enumerate(body)}
insertions = []
for quest, identities in ((40419, (281309, 281978, 281310, 281311, 281980, 281574)),
                          (41105, (281862, 286343, 286344, 281863, 282354))):
    gate = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and
                entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == quest)
    assert body[gate + 1].mnemonic == 'jne'
    for index, identity in enumerate(identities):
        entry = body[gate + 2 + index]
        assert entry.mnemonic == 'mov' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == identity
    end = body[gate + 1].operands[0].imm
    branch = [entry for entry in body[gate + 2:] if entry.address < end]
    calls = [index for index, entry in enumerate(branch) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
    assert len(calls) >= 2
    assert native['resolve'](branch[calls[0]].operands[0].imm) == native['resolve'](branch[calls[1]].operands[0].imm)
    assert branch[calls[0] + 1].op_str == 'rsi, rax', 'New rank not retained'
    second = calls[1]
    assert branch[second - 3].op_str == 'r8d, dword ptr [rbx]', 'Existing identity not used'
    assert branch[second + 1].op_str == 'rax, rsi' and branch[second + 2].mnemonic == 'ja'
    assert branch[second + 3].op_str == 'rbx, 0x60', 'Objective stride changed'
    insertions.append(branch[second + 2].operands[0].imm)
assert insertions[0] == insertions[1], 'Scoped branches do not converge on real native insertion'
at = by_address[insertions[0]]
assert body[at].op_str == 'r8, rbx', 'Actual selected vector position lost'
call = next(index for index in range(at + 1, at + 6) if body[index].mnemonic == 'call')
assert not any(entry.operands and entry.operands[0].type == CS_OP_REG and entry.reg_name(entry.operands[0].reg) in {'r8', 'r8d'}
               for entry in body[at + 1:call]), 'Insertion position overwritten'
assert native['symbol']('?LoadQuestObjective@Quest@@') in native['calls']('?LoadQuests@QuestDataStoreMgr@@')
print(f'PASS {native["args"].release}: actual40419/41105 identities, rank loops, shared selected-position insertion and live DB loader. Storage isolation/save/load are native-host checks, not client traversal.')
