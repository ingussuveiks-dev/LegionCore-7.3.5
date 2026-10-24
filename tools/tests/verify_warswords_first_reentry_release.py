"""Check the selected first Vigfus re-entry, combat, damage and refresh policies."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_visibility_release.py')))
native = previous['native']
instructions = previous['instructions']
actor = '@npc_vigfus_bladewindAI@npc_vigfus_bladewind@@'
active = '?IsEncounterActive' + actor
selection = '?CanAIAttack' + actor
attack = '?AttackStart' + actor
update = '?UpdateAI' + actor
post = '?DamageTakenAfterScaling' + actor
pre = '?DamageTaken' + actor
reset = '?Reset' + actor


def call_sites(body, prefix):
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


body = instructions(active)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {98007, 1511, 944, 4, 5} <= values, 'First actor admission lost native scope'
for dependency in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](active), 'First admission does not use live scenario state'
four = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'eax, 4')
five = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'eax, 5')
phase = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rbx + 0xc4], 0')
assert four < five < phase and body[four + 1].mnemonic == 'je' and body[five + 1].mnemonic == 'jne' and body[phase + 1].mnemonic == 'je', 'First visibility no longer allows4 or genuine retreat at5'
assert body[five + 1].operands[0].imm == body[phase + 1].operands[0].imm, 'Fresh5 and wrong stage do not reject together'
rejection = next(index for index, entry in enumerate(body) if entry.address == body[phase + 1].operands[0].imm)
assert any(entry.mnemonic == 'xor' and entry.op_str == 'al, al' for entry in body[rejection:rejection + 3]), 'Rejected stage remains visible'
success = next(index for index, entry in enumerate(body) if entry.address == body[four + 1].operands[0].imm)
assert any(entry.op_str == 'al, 1' for entry in body[success:success + 3]), 'Stage4 is incorrectly hidden'

for consumer in (selection, attack, update):
    decoded = instructions(consumer)
    admission = call_sites(decoded, active)[0]
    assert any(entry.mnemonic == 'cmp' and entry.op_str == 'eax, 0x17ed7' for entry in decoded[:admission]), 'Unrelated entry admission changed'
    assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rax + 0xe0], 0x5e7' for entry in decoded[:admission]), 'Unrelated map admission changed'
    phase = next(index for index, entry in enumerate(decoded[:admission]) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rbx + 0xc4], 0')
    assert decoded[phase + 1].mnemonic == 'jne' and decoded[admission + 1].op_str == 'al, al', 'Retreating or inactive first actor can engage'
    if consumer == selection:
        target = next(index for index, entry in enumerate(decoded) if entry.address == decoded[phase + 1].operands[0].imm)
        assert decoded[target].op_str == 'al, al' and decoded[target].mnemonic == 'xor' and decoded[admission + 2].mnemonic == 'jne', 'Native selection does not return false'
    else:
        dependency = '?AttackStart@ScriptedAI@@' if consumer == attack else '?UpdateSpectreRush' + actor
        execution = call_sites(decoded, dependency)[0]
        assert admission < execution and decoded[admission + 2].mnemonic == 'je' and decoded[admission + 2].operands[0].imm == decoded[phase + 1].operands[0].imm > decoded[execution].address, 'Rejected first actor still executes attack/update'
        if consumer == update:
            victim = call_sites(decoded, '?UpdateVictim@CreatureAI@@')[0]
            assert execution < victim and decoded[admission + 2].operands[0].imm > decoded[victim].address, 'Inactive first actor still selects victims/evades'

damage = instructions(post)
admission = call_sites(damage, active)[0]
assert damage[admission + 1].op_str == 'al, al' and damage[admission + 2].mnemonic == 'je', 'Cached damage does not reject inactive first actor'
zero = next(index for index, entry in enumerate(damage) if entry.address == damage[admission + 2].operands[0].imm)
assert damage[zero].op_str == 'dword ptr [rsi], 0' and any(entry.op_str == 'rsi, r8' for entry in damage[:admission]), 'Inactive damage does not zero actual final reference'
assert all(entry.mnemonic in ('mov', 'add', 'pop', 'ret') for entry in damage[zero + 1:]), 'Rejected damage continues into progression'
before = instructions(pre)
admission = call_sites(before, active)[0]
pending = next(index for index, entry in enumerate(before) if entry.op_str == 'byte ptr [rdi + 0xc5], 0' and entry.mnemonic == 'cmp')
assert admission < pending and before[admission + 2].mnemonic == 'je' and before[admission + 2].operands[0].imm > before[pending].address, 'Stale pending flag can start departure outside active stage'

factory = instructions('?GetAI@npc_vigfus_bladewind@@')
assert any(entry.op_str == 'ebp, ebp' and entry.mnemonic == 'xor' for entry in factory) and any(entry.op_str == 'byte ptr [rsi + 0xc4], bpl' and entry.mnemonic == 'mov' for entry in factory), 'Retreat state is uninitialized before Reset/corpse visibility'
tables = []
for entry in factory:
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = struct.unpack_from('<128Q', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))
            if table[0xa8 // 8] == native['symbol'](post):
                tables.append(table)
assert len(tables) == 1, 'First factory does not select guarded post-scaled damage'
for slot, prefix in ((8, selection), (0x10, attack), (0x118, '?CanBeVisibleFor' + actor)):
    assert native['resolve'](tables[0][slot // 8]) == native['symbol'](prefix), 'Selected first actor callback missing: ' + prefix
assert native['symbol']('?CanBeVisibleFor' + actor) == native['symbol'](active) or native['symbol'](active) in native['calls']('?CanBeVisibleFor' + actor), 'Visibility does not consume the real active-stage policy'
assert native['symbol'](update) in tables[0] and native['symbol'](reset) in tables[0], 'Selected first actor lacks guarded update/reset'

decoded = instructions(reset)
capture = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'movzx' and entry.op_str == 'ebx, byte ptr [rcx + 0xc4]')
clear = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'mov' and entry.op_str == 'word ptr [rcx + 0xc4], 0')
refresh = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0xf0]')
assert capture < clear < refresh and decoded[refresh - 1].op_str == 'dl, 1', 'Reset does not force visibility after clearing old retreat'
for condition, branch in (('bl, bl', 'je'), ('eax, 0x17ed7', 'jne'), ('dword ptr [rcx + 0xe0], 0x5e7', 'jne')):
    position = next(index for index, entry in enumerate(decoded[:refresh]) if entry.op_str == condition and entry.mnemonic in ('cmp', 'test'))
    assert decoded[position + 1].mnemonic == branch and decoded[position + 1].operands[0].imm > decoded[refresh].address, 'Unrelated/nonretreat Reset can force first-actor refresh'

transition = instructions(previous['step'])
assert all(any(entry.op_str == expected for entry in transition) for expected in ('dword ptr [rsp + 0x20], 0x17ed7', 'dword ptr [rsp + 0x24], 0x1812a', 'rbx, 4', 'rax, [rsp + 0x28]')), 'Stage refresh does not iterate both first/final identities'
backedge = next(index for index, entry in enumerate(transition) if entry.mnemonic == 'cmp' and entry.op_str == 'rbx, rax')
assert transition[backedge + 1].mnemonic == 'jne' and transition[backedge + 1].operands[0].imm < transition[previous['refresh']].address, 'Two-actor refresh loop does not repeat the map lookup'
print('PASS: selected first98007 initializes retreat state and uses live1511/944/4 or genuine5 retreat for visibility; native selection/direct attack/update reject inactive or retreating combat, final scaled damage zeros cached/owner/pet hits and stale departure is gated. Reset refreshes after clearing retreat; instance retains exact first/final GUIDs and refreshes both before bootstrap. Native client visibility/pathing, persisted traversal and full36 remain unverified.')
