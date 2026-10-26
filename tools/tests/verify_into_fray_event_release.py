"""Verify the selected map command7 event completion path, retaining legacy diagnostics."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
helper = '?CompleteScriptedQuestEvent@'
body = instructions(helper)


def sites(prefix):
    target = native['symbol'](prefix)
    return [i for i, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


lookup, = sites('?GetQuestTemplate@QuestDataStoreMgr@@')
slot, = sites('?FindQuestSlot@Player@@')
status, = sites('?GetQuestStatus@Player@@')
complete, = sites('?CompleteQuest@Player@@')
legacy, = sites('?AreaExploredOrEventHappens@Player@@')
assert lookup < slot < status < complete < legacy
assert body[lookup + 1].op_str == 'rax, rax' and body[lookup + 2].mnemonic == 'je'
objective_gate = next(i for i, e in enumerate(body[:slot]) if e.mnemonic == 'cmp' and e.op_str == 'qword ptr [rax + 0x168], r8')
assert any(e.op_str == 'r8, qword ptr [rax + 0x170]' for e in body[:objective_gate])
assert body[objective_gate + 1].mnemonic == 'jne' and body[objective_gate + 1].operands[0].imm == body[legacy].address
assert body[slot + 1].op_str == 'ax, 0x19' and body[slot + 2].mnemonic == 'jae'
assert body[status + 1].op_str == 'eax, 3' and body[status + 2].mnemonic == 'jne'
assert body[complete].mnemonic == 'jmp', 'Selected direct native CompleteQuest tail missing'
assert native['symbol'](helper) in native['calls']('?ScriptsProcess@Map@@')
assert native['symbol']('?FailQuest@Player@@') in native['calls']('?ScriptsProcess@Map@@')
assert b'Deprecated function AreaExploredOrEventHappens called for quest' in native['binary']
print('PASS supplied Release Map::ScriptsProcess selects command7 helper: actual missing-quest and empty-objective vector branch, active slot<25/statusINCOMPLETE3 before CompleteQuest; objective-bearing legacy handler/diagnostic and native failure path retained. Required spell189764 event flag is validated by data/native catalog, not removed. Not actual client acceptance/turn-in proof.')
