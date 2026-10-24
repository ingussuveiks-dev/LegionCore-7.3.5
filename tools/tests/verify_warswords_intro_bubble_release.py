"""Verify completed-intro addon cleanup in the selected final Vigfus Reset."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_helya_channel_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?Reset@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@')
removal = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == 'edx, 0x32270')
cleanup = next(index for index, entry in enumerate(body[removal:], start=removal) if entry.mnemonic == 'call')
assert native['resolve'](body[cleanup].operands[0].imm) == native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@'), 'Bubble cleanup does not use native aura removal'
pending = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rcx + 0xc7], 0')
clear = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rcx + 0xc6], 0')
done = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rcx + 0xc6], 0')
assert pending < clear < done < removal, 'Pending Reset no longer clears introDone before deciding addon cleanup'
assert body[pending + 1].mnemonic == 'je' and body[pending + 1].operands[0].imm == body[done].address, 'Pending and completed introductions share the wrong reset branch'
for condition in ('byte ptr [rcx + 0xc6], 0', 'eax, 0x1812a', 'dword ptr [rcx + 0xe0], 0x5e7'):
    position = next(index for index, entry in enumerate(body[:removal]) if entry.mnemonic == 'cmp' and entry.op_str == condition)
    branch = next(entry for entry in body[position + 1:removal] if entry.mnemonic.startswith('j'))
    assert branch.operands[0].type == CS_OP_IMM and branch.operands[0].imm > body[cleanup].address, 'Unfinished/other-entry/map Reset can remove the bubble'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'rcx, qword ptr [rbx + 0x20]' for entry in body[:removal]), 'Bubble removal lost the actual final creature receiver'
for method, dependency in (('?LoadCreaturesAddon@Creature@@', '?GetCreatureAddon@ObjectMgr@@'), ('?LoadCreaturesAddon@Creature@@', '?GetCreatureTemplateAddon@ObjectMgr@@'), ('?LoadCreaturesAddon@Creature@@', '?AddAura@Unit@@QEAAPEAVAura@@I'), ('?_EnterEvadeMode@CreatureAI@@', '?LoadCreaturesAddon@Creature@@')):
    assert native['symbol'](dependency) in native['calls'](method), 'Native addon/evade consumer missing: ' + dependency
addon = instructions('?LoadCreaturesAddon@Creature@@')
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'qword ptr [rcx + 0x3a88], 0' for entry in addon) and any(entry.mnemonic == 'lea' and entry.op_str == 'rdx, [rdi + 0x3a88]' for entry in addon), 'Inlined addon lookup no longer uses this creature persistent DB GUID'
poll = instructions('?UpdateIntro@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@')
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(poll) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
present = next(index for index, target in calls if target == native['symbol']('?HasAura@Unit@@'))
remove = next(index for index, target in calls if target == native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@'))
assert present < remove and poll[present - 1].op_str == 'edx, 0x32270' and poll[remove - 2].op_str == 'edx, 0x32270', 'Late addon cleanup no longer tests/removes205424'
for condition, jump in (('byte ptr [rcx + 0xc6], 0', 'je'), ('byte ptr [rcx + 0xc7], 0', 'jne'), ('eax, 0x1812a', 'jne'), ('dword ptr [rcx + 0xe0], 0x5e7', 'jne')):
    position = next(index for index, entry in enumerate(poll[:present]) if entry.mnemonic == 'cmp' and entry.op_str == condition)
    branch = poll[position + 1]
    assert branch.mnemonic == jump and branch.operands[0].imm > poll[remove].address, 'Pending/unfinished/other-entry/map poll can strip the starting bubble'
assert poll[present + 1].op_str == 'al, al' and poll[present + 2].mnemonic == 'je' and poll[present + 2].operands[0].imm > poll[remove].address, 'Poll repeatedly removes an absent aura'
assert poll[remove - 1].op_str == 'rcx, qword ptr [rsi + 0x20]', 'Late removal lost the actual final actor receiver'
death = instructions('?JustDied@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@')
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(death) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
remove = next(index for index, target in calls if target == native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@'))
talk = next(index for index, target in calls if target == native['symbol']('?Talk@CreatureAI@@QEAAXE'))
owner = next(index for index, target in calls if target == native['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@'))
assert remove < talk < owner and death[remove - 2].op_str == 'edx, 0x32270', 'Death-persistent bubble is not removed before dialogue/owner resolution'
for condition in ('eax, 0x1812a', 'dword ptr [rcx + 0xe0], 0x5e7'):
    position = next(index for index, entry in enumerate(death[:remove]) if entry.mnemonic == 'cmp' and entry.op_str == condition)
    branch = death[position + 1]
    assert branch.mnemonic == 'jne' and death[remove].address < branch.operands[0].imm < death[talk].address, 'Death cleanup strips unrelated entry/map or skips original dialogue'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'rcx, qword ptr [rbx + 0x20]' for entry in death[:remove]), 'Death cleanup lost the selected final creature receiver'
lifecycle = '?UpdateLifecycle@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
body = instructions(lifecycle)
remove = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@'))
for condition, jump in (('eax, 0x1812a', 'jne'), ('dword ptr [rax + 0xe0], 0x5e7', 'jne'), ('dword ptr [rax + 0xa18], 0', 'je')):
    position = next(index for index, entry in enumerate(body[:remove]) if entry.mnemonic == 'cmp' and entry.op_str == condition)
    assert body[position + 1].mnemonic == jump and body[position + 1].operands[0].imm > body[remove].address, 'Corpse lifecycle can remove an alive or unrelated actor bubble'
assert body[remove - 2].op_str == 'edx, 0x32270' and native['symbol']('?HasAura@Unit@@') in native['calls'](lifecycle), 'Corpse lifecycle does not conditionally remove actual205424'
slots = []
for entry in instructions('?GetAI@npc_vigfus_bladewind_last@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = struct.unpack_from('<128Q', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))
            if native['symbol'](lifecycle) in table and native['symbol']('?Reset@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@') in table:
                slots.append(table.index(native['symbol'](lifecycle)) * 8)
assert len(slots) == 1, 'Final factory does not select the corpse lifecycle override'
dispatch = [entry for entry in instructions('?Update@Creature@@') if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slots[0]]
assert len(dispatch) >= 2, 'Native active/suspended creature lifecycle dispatch missing'
print('PASS: selected canonical Reset/poll clean completed-intro205424 and late addon reload only on98602/1511; pending bubble retained. Selected death callback clears death-persistent aura before Talk2/owner lookup, including null killer; dead-only lifecycle clears a surviving corpse aura through native active/suspended dispatch. Native DB-GUID/addon/Aura/evade consumers retained. No rendered visual or full scenario proof.')
