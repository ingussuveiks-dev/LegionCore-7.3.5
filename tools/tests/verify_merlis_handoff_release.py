"""Actual source/corpse updater and acknowledged original Garona handoff."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_garona_reveal_release.py')))
n = p['n']
ins = n['instructions']
method = lambda name: '?' + name + '@npc_merlis_malaganAI@npc_merlis_malagan@@'
receipt = '?GetData@npc_garona_halforcen_outroAI@npc_garona_halforcen_outro@@'
body, lifecycle, context, actor, death, summoned = [ins(method(x)) for x in
    ('UpdateOutro', 'UpdateLifecycle', 'OutroContext', 'OutroActor', 'JustDied', 'JustSummoned')]
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}

def normalized(rows):
    result = []
    for op in rows:
        operands = []
        for operand in op.operands:
            if operand.type == CS_OP_IMM:
                value = operand.imm
                operands.append(('local', value - rows[0].address) if rows[0].address <= value <= rows[-1].address else ('imm', value))
            elif operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                operands.append(('rip', op.address + op.size + operand.mem.disp))
            elif operand.type == CS_OP_REG:
                operands.append(('reg', op.reg_name(operand.reg)))
            else:
                operands.append(('other', op.op_str))
        result.append((op.mnemonic, operands))
    return result

# MSVC inlines the exact retry body into the lifecycle override. Compare its
# symbolic instructions, including all branches, rather than trusting a tag.
assert normalized(body) == normalized(lifecycle)
assert {107831, 1620} <= values(context)
for prefix in ('?IsInWorld@Object@@', '?IsPreDelete@Object@@', '?IsQuestObjectExpired@WorldObject@@', '??8ObjectGuid@@'):
    assert sites(n, context, prefix)
assert ('cmp', 'dword ptr [rcx + 0x3b8], eax') in pairs(context), 'Original instance identity guard absent'
assert {108222} <= values(actor) and len(sites(n, actor, '?CanTargetQuestObject@WorldObject@@')) == 2
assert sites(n, actor, '?InSamePhase@WorldObject@@')
assert ('cmp', 'qword ptr [rbx + 8], rcx') in pairs(actor), 'Source and target map identity differs'
assert len(sites(n, body, method('OutroContext'))) == 3
assert len(sites(n, body, '?GetCreature@ObjectAccessor@@')) == 4
assert ('mov', 'dword ptr [rdi + 0x144], 0x7d0') in pairs(body), 'Missing bounded2s retry'

def vtable(name, size=82):
    text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
    value, = re.findall(r'\s+\w+:\w+\s+' + re.escape(name) + r'\s+([0-9a-fA-F]{16})', text)
    return struct.unpack_from('<' + str(size) + 'Q', n['binary'], n['offset'](int(value, 16)))

garona = vtable('??_7npc_garona_halforcen_outroAI@npc_garona_halforcen_outro@@6B@')
get_slot, = [i for i, target in enumerate(garona) if n['resolve'](target) == n['symbol'](receipt)]
check, = [i for i, op in enumerate(body) if op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == get_slot * 8]
assert body[check - 1].op_str == 'edx, 0x1a6be'
assert body[check + 1].op_str == 'eax, eax' and body[check + 2].mnemonic == 'je'
cleanup, = sites(n, body, '?DespawnOrUnsummon@Creature@@')
assert cleanup > check and body[cleanup].address not in reachable(body, body[check + 2].operands[0].imm)
assert ('mov', 'byte ptr [rdi + 0x14c], 0') in pairs(body[check:cleanup])
assert ('mov', 'byte ptr [rdi + 0x14e], 1') in pairs(body[check:cleanup])
assert sites(n, body[check:cleanup], '??9ObjectGuid@@') and sites(n, body[check:cleanup], method('OutroActor'))
assert {('cmp', 'edx, 0x1a6be'), ('cmp', 'byte ptr [rcx + 0x38], 0'), ('mov', 'eax, 1'), ('xor', 'eax, eax')} <= pairs(ins(receipt))
cancel_calls = sites(n, body, method('CancelOutro'))
assert cancel_calls
for at in sites(n, body, method('OutroContext')):
    assert body[at + 1].op_str == 'al, al' and body[at + 2].mnemonic in ('je', 'jne')
    branch = body[at + 2]
    rejected = branch.operands[0].imm if branch.mnemonic == 'je' else body[at + 3].address
    path = reachable(body, rejected)
    assert any(body[i].address in path for i in cancel_calls), 'Rejected context must reach owned cancellation'
    assert body[cleanup].address not in path, 'Rejected context must retain original actor'
cancel = ins(method('CancelOutro'))
assert sites(n, cancel, '?FindMap@MapManager@@') and sites(n, cancel, '??8ObjectGuid@@')
assert {1620, 108222} <= values(cancel)
assert {('test', 'byte ptr [rbx + 0x30e8], 1'), ('movups', 'xmm0, xmmword ptr [rbx + 0x3b64]'), ('lea', 'rdx, [rdi + 0x131]')} <= pairs(cancel), 'Cancellation must compare the actual TempSummon full summoner GUID'
cancel_check, = [i for i, op in enumerate(cancel) if op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == get_slot * 8]
cancel_remove, = sites(n, cancel, '?DespawnOrUnsummon@Creature@@')
assert cancel[cancel_check + 1].op_str == 'eax, eax'
branch = cancel[cancel_check + 2]
assert branch.mnemonic in ('je', 'jne')
accepted = branch.operands[0].imm if branch.mnemonic == 'jne' else cancel[cancel_check + 3].address
assert cancel[cancel_remove].address not in reachable(cancel, accepted), 'Cancellation must retain an already accepted reveal'
assert any(op.mnemonic == 'mov' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == 0x14e and op.operands[1].type == CS_OP_IMM and op.operands[1].imm == 1 for op in cancel)
assert n['symbol'](method('UpdateOutro')) in n['calls'](method('JustDied'))
owner, = sites(n, death, '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
drain, = sites(n, death, '?DespawnAll@SummonList@@')
handoff, = sites(n, death, method('UpdateOutro'))
credit, = sites(n, death, '?UpdateAchievementCriteria@Player@@')
assert owner < drain < handoff < credit
entry_check, = [i for i, op in enumerate(summoned) if (op.mnemonic, op.op_str) == ('cmp', 'eax, 0x1a6be')]
summon, = sites(n, summoned, '?Summon@SummonList@@')
combat, = sites(n, summoned, '?DoZoneInCombat@CreatureAI@@')
assert entry_check < summon < combat
assert summoned[entry_check + 1].mnemonic == 'je' and summoned[entry_check + 1].operands[0].imm > summoned[combat].address
merlis = vtable('??_7npc_merlis_malaganAI@npc_merlis_malagan@@6B@')
slot, = [i for i, target in enumerate(merlis) if n['resolve'](target) == n['symbol'](method('UpdateLifecycle'))]
assert any(op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == slot * 8 for op in ins('?Update@Creature@@'))
for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
    assert not sites(n, body, forbidden)
print('PASS selected Merlis source/corpse lifecycle: actual complete retry body/core virtual dispatch, original GUID/instance/dead/world/phase/scope gates,2s retry, same full-GUID actor and real Garona receipt before old cleanup, owned unaccepted-candidate cancellation preserving accepted reveal, friendly outro excluded from combat drain; original owner credit order retained and no retry progression/item grants')
