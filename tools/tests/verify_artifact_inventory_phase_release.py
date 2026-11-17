"""Verify real artifact inventory phase notifications in the linked Release PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
notify = native['symbol']('?NotifyConditionChanged@PhaseMgr@@')
for name in ('?ItemAddedQuestCheck@Player@@', '?ItemRemovedQuestCheck@Player@@'):
    body = instructions(name)
    calls = [(i, native['resolve'](x.operands[0].imm)) for i, x in enumerate(body)
             if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM]
    site = next(i for i, target in calls if target == notify)
    quest = native['symbol']('?GetQuestTemplate@QuestDataStoreMgr@@')
    assert site < next(i for i, target in calls if target == quest), 'Notification lost behind deliver-quest return'
    # The actual item type check and world/count/null guards bypass notification,
    # then converge on the unchanged quest loop.
    bypasses = [x.operands[0].imm for x in body[:site] if x.mnemonic == 'je']
    assert len(bypasses) == 4 and len(set(bypasses)) == 1
    assert bypasses[0] == body[site + 1].address
    assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM and
               x.operands[-1].type == CS_OP_IMM and x.operands[-1].imm == 8
               for x in body[max(0, site - 6):site]), 'Wrong native phase condition bit'
    assert any(x.mnemonic == 'cmp' and x.op_str.startswith('byte ptr [rax + ') for x in body[:site])
print('PASS linked canonical Player.obj: guarded artifact inventory notifications precede both native quest loops and use ITEM_EQUIPPED phase subscriptions; not client visibility proof')
