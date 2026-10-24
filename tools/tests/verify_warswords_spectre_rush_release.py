"""Verify a selected Vigfus two-spectre retry/death lifecycle in the deployed PE."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_intro_release.py')))
native = previous['native']
instructions = previous['instructions']
first = globals().get('first_spectre_rush', False)
actor = '@npc_vigfus_bladewindAI@npc_vigfus_bladewind@@' if first else '@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
factory = '?GetAI@npc_vigfus_bladewind@@' if first else '?GetAI@npc_vigfus_bladewind_last@@'
active, defeated, slot, timer, generation = (0xe9, 0xe7, 0xec, 0xf0, 0xf4) if first else (0xf2, 0xf0, 0xf4, 0xf8, 0xfc)


def callback(name):
    return '?' + name + actor


def call_indexes(prefix, dependency):
    return [index for index, entry in enumerate(instructions(prefix))
            if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](dependency)]


update = callback('UpdateAI')
retry = callback('UpdateSpectreRush')
cancel = callback('CancelSpectreRush')
death = callback('SummonedCreatureDies')
spawn = callback('JustSummoned')
despawn = callback('SummonedCreatureDespawn')
assert call_indexes(update, retry)[0] < call_indexes(update, '?UpdateVictim@CreatureAI@@')[0], 'No-victim gate blocks failed-spawn recovery'
assert native['symbol']('?GetCreature@ObjectAccessor@@') in native['calls'](retry), 'Retry does not resolve actual tracked GUIDs on the actor map'
assert native['symbol']('?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMM') in native['calls'](retry), 'Retry no longer creates real spectres'
retry_body = instructions(retry)
for text in (f'dword ptr [rdi + {timer:#x}], 0x3e8', f'dword ptr [rdi + {slot:#x}], 2'):
    assert any(entry.mnemonic == 'mov' and entry.op_str == text for entry in retry_body), f'Missing bounded retry/slot restoration: {text}'
assert any(entry.mnemonic == 'cmp' and entry.op_str == f'r15d, dword ptr [rdi + {generation:#x}]'
           and retry_body[index + 1].mnemonic == 'jne' for index, entry in enumerate(retry_body[:-1])), 'Reset during creation can continue the old batch'
assert any(entry.mnemonic == 'inc' and entry.op_str == f'dword ptr [rcx + {generation:#x}]' for entry in instructions(cancel)), 'Cancellation does not invalidate pending work'
assert any(entry.mnemonic == 'mov' and entry.op_str == f'byte ptr [rcx + {active:#x}], 0' for entry in instructions(cancel)), 'Cancellation leaves retry active'
for name in (('Reset', 'JustDied', 'DamageTakenAfterScaling') if first else ('Reset', 'JustDied')):
    prefix = callback(name)
    assert call_indexes(prefix, cancel)[0] < call_indexes(prefix, '?DespawnAll@SummonList@@')[0], 'Despawn callbacks run before generation invalidation'
assert native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@') in native['calls'](callback('Reset')), 'Reset retains the blocking Spectre Rush aura'

body = instructions(death)
equality = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol']('??8ObjectGuid@@'))
claim = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == f'byte ptr [r14 + {defeated:#x}], 1')
count = next(index for index, entry in enumerate(body) if entry.mnemonic == 'inc' and entry.op_str == 'dword ptr [rdi + 0xc0]')
release = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == f'byte ptr [rdi + {active:#x}], 0')
assert equality < claim < count < release, 'Death count precedes tracked GUID/once-only claim'
assert body[count + 1].mnemonic == 'cmp' and body[count + 1].op_str == 'dword ptr [rdi + 0xc0], 2' and body[count + 2].mnemonic == 'jne', 'Immunity releases before two distinct deaths'
assert any(entry.mnemonic == 'cmp' and entry.op_str == f'byte ptr [r14 + {defeated:#x}], 0' and body[index + 1].mnemonic == 'jne'
           for index, entry in enumerate(body[:claim])), 'Previously defeated GUID can count again'
for dependency in ('?RemoveFlag@Object@@', '?SetReactState@Creature@@', '?RemoveAura@Unit@@QEAAXIVObjectGuid@@'):
    assert native['symbol'](dependency) in native['calls'](death), f'Completed wave cannot restore combat: {dependency}'
spawn_body = instructions(spawn)
guid = call_indexes(spawn, '?GetGuidValue@Object@@')[0]
visual = call_indexes(spawn, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags@@')[0]
storage = 'xmmword ptr [rdi + rax*8 + 0xc7], xmm0' if first else 'xmmword ptr [rdi + rax*8], xmm0'
assert guid < visual and any(entry.mnemonic == 'movups' and entry.op_str == storage for entry in spawn_body[guid:visual]), 'Synchronous visual callbacks precede GUID registration'
assert native['symbol']('??8ObjectGuid@@') in native['calls'](despawn), 'Despawn can erase an unrelated or completed slot'

tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<128Q', native['binary'], offset)
            if all(native['symbol'](prefix) in slots for prefix in (update, spawn, death, despawn, callback('Reset'), callback('JustDied'))):
                tables.append(slots)
assert len(tables) == 1, 'Final factory does not select all repaired lifecycle overrides'
for prefix, dispatcher in ((spawn, '?InitSummon@TempSummon@@'), (death, '?Kill@Unit@@'), (despawn, '?UnSummon@TempSummon@@')):
    slot = tables[0].index(native['symbol'](prefix)) * 8
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot
               for entry in instructions(dispatcher)), f'Native lifecycle dispatcher misses selected callback: {prefix}'
for prefix, expected in ((retry, {99609, 1000, 2}), (update, {195996, 51000}), (spawn, {197852}), (death, {195996, 99609})):
    literals = {operand.imm for entry in instructions(prefix) for operand in entry.operands if operand.type == CS_OP_IMM}
    assert expected <= literals, f'Original spectre identities/count/timing changed: {prefix}'
if first:
    for prefix in (retry, update):
        decoded = instructions(prefix)
        phase_check = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'cmp'
                           and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xc4)
        branch = phase_check + 1
        while decoded[branch].mnemonic in ('mov', 'lea', 'nop'):
            branch += 1
        assert decoded[branch].mnemonic == 'jne', 'Retreat can spawn a new Spectre Rush'
        next_action = call_indexes(prefix, '?SummonCreature@WorldObject@@QEAAPEAVTempSummon@@IMMMM')[0] if prefix == retry else call_indexes(prefix, cancel)[0]
        assert decoded[branch].operands[0].imm > decoded[next_action].address, 'Retreat branch does not skip wave creation'
    post = callback('DamageTakenAfterScaling')
    decoded = instructions(post)
    phase = next(index for index, entry in enumerate(decoded) if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rbx + 0xc4], 1')
    cancellation = call_indexes(post, cancel)[0]
    cleanup = call_indexes(post, '?DespawnAll@SummonList@@')[0]
    presentation = call_indexes(post, '?Talk@CreatureAI@@QEAAXE')[0]
    credit = call_indexes(post, '?UpdateAchievementCriteria@Player@@')[0]
    assert phase < cancellation < cleanup < presentation < credit, 'Retreat progression precedes claimed phase/current-wave cleanup'
    assert native['symbol']('?RemoveAura@Unit@@QEAAXIVObjectGuid@@') in native['calls'](post), 'Retreat retains the old Spectre Rush aura'
    print('PASS: canonical first98007 selected vtable/native callbacks, GUID/count/retry/epoch lifecycle and retreat-before-credit cancellation, no post-retreat spawning; original half-health owner/second-leg path retained. Not live combat/group/visual proof.')
else:
    print('PASS: canonical final98602 vtable/native summon-death-despawn dispatch, GUID-before-visual registration, two distinct deaths, 1s real-summon recovery before victim gate, reset/death cancellation and original195996/197852/99609/51s retained. First98007 and full client combat/visuals remain separate.')
