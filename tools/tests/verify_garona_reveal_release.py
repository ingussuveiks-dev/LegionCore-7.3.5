"""Actual selected reveal factory/vtable and accepted native factory state."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
method = lambda name: '?' + name + '@npc_garona_halforcen_outroAI@npc_garona_halforcen_outro@@'
body, start, reset, factory = [ins(x) for x in (method('UpdateAI'), method('DoAction'), method('Reset'), '?GetAI@npc_garona_halforcen_outro@@')]
pairs = lambda rows: [(x.mnemonic, x.op_str) for x in rows]
values = lambda rows: {op.imm for x in rows for op in x.operands if op.type == CS_OP_IMM}

def calls(rows, prefix):
    targets = {n['resolve'](value) for name, value in n['symbols'].items() if name.startswith(prefix)}
    return [i for i, op in enumerate(rows) if op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets]

assert {214940, 2449, 23000} <= values(start)
assert ('cmp', 'byte ptr [rcx + 0x38], 0') in pairs(start)
latch = pairs(start).index(('mov', 'byte ptr [rcx + 0x38], 1'))
assert start[latch - 1].mnemonic == 'jne' and start[latch - 1].operands[0].imm > start[latch].address
assert all(i > latch for i in calls(start, '?CreateConversation@Conversation@@'))
assert pairs(reset)[:2] == [('cmp', 'byte ptr [rcx + 0x38], 0'), ('jne', reset[1].op_str)]
assert next(x for x in reset if x.address == reset[1].operands[0].imm).mnemonic == 'ret'
assert calls(reset, '?Reset@EventMap@@')
assert {108325, 251107, 2450, 11000, 2000, 86400} <= values(body)
visual, = calls(body, '?SummonCreature@WorldObject@@')
artifact, = calls(body, '?SummonGameObject@WorldObject@@')
assert visual < artifact
for at, register in ((visual, 'r15'), (artifact, 'r14')):
    assert pairs(body[at + 1:at + 4]) == [('test', 'rax, rax'), ('setne', 'cl'), ('mov', f'byte ptr [{register}], cl')]
assert ('lea', 'r15, [rbx + 0x61]') in pairs(body[:visual])
assert ('lea', 'r14, [rbx + 0x60]') in pairs(body[visual:artifact])
assert ('cmp', 'byte ptr [r15], dil') in pairs(body[:visual])
assert ('cmp', 'byte ptr [r14], 0') in pairs(body[visual:artifact])
assert ('mov', 'byte ptr [rbx + 0x62], 1') in pairs(body[artifact:])
assert ('mov', 'r8d, 0x7d0') in pairs(body[artifact:]) and calls(body, '?ScheduleEvent@EventMap@@')
for forbidden in ('?AddItem@Player@@', '?UpdateAchievementCriteria@Player@@', '?CompleteQuest@Player@@', '?KilledMonsterCredit@Player@@'):
    assert not calls(body + start, forbidden)
text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
vtable, = re.findall(r'\s+\w+:\w+\s+\?\?_7npc_garona_halforcen_outroAI@npc_garona_halforcen_outro@@6B@\s+([0-9a-fA-F]{16})', text)
address = int(vtable, 16)
refs = {x.address + x.size + op.mem.disp for x in factory for op in x.operands if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}
assert address in refs
slots = struct.unpack_from('<82Q', n['binary'], n['offset'](address))
slot, = [i for i, value in enumerate(slots) if n['resolve'](value) == n['symbol'](method('UpdateAI'))]
assert any(x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == slot * 8 for x in ins('?Update@Creature@@'))
assert n['symbol'](method('DoAction')) in slots and n['symbol'](method('Reset')) in slots
print('PASS selected native Garona reveal factory/core updater: one-shot start, Reset preserves earned events, original23+11s/voices, separate accepted visual/artifact states from actual nonnull factories,2s retry, single movement and no progression/item grants')
