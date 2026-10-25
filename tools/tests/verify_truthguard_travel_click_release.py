"""Verify the installed guarded Truthguard travel click and native empty-row dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_credit_cast_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_sr_argent_hippogryphAI@npc_sr_argent_hippogryph@@'
helper = '?SummonForPlayer' + suffix
callback = '?OnSpellClick' + suffix
body = instructions(helper)
cast_target = native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
casts = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
         and native['resolve'](entry.operands[0].imm) == cast_target]
assert len(casts) == 1
cast = casts[0]
assert body[cast - 1].op_str == 'rcx, rdx' and body[cast - 2].op_str == 'r9b, 1', 'Not a native triggered player-self cast'
assert body[cast + 1].op_str == 'esi, eax' and body[cast + 2].op_str == 'eax, eax'
assert body[cast + 3].mnemonic == 'je', 'Native zero/OK result does not select success'
accepted = body[cast + 3].operands[0].imm
destroy = [entry for entry in body if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x38]']
assert len(destroy) == 1
accepted_index = next(index for index, entry in enumerate(body) if entry.address == accepted)
assert [entry.op_str for entry in body[accepted_index:accepted_index + 4]] == [
    'rcx, qword ptr [rdi + 0x20]', 'rax, qword ptr [rcx]', 'rdx, rbx', 'qword ptr [rax + 0x38]']


def reachable(start):
    by_address = {entry.address: entry for entry in body}
    pending = [start]
    seen = set()
    while pending:
        address = pending.pop()
        if address in seen:
            continue
        assert address in by_address, 'Helper branches outside checked runtime bounds'
        seen.add(address)
        entry = by_address[address]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            assert entry.operands[0].type == CS_OP_IMM
            pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return seen


assert destroy[0].address in reachable(accepted)
assert destroy[0].address not in reachable(body[cast + 4].address), 'Rejected cast can still hide the source'
assert b'Truthguard travel: summon cast %u rejected with result %u' in native['binary']
click = instructions(callback)
helper_calls = [index for index, entry in enumerate(click) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol'](helper)]
assert len(helper_calls) == 2
for index, spell in zip(helper_calls, (209884, 210358)):
    assert click[index - 3].op_str == f'r8d, {hex(spell)}'
    assert click[index - 2].op_str == 'rdx, rbx' and click[index - 1].op_str == 'rcx, rdi'
for operand in ('byte ptr [rdx + 0x16], 4', 'eax, 0x19d9b', 'edx, 0xa421', 'eax, 3', 'eax, 0x19edb', 'edx, 6', 'eax, 7'):
    assert any(entry.op_str == operand for entry in click), f'Original live player/entry/quest/event gate missing: {operand}'
assert cast_target not in native['calls'](callback), 'A direct duplicate summon cast remains outside the helper'

tables = []
for entry in instructions('?GetAI@npc_sr_argent_hippogryph@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<80Q', native['binary'], table)
            if native['symbol'](callback) in slots:
                tables.append(slots)
assert len(tables) == 1 and tables[0][0x218 // 8] == native['symbol'](callback), 'Factory does not select the repaired OnSpellClick'
dispatch = instructions('?HandleSpellClick@Unit@@')
ai_call = next(entry for entry in dispatch if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x218]')
empty = next(index for index, entry in enumerate(dispatch) if entry.op_str == 'rbx, qword ptr [rsp + 0xc8]' and entry.mnemonic == 'cmp')
assert dispatch[empty + 1].mnemonic == 'je'
assert dispatch[empty + 1].operands[0].imm < ai_call.address
empty_target = next(index for index, entry in enumerate(dispatch) if entry.address == dispatch[empty + 1].operands[0].imm)
segment = [entry for entry in dispatch[empty_target:] if entry.address < ai_call.address]
assert not any(entry.mnemonic == 'ret' or entry.mnemonic == 'jmp' for entry in segment), 'Empty rows bypass the AI'
assert any(entry.op_str == 'sil, sil' and entry.mnemonic == 'test' and entry.address > ai_call.address for entry in dispatch), 'Native result is consulted before AI'
visibility = instructions('?canSeeSpellClickOn@Player@@')
empty = next(index for index, entry in enumerate(visibility) if entry.op_str == 'rbx, qword ptr [rsp + 0x38]' and entry.mnemonic == 'cmp')
assert visibility[empty + 1].mnemonic == 'je'
target = next(entry for entry in visibility if entry.address == visibility[empty + 1].operands[0].imm)
assert target.op_str == 'al, 1', 'Explicit static spellclick flag is hidden without DB rows'
assert any(entry.op_str == 'r8d, 0x1000000' for entry in visibility[:empty]), 'Native visibility no longer checks the actual flag'
assert native['symbol']('?HandleSpellClick@Unit@@') in native['calls']('?HandleSpellClick@WorldSession@@'), 'Packet handler bypasses native AI dispatch'
print('PASS: canonical selected hippogryph AI requests each original summon once through the native cast-result guard; nonzero paths cannot reach source hide. Native empty-row visibility and packet-to-AI dispatch retained. Accepted spawn/boarding/flight and client checks remain separate.')
