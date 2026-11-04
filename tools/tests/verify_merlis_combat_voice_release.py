"""Selected registered Merlis combat dialogue, plus retained native handoff."""
from pathlib import Path
import runpy
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_merlis_handoff_release.py')))
n = p['n']
method = lambda name: '?' + name + '@npc_merlis_malaganAI@npc_merlis_malagan@@'
voice, update, reset, enter = [n['instructions'](method(x)) for x in
                              ('UpdateCombatVoice', 'UpdateAI', 'Reset', 'EnterCombat')]
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
assert {2442, 2448, 107831, 1620, 1000} <= values(voice)
for prefix in ('?IsInWorld@Object@@', '?IsPreDelete@Object@@',
               '?isInCombat@Unit@@', '?IsQuestObjectExpired@WorldObject@@'):
    assert sites(n, voice, prefix)
create, = sites(n, voice, '?CreateConversation@Conversation@@')
assert voice[create + 1].op_str == 'al, al' and voice[create + 2].mnemonic == 'jne'
success = voice[create + 2].operands[0].imm
refused = reachable(voice, voice[create + 3].address)
accepted = reachable(voice, success)
advance, = [op for op in voice if (op.mnemonic, op.op_str) == ('inc', 'dword ptr [r12]')]
assert advance.address in accepted and advance.address not in refused
assert {('cmp', 'r13d, dword ptr [r14 + 0x158]'),
        ('mov', 'eax, dword ptr [r15 + 0x4dc]'),
        ('mov', 'dword ptr [r14 + 0x154], eax'),
        ('mov', 'byte ptr [r14 + 0x15c], 0')} <= pairs([op for op in voice if op.address in accepted])
assert ('mov', 'dword ptr [r14 + 0x154], 0x3e8') in pairs([op for op in voice if op.address in refused])
assert {('lea', 'rbx, [rcx + 0x15d]'), ('cmp', 'byte ptr [rbx], 0'),
        ('mov', 'byte ptr [rbx], 1'), ('mov', 'byte ptr [rbx], 0')} <= pairs(voice)
calls = sites(n, update, method('UpdateCombatVoice'))
summons = sites(n, update, '?SummonCreature@WorldObject@@')
assert len(calls) == 3 and len(summons) == 6
assert calls[0] < summons[0] < summons[-1] < calls[1] < calls[2]
assert not any(summons[0] <= at <= summons[-1] for at in calls), 'Dialogue request remains inside unrolled six-add wave'
assert sites(n, update, method('UpdateIntro'))[0] < calls[0]
for rows in (reset, enter):
    assert any(op.mnemonic == 'inc' and op.op_str.endswith('+ 0x158]') for op in rows)
    assert any(op.mnemonic == 'mov' and op.op_str.startswith('dword ptr [') and '+ 0x154]' in op.op_str for op in rows)
    assert any(op.mnemonic == 'mov' and '+ 0x15c]' in op.op_str for op in rows)
assert {2442, 5000, 25000} <= values(enter)
assert n['symbol'](method('UpdateAI')) in [n['resolve'](at) for at in p['merlis']]
for prefix in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
    assert not sites(n, voice, prefix)
print('PASS selected registered Merlis combat: original six summons before one wave request, independent tick/ability requests, real CreateConversation acceptance before line advance, admitted native duration -> delay, bounded same-line failure retry, generation/reset/reentry and source/combat guards; no dialogue progression/item grants')
