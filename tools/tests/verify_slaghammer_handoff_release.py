"""Verify the linked static Slaghammer owner/acknowledgement route."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, ins = n['native'], n['instructions']

def calls(body, prefix):
    addresses = {native['resolve'](value) for name, value in native['symbols'].items() if name.startswith(prefix)}
    return [i for i, x in enumerate(body) if x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_IMM and native['resolve'](x.operands[0].imm) in addresses]

def operand(body, text):
    return next(i for i, x in enumerate(body) if x.op_str == text)

factory = ins('?GetAI@npc_slaghammer_handoff@@')
candidates = []
for x in factory:
    if x.mnemonic == 'lea' and len(x.operands) == 2 and x.operands[1].type == CS_OP_MEM and x.operands[1].mem.base == X86_REG_RIP:
        table = x.address + x.size + x.operands[1].mem.disp
        slots = struct.unpack_from('<128Q', native['binary'], native['offset'](table))
        if slots[0x110 // 8] == native['symbol']('?UpdateLifecycle@AI@SlaghammerHandoff@@'):
            candidates.append(slots)
slots, = candidates
for method in ('?MoveInLineOfSight@AI@SlaghammerHandoff@@', '?SetData@AI@SlaghammerHandoff@@', '?UpdateLifecycle@AI@SlaghammerHandoff@@'):
    assert slots.count(native['symbol'](method)) == 1
assert slots[0x110 // 8] == native['symbol']('?UpdateLifecycle@AI@SlaghammerHandoff@@')
life = ins('?UpdateLifecycle@AI@SlaghammerHandoff@@')
assert len(calls(life, '?Update@State@SlaghammerHandoff@@')) == 1

intro = ins('?MoveInLineOfSight@AI@SlaghammerHandoff@@')
receipt, = calls(intro, '?GetQuestCancellationReceipt@Player@@')
events = calls(intro, '?HasEvent@SlaghammerHandoff@@')
native_intro, = [i for i in calls(intro, '?MoveInLineOfSight@SmartAI@@') if events[0] < i < events[1]]
assert receipt < events[0] < native_intro < events[1]
assert intro[events[0] - 2].op_str == 'edx, 0xbbf9' and intro[events[1] - 2].op_str == 'edx, 0xbbf9'
assert operand(intro, 'xmmword ptr [r14 + 4], xmm0') > events[1]
assert operand(intro, 'xmmword ptr [r14 + 0x14], xmm0') > events[1]
assert operand(intro, 'qword ptr [r14 + 0x28], rax') > events[1]
assert not calls(intro, '?UpdateAchievementCriteria@Player@@')  # Original SmartAI retains root0 credit.

update = ins('?Update@State@SlaghammerHandoff@@')
path, = calls(update, '?StartPath@SmartAI@@')
credit, = calls(update, '?UpdateAchievementCriteria@Player@@')
texts = calls(update, '?SendChat@CreatureTextMgr@@')
assert len(texts) == 2 and path < texts[0] < credit < texts[1]
assert operand(update, 'r8d, 0x18c36') < path
assert update[path + 1].op_str == 'al, al' and update[path + 2].mnemonic == 'je'
assert operand(update, 'dword ptr [r14 + 0x3c], 0x1f40') < texts[0]
assert operand(update, 'dword ptr [r14 + 0x3c], 0x2710') < texts[1]
assert update[credit - 2].op_str == 'r8d, 0xbc20' and update[credit - 3].op_str == 'edx, 0x5c'
ack = calls(update, '?Companion@State@SlaghammerHandoff@@')
retire, = calls(life, '?StartDespawn@SmartAI@@')
arm, = calls(life, '?SetDespawnTime@SmartAI@@')
dispatch, = calls(life, '?Update@State@SlaghammerHandoff@@')
assert dispatch < arm < retire
events = calls(update, '?HasEvent@SlaghammerHandoff@@')
assert any(update[i - 2].op_str == 'edx, 0xbcb2' for i in events if i < ack[0])
assert update[ack[0] + 1].op_str == 'al, al' and update[ack[0] + 2].mnemonic == 'je'
assert next(i for i, x in enumerate(update) if x.mnemonic == 'mov' and x.op_str == 'dword ptr [r14], 5') > ack[0]
assert operand(life, 'byte ptr [rbx + 0x258], bpl') > dispatch
assert operand(life, 'dword ptr [rdi], 5') < arm
assert operand(life, 'byte ptr [rbx + 0x258], 1') < arm
cast, = calls(update, '?CastSpell@Unit@@')
pending, = calls(update, '?FindCurrentSpellBySpellId@Unit@@')
assert ack[1] < pending < cast
assert update[pending - 2].op_str == 'edx, 0x3112c'
assert update[pending + 1].op_str == 'rax, rax' and update[pending + 2].mnemonic == 'jne'
assert [x.op_str for x in update[cast - 4:cast]] == ['r9d, r9d', 'r8d, 0x3112c', 'rdx, r15', 'rcx, r15']
assert update[cast + 2].op_str == 'dword ptr [r14 + 0x3c], 0x1388'
assert update[cast + 3].op_str == 'eax, eax' and update[cast + 4].mnemonic == 'je'
assert not calls(update, '?AddItem@Player@@') and not calls(update, '?KilledMonsterCredit@Player@@')
print('PASS selected static101430 derived SmartAI factory/lifecycle; original48121 admission stores full source/owner GUIDs and quest receipt after native root0, native path101430/8s+10s/owner48160 retained; acknowledgement requires48306 and actual scoped companion before retirement; pending201004 casts are not duplicated, native nontriggered self-cast retries5s without item/kill-credit bypass. Native tests cover eligibility/cancellation/shared cleanup; missingLINK2 and client traversal remain OPEN.')
