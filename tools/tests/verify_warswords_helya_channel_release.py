"""Require acknowledged native Helya aura before arming the original introduction."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_intro_release.py')))
native = previous['native']
instructions = previous['instructions']
actor = 'npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
opening = '?MoveInLineOfSight@' + actor
body = instructions(opening)
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]


def sites(prefix):
    return [index for index, target in calls if target == native['symbol'](prefix)]


auras = sites('?HasAura@Unit@@')
casts = sites('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')
dialogue = sites('?CreateConversation@Conversation@@')
phase = sites('?InSamePhase@WorldObject@@')
assert len(auras) == 2 and len(casts) == 1 and len(dialogue) == 1 and len(phase) == 2, 'Missing initial/post-cast aura or phase acknowledgement'
assert phase[0] < auras[0] < casts[0] < phase[1] < auras[1] < dialogue[0], 'Dialogue can precede the visual acknowledgement path'
for index in auras:
    assert any(entry.mnemonic == 'mov' and entry.op_str == 'edx, 0x2f9f6' for entry in body[index - 9:index]), 'Aura check no longer uses native195062'
    assert any(entry.mnemonic == 'lea' and entry.op_str == 'r8, [rsp + 0x70]' for entry in body[index - 9:index]), 'Helya-caster GUID filter lost'
    assert body[index - 1].op_str == 'rcx, rbx' and any(entry.mnemonic == 'mov' and entry.op_str == 'rbx, qword ptr [r13]' for entry in body[index - 17:index]), 'Aura acknowledgement is not on actual Vigfus'
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?GetGuidValue@Object@@') and body[position - 1].op_str == 'rcx, r14' and body[position - 2].op_str == 'edx, edx' for position, entry in enumerate(body[index - 17:index], start=index - 17)), 'Aura caster GUID does not come from selected Helya OBJECT_FIELD_GUID'
    assert body[index + 1].op_str == 'al, al' and body[index + 2].mnemonic == 'jne', 'Aura result is no longer a success branch'
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r9d, r9d' for entry in body[casts[0] - 8:casts[0]]), 'Original normal rather than triggered cast lost'
assert any(entry.mnemonic == 'lea' and entry.op_str == 'r13, [rdi + 0x20]' for entry in body[:casts[0]]), 'Actual final actor pointer lost'
assert body[casts[0] - 2].op_str == 'rdx, qword ptr [r13]' and body[casts[0] - 1].op_str == 'rcx, r14', 'Native channel is not selected Helya to actual Vigfus'
assert any(entry.mnemonic == 'test' and entry.op_str == 'eax, eax' and body[index + 1].mnemonic == 'jne' for index, entry in enumerate(body[casts[0] + 1:auras[1]], start=casts[0] + 1)), 'Failed casts no longer reject the intro'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rdi + 0xc6], 0' for entry in body[auras[1]:dialogue[0]]), 'Failed visual permanently consumes introDone'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [r12], 0' for entry in body[auras[1]:dialogue[0]]), 'Failed visual leaves delayed intro pending'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rcx + 0x100], 0' and body[index + 1].mnemonic == 'jne' for index, entry in enumerate(body[:-1])), 'LOS ignores visual retry throttle'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rdi + 0x100], 0x3e8' for entry in body[:auras[0]]), 'Visual retry is not bounded to one second'
created = body[dialogue[0] + 2]
assert created.mnemonic == 'jne', 'Native conversation result is not a success branch'
success = next(index for index, entry in enumerate(body) if entry.address == created.operands[0].imm)
after_dialogue = body[success:success + 11]
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'ebx, dword ptr [rax]' for entry in after_dialogue) and any(entry.mnemonic == 'je' for entry in after_dialogue), 'Conversation reset can arm an obsolete transition'
update = instructions('?UpdateAI@' + actor)
tick = next(index for index, entry in enumerate(update) if entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rcx + 0x100], eax')
assert any(entry.mnemonic == 'cmovbe' for entry in update[:tick]), 'Visual countdown can underflow'
retry = next(index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?UpdateIntro@' + actor))
assert tick < retry, 'No-victim stationary retry does not tick the visual throttle first'
strings = native['binary']
for diagnostic in (b'Warswords intro waiting for live same-phase Helya99043', b'Warswords intro waiting for native Helya195062 aura', b'Warswords intro recovered native Helya195062 aura'):
    assert diagnostic in strings, 'Visual failure/recovery diagnostic absent'
print('PASS: canonical selected final intro checks live Helya/phase and her195062 aura on actual Vigfus before843/31s handoff, normal Helya-to-Vigfus cast plus post-cast recipient/caster acknowledgement, rollback and post-dialogue epoch guard, bounded pre-retry countdown and visible diagnostics. Native spell rendering, full combat/cinematic/client traversal remain separate.')
