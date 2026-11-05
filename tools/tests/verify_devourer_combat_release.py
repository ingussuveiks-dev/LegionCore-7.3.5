"""Selected final Akaari combat voice; original abilities and retained death/entry."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_devourer_death_voice_release.py')))
n = p['n']
method = lambda name: '?' + name + '@npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@'
voice, update, reset, enter = [p['ins'](method(x)) for x in
                              ('UpdateCombatVoice', 'UpdateAI', 'Reset', 'EnterCombat')]
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
assert {1943, 1947, 105660, 1607, 1000} <= values(voice)
for prefix in ('?IsInWorld@Object@@', '?IsPreDelete@Object@@',
               '?isInCombat@Unit@@', '?IsQuestObjectExpired@WorldObject@@'):
    assert sites(n, voice, prefix)
create, = sites(n, voice, '?CreateConversation@Conversation@@')
assert voice[create + 1].op_str == 'al, al' and voice[create + 2].mnemonic == 'jne'
success = voice[create + 2].operands[0].imm
refused = reachable(voice, voice[create + 3].address)
accepted = reachable(voice, success)
advance, = [op for op in voice if (op.mnemonic, op.op_str) == ('inc', 'dword ptr [r15]')]
assert advance.address in accepted and advance.address not in refused
assert {('cmp', 'r12d, dword ptr [rsi + 0x98]'),
        ('mov', 'eax, dword ptr [r14 + 0x4dc]'),
        ('mov', 'dword ptr [rsi + 0x94], eax'),
        ('mov', 'byte ptr [rsi + 0x9c], 0')} <= pairs([op for op in voice if op.address in accepted])
assert ('mov', 'dword ptr [rsi + 0x94], 0x3e8') in pairs([op for op in voice if op.address in refused])
assert {('lea', 'rbx, [rcx + 0x9d]'), ('cmp', 'byte ptr [rbx], 0'),
        ('mov', 'byte ptr [rbx], 1'), ('mov', 'byte ptr [rbx], 0')} <= pairs(voice)
# Selected field layout and control-flow proofs below are pinned to the
# final compiler output, independently of the source boundary certificate.
reset_voice=p['ins'](method('ResetCombatVoice'))
for rows in (reset,enter):assert sites(n,rows,method('ResetCombatVoice'))
assert {('inc','dword ptr [rcx + 0x98]'),('mov','dword ptr [rbx + 0x94], 0'),
        ('mov','byte ptr [rbx + 0x9c], 0')} <= pairs(reset_voice)
assert {1942,41924,42000,11000,14000} <= values(enter)
calls = sites(n, update, method('UpdateCombatVoice'))
assert len(calls) == 2
assert sites(n, update, '?UpdateVictim@CreatureAI@@')[0] < calls[0]
clock, = [i for i, op in enumerate(update) if (op.mnemonic, op.op_str) == ('add', 'dword ptr [rbx + 0x48], esi')]
assert calls[0] < clock < calls[-1]
assert {206866, 224990, 209233, 42000, 11000, 14000} <= values(update)
assert {('sub', 'eax, 0x796'), ('cmp', 'eax, 4'), ('setbe', 'al'),
        ('mov', 'byte ptr [rbx + 0x9c], al'), ('xor', 'edx, edx')} <= pairs(update[:calls[-1]])
for prefix in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
    assert not sites(n, voice, prefix)
print('PASS selected registered final Akaari combat: original EVENT3 requests1943..1946 after spell209233, successful real factory before cursor advance, independent tick, full native duration, bounded same-line failure retry/reset/generation/reentry and source guards; original abilities/timers/death/entry retained; opening1942 independently verified, no dialogue quest/item grant. Native/client scope separate.')
