"""Check the linked Magnar scaled-lethal policy and actual selected recipient."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
post = '?DamageTakenAfterScaling' + suffix
eligible = '?IsFinalePlayer' + suffix
recipient = '?GetFinalePlayer' + suffix


def positions(body, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


body = instructions(post)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {96034, 1495, 128749, 504, 92, 45784, 6000} <= values, 'Native boss scope/dialogue/event/delay changed'
health = positions(body, '?GetHealth@Unit@@QEBA_KPEAV1@@')[0]
assert body[health - 1].op_str == 'rdx, rbp', 'Health comparison lost actual attacker context'
assert body[health + 1].op_str == 'rbx, rax' and body[health + 2].mnemonic == 'jb', 'Nonlethal damage must remain unchanged'
assert body[health + 3].op_str == 'ebx, ebx' and body[health + 3].mnemonic == 'xor'
assert body[health + 4].op_str == 'dword ptr [rsi], ebx', 'Lethal damage is not zeroed before side effects'
assert body[health + 5].op_str == 'byte ptr [rdi + 0xd1], bl' and body[health + 6].mnemonic == 'jne', 'First phase can finish early'
no_effects = body[health + 2].operands[0].imm
assert body[health + 6].operands[0].imm == no_effects
selection = positions(body, recipient)[0]
assert body[selection + 1].op_str == 'rsi, rax' and body[selection + 2].op_str == 'rax, rax'
assert body[selection + 3].mnemonic == 'je' and body[selection + 3].operands[0].imm == no_effects, 'Missing recipient must remain retryable'
claim = next(entry for entry in body if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rdi + 0xd2], 1')
conversation = positions(body, '?CreateConversation@Unit@@')[0]
credit = positions(body, '?UpdateAchievementCriteria@Player@@')[0]
assert claim.address < body[conversation].address < body[credit].address
assert body[credit - 1].op_str == 'rcx, rsi', 'Credit no longer uses selected eligible player'
pending = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0xd2], 0')
assert body[pending + 2].mnemonic == 'je' and body[pending + 2].operands[0].imm == body[health - 2].address
assert body[pending + 4].op_str == 'dword ptr [rsi], ebx' and body[pending + 5].operands[0].imm == no_effects, 'Repeated lethal hit can replay finale'
assert not any(native['symbol'](dependency) in native['calls'](post) for dependency in ('?AddItem@Player@@', '?Kill@Unit@@'))

selection_body = instructions(recipient)
owner = positions(selection_body, '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')[0]
owner_check = positions(selection_body, eligible)[0]
assert selection_body[owner - 3].op_str == 'rdx, rdx' and selection_body[owner - 2].mnemonic == 'je'
assert selection_body[owner - 2].operands[0].imm > selection_body[owner].address, 'Null attacker reaches owner resolver'
assert selection_body[owner_check - 2].op_str == 'rdx, rbx', 'Eligibility is not checked on resolved owner'
assert any(entry.mnemonic == 'ret' for entry in selection_body[owner_check + 1:owner_check + 9]), 'Eligible owner no longer precedes fallback'
assert any(entry.op_str == 'rax, qword ptr [rax + 0x768]' for entry in selection_body), 'Fallback does not enumerate actual Map players'
assert len(positions(selection_body, '?GetDistance@WorldObject@@QEBAMPEBV1@@')) == 2
assert any(entry.mnemonic == 'comiss' and entry.op_str == 'xmm0, xmm6' and selection_body[index + 1].mnemonic == 'jbe'
           for index, entry in enumerate(selection_body[:-1])), 'Fallback no longer selects nearest eligible participant'

for gate_body, player_register in ((instructions(eligible), 'rbx'), (selection_body, 'rsi')):
    rejection = None
    for dependency, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'),
                               ('?InSamePhase@WorldObject@@', 'je'), ('?IsWithinDistInMap@WorldObject@@', 'je'),
                               ('?GetQuestRewardStatus@Player@@', 'jne')):
        position = positions(gate_body, dependency)[0]
        assert gate_body[position + 1].op_str == 'al, al' and gate_body[position + 2].mnemonic == branch
        destination = gate_body[position + 2].operands[0].imm
        rejection = rejection or destination
        assert destination == rejection, 'Eligibility rejection paths differ'
    for operand, branch in ((f'qword ptr [{player_register} + 8], rax', 'jne'),
                            (f'dword ptr [{player_register} + 0x3b8], eax', 'jne'),
                            ('al, 1', 'jne'), ('eax, 1', 'jne'), ('eax, 0x38d', 'jne'), ('eax, 4', 'jne')):
        assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and gate_body[index + 1].mnemonic == branch
                   and gate_body[index + 1].operands[0].imm == rejection for index, entry in enumerate(gate_body[:-1])), f'Missing gate: {operand}'
    assert len(positions(gate_body, '?GetQuestStatus@Player@@')) == 2
    assert sum(entry.mnemonic == 'mov' and entry.op_str == 'edx, 0x9917' for entry in gate_body) == 3
    ranges = [entry for entry in gate_body if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM
              and entry.operands[1].mem.base == X86_REG_RIP]
    assert any(struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))[0] == 100.0 for entry in ranges)

tables = []
for entry in instructions('?GetAI@npc_magnar_icebreaker@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0xa8)[0] == native['symbol'](post):
                tables.append(offset)
assert len(tables) == 1, 'Actual Magnar factory omits post-scaling policy'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol']('?DamageTaken@UnitAI@@'), 'Old pre-scaling override remains installed'
assert any(entry.op_str == 'byte ptr [rbx + 0xd2], 0' for entry in instructions('?Reset' + suffix)), 'Reset does not clear scoped finale claim'
damage = instructions('?DealDamage@Unit@@')
division = next(index for index, entry in enumerate(damage) if entry.mnemonic == 'divss' and entry.op_str == 'xmm1, xmm0')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x180 for entry in damage[division - 8:division])
assert damage[division + 1].mnemonic == 'cvttss2si' and damage[division + 2].op_str == 'dword ptr [rsp + 0x60], ecx'
assert sum(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xa8 for entry in damage[division + 3:division + 70]) == 2
assert b'npc_magnar_icebreaker' in native['binary']
print('PASS: canonical Magnar factory selects post-scaling protection, attacker-relative lethal comparison, first-phase/once guards and eligible owner-first/native Map fallback; original504/45784/6000 and both scaled AI dispatches retained. Not full waves, group criteria or client combat/VFX.')
