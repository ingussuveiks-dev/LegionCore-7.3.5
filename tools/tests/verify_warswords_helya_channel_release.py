"""Require the installed late blessing, native recipient acknowledgement and full channel wait."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_intro_release.py')))
native = previous['native']
instructions = previous['instructions']
reachable = previous['reachable']
actor = previous['actor']
opening = previous['opening']
outer = previous['outer']
body = instructions(outer)


def sites(decoded, prefix):
    return [index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


initial = instructions(opening)
cast_prefix = '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@'
assert not sites(initial, cast_prefix) and not sites(initial, '?HasAura@Unit@@'), 'Intro still casts/reuses the blessing before843'
assert len(sites(initial, '?IsIntroHelya@' + actor)) == 1 and len(sites(initial, '?CreateConversation@Conversation@@')) == 1, 'Initial Helya admission or dialogue factory lost'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rcx + 0x100], 0' and initial[index + 1].mnemonic == 'jne' for index, entry in enumerate(initial[:-1])), 'LOS ignores visual retry throttle'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rdi + 0x100], 0x3e8' for entry in initial), 'Visual retry is not bounded to one second'
for register, destination in (('r14', 'xmm7'), ('rbx', 'xmm8')):
    assert any(initial[index - 1].op_str == 'rcx, ' + register and initial[index + 1].op_str == destination + ', xmmword ptr [rax]' for index in sites(initial, '?GetGuidValue@Object@@')), 'Initial player/Helya capture lost its actual128-bit identity'
assert all(any(entry.op_str == expected for entry in initial) for expected in ('xmmword ptr [rsp + 0xac], xmm7', 'xmmword ptr [rsp + 0xbc], xmm8')), 'Installed outer closure does not carry both original identities'

helya = instructions('?IsIntroHelya@' + actor)
for dependency in ('?GetUInt32Value@Object@@', '?IsInWorld@Object@@', '?IsPreDelete@Object@@', '?InSamePhase@WorldObject@@', '?GetExactDist2d@Position@@QEBAMPEBU1@@Z'):
    assert native['symbol'](dependency) in native['calls']('?IsIntroHelya@' + actor), 'Missing live Helya admission: ' + dependency
for condition in ('eax, 0x182e3', 'dword ptr [rbx + 0xa18], 0', 'qword ptr [rbx + 8], rax'):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == condition and helya[index + 1].mnemonic == 'jne' for index, entry in enumerate(helya[:-1])), 'Helya entry/death/map gate lost'
range_load = next(entry for entry in helya if entry.mnemonic == 'movss' and entry.op_str.startswith('xmm1, dword ptr [rip'))
range_address = range_load.address + range_load.size + range_load.operands[1].mem.disp
assert struct.unpack_from('<f', native['binary'], native['offset'](range_address))[0] == 100.0 and any(entry.mnemonic == 'comiss' and entry.op_str == 'xmm1, xmm0' and helya[index + 1].mnemonic == 'jb' for index, entry in enumerate(helya[:-1])), 'Helya live radius no longer100 yards'

cast = sites(body, cast_prefix)
auras = sites(body, '?HasAura@Unit@@')
targets = sites(body, '?IsIntroTarget@' + actor)
actors = sites(body, '?IsIntroHelya@' + actor)
timer = sites(body, '?AddDelayedEvent@FunctionProcessor@@')
assert len(cast) == len(auras) == len(timer) == 1 and len(targets) == len(actors) == 2, 'Missing late cast or before/after participant acknowledgement'
cast, aura, timer = cast[0], auras[0], timer[0]
assert targets[0] < actors[0] < cast < targets[1] < actors[1] < aura < timer, 'Late handoff can precede live native aura acknowledgement'
assert [entry.op_str for entry in body[cast - 4:cast]] == ['r9d, r9d', 'r8d, 0x2f9f6', 'rdx, qword ptr [rdx + 0x20]', 'rcx, r14'], 'Late normal195062 is not original Helya to actual Vigfus'
assert body[cast + 1].op_str == 'r13d, eax', 'Native cast result lost'
assert body[timer - 1].op_str == 'edx, 0x1b58' and body[timer - 3].op_str == 'rcx, 0x36a0', 'Engagement does not wait7000 from the actual acknowledgement'
assert all(any(entry.op_str == expected for entry in body[:cast]) for expected in ('r15, [rbx + 0x1c]', 'xmm0, xmmword ptr [r15]', 'r14, rax')), 'Late cast does not resolve the captured Helya GUID'
assert all(any(entry.op_str == expected for entry in body[aura - 12:aura]) for expected in ('xmm0, xmmword ptr [r15]', 'r8, [rsp + 0xc0]', 'edx, 0x2f9f6', 'rcx, qword ptr [rcx + 0x20]')), 'Native aura filter lost captured caster or actual Vigfus recipient'
assert body[aura + 1].op_str == 'al, al' and body[aura + 2].mnemonic == 'je', 'Accepted cast without native recipient aura can advance'
rejection_index = next(index + 1 for index in range(cast + 1, aura) if body[index].mnemonic == 'test' and body[index].op_str == 'eax, eax' and body[index + 1].mnemonic == 'jne')
assert body[rejection_index].operands[0].imm == body[aura + 2].operands[0].imm, 'Failed cast and missing recipient aura bypass common cancellation'
timer_address = body[timer].address
for branch in (body[rejection_index], body[aura + 2], body[targets[0] + 2], body[actors[0] + 2], body[targets[1] + 2], body[actors[1] + 2]):
    assert branch.mnemonic in ('je', 'jne') and timer_address not in reachable(body, branch.operands[0].imm), 'Rejected blessing still installs engagement'
for start, epoch_register, zero in ((0, 'eax', '0'), (cast + 1, 'edx', 'dil')):
    window = body[start:targets[0] if not start else targets[1]]
    assert any(entry.op_str == 'dword ptr [rsi], ' + epoch_register and entry.mnemonic == 'cmp' and window[index + 1].mnemonic == 'jne' for index, entry in enumerate(window[:-1])), 'Late cast reset-generation guard lost'
    assert any(entry.op_str == 'byte ptr [rcx + 0xc7], ' + zero and entry.mnemonic == 'cmp' and window[index + 1].mnemonic == 'je' for index, entry in enumerate(window[:-1])), 'Inactive late cast remains admitted'
clear = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == 'word ptr [rcx + 0xc6], 0')
assert body[clear + 1].op_str == 'dword ptr [rcx + 0x100], 0x3e8' and body[clear + 2].mnemonic == 'call' and native['resolve'](body[clear + 2].operands[0].imm) == native['symbol']('?RemoveIntroConversation@' + actor), 'Failed late blessing does not clear both states/throttle/remove tracked843'

inner = instructions(previous['callback'])
assert all(any(entry.op_str == expected for entry in inner) for expected in ('xmm0, xmmword ptr [rdi + 0x14]', 'xmm0, xmmword ptr [rdi + 0x24]')), 'Final handoff lost its captured player/Helya pair'
assert all(any(entry.op_str == expected for entry in body[aura:timer]) for expected in ('xmmword ptr [rsp + 0xec], xmm0', 'xmmword ptr [rsp + 0xfc], xmm1')), 'Seven-second closure does not retain original identities'
update = instructions('?UpdateAI@' + actor)
tick = next(index for index, entry in enumerate(update) if entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rcx + 0x100], eax')
assert any(entry.mnemonic == 'cmovbe' for entry in update[:tick]), 'Visual countdown can underflow'
assert tick < sites(update, '?UpdateIntro@' + actor)[0], 'Stationary retry does not tick the throttle first'
for diagnostic in (b'Warswords intro waiting for live same-phase Helya99043', b'Warswords intro waiting for native Helya195062 aura', b'Warswords intro recovered native Helya195062 aura', b'Warswords intro lost its player or Helya before the late blessing'):
    assert diagnostic in native['binary'], 'Visual failure/recovery diagnostic absent'
print('PASS: deployed selected843 starts without early195062; installed24s closure resolves original player/Helya, validates both before/after a normal195062 cast and acknowledges actual Vigfus aura with original Helya GUID. All failure branches exclude the installed7s handoff; cancellation, scoped diagnostics and1s retry retained. Native rendering/interruption and client traversal remain separate.')
