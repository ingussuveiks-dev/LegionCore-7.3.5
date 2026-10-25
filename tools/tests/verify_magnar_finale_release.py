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
eligible = '?IsEncounterPlayer' + suffix
recipient = '?GetEncounterPlayer' + suffix
conversation_helper = '?StartFinaleConversation' + suffix
handoff = '?UpdateFinale' + suffix


def positions(body, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


body = instructions(post)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {96034, 1495, 128749, 504} <= values, 'Native boss/conversation scope changed'
health = positions(body, '?GetHealth@Unit@@QEBA_KPEAV1@@')[0]
assert body[health - 1].op_str == 'rdx, r14', 'Health comparison lost actual attacker context'
assert body[health + 1].op_str == 'rbx, rax' and body[health + 2].mnemonic == 'jb', 'Nonlethal damage must remain unchanged'
assert body[health + 3].op_str == 'ebx, ebx' and body[health + 3].mnemonic == 'xor'
assert body[health + 4].op_str == 'dword ptr [rdi], ebx', 'Lethal damage is not zeroed before side effects'
assert body[health + 5].op_str == 'byte ptr [rsi + 0xd1], bl' and body[health + 6].mnemonic == 'jne', 'First phase can finish early'
no_effects = body[health + 2].operands[0].imm
assert body[health + 6].operands[0].imm == no_effects
selection = positions(body, recipient)[0]
assert body[selection - 3].op_str == 'r8d, 4', 'Finale lost its exact native stage4 argument'
assert body[selection + 1].op_str == 'r14, rax' and body[selection + 2].op_str == 'rax, rax'
assert body[selection + 3].mnemonic == 'je' and body[selection + 3].operands[0].imm == no_effects, 'Missing recipient must remain retryable'
claim = next(entry for entry in body if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rsi + 0xd2], 1')
conversation = positions(body, conversation_helper)[0]
data = positions(body, '?GetConversationData@ConversationDataStoreMgr@@QEBAPEBV?$vector@UConversationData@@')[0]
assert body[selection].address < body[data].address < claim.address < body[conversation].address
assert body[data - 2].op_str == 'edx, 0x1f8', 'Timer does not read actual504 line data'
assert any(entry.op_str == 'eax, [rdx + 0x53d]' for entry in body[data:conversation]), 'Native1341..1344 row gate missing'
assert any(entry.op_str == 'edx, 4' for entry in body[data:conversation]), 'Four-line schedule gate missing'
for operand in ('dword ptr [rcx - 4], eax', 'dword ptr [rcx], edx', 'dword ptr [rcx + 8], ebx', 'dword ptr [rcx + 0xc], eax'):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and body[index + 1].mnemonic == 'jne'
               for index, entry in enumerate(body[data:conversation], start=data)), f'Missing native schedule gate {operand}'
assert any(entry.op_str == 'r8d, dword ptr [rsi + 0xec]' for entry in body[selection + 4:conversation]), 'Finale request lost its Reset epoch'
assert body[conversation + 1].op_str == 'al, al' and body[conversation + 2].mnemonic == 'je'
assert body[conversation + 2].operands[0].imm == no_effects, 'Failed conversation still mutates finale flags/credit/despawn'
assert any(entry.op_str == 'byte ptr [rsi + 0xd0], 1' for entry in body[conversation + 3:]), 'Combat stop must follow acknowledged creation'
for dependency in ('?UpdateAchievementCriteria@Player@@', '?DespawnOrUnsummon@Creature@@'):
    assert not positions(body, dependency), 'Lethal-hit callback still advances/removes before dialogue'
for operand in ('xmmword ptr [rsi + 0x100], xmm0', 'dword ptr [rsi + 0x110], edi', 'dword ptr [rsi + 0x114], ecx', 'byte ptr [rsi + 0x118], 1'):
    assert any(entry.mnemonic in ('mov', 'movups') and entry.op_str == operand for entry in body[conversation + 3:]), f'Missing scoped handoff snapshot {operand}'
assert native['symbol']('?CreateConversation@Unit@@') not in native['calls'](post), 'Finale still uses the void creation wrapper'
pending = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rsi + 0xd2], 0')
assert body[pending + 1].mnemonic == 'je' and body[pending + 1].operands[0].imm == body[health - 2].address
assert body[pending + 3].op_str == 'dword ptr [rdi], ebx' and body[pending + 4].operands[0].imm == no_effects, 'Repeated lethal hit can replay finale'
assert not any(native['symbol'](dependency) in native['calls'](post) for dependency in ('?AddItem@Player@@', '?Kill@Unit@@'))

creation_body = instructions(conversation_helper)
creation = positions(creation_body, '?CreateConversation@Conversation@@')[0]
assert any(entry.op_str == 'r8d, 0x1f8' for entry in creation_body[creation - 6:creation]), 'Wrong native conversation entry'
assert creation_body[creation - 1].op_str == 'rcx, rsi' and any(entry.op_str == 'r9, rbx' for entry in creation_body[creation - 6:creation]), '504 allocation/caster bridge changed'
assert creation_body[creation + 1].op_str == 'al, al' and creation_body[creation + 2].mnemonic == 'jne'
success = next(index for index, entry in enumerate(creation_body) if entry.address == creation_body[creation + 2].operands[0].imm)
failed = creation_body[creation + 3:success]
deletion = next(index for index, entry in enumerate(failed) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax]')
assert [entry.op_str for entry in failed[deletion - 3:deletion]] == ['rax, qword ptr [rsi]', 'edx, 1', 'rcx, rsi'], 'Failed504 does not delete its own unowned allocation'
rollback = next(index for index, entry in enumerate(failed) if entry.op_str == 'byte ptr [rdi + 0xd2], 0')
assert failed[rollback - 2].op_str == 'r14d, dword ptr [rdi + 0xec]' and failed[rollback - 1].mnemonic == 'jne'
assert failed[rollback - 1].operands[0].imm > failed[rollback].address, 'Old failed creation clears a newer epoch'
assert creation_body[success].op_str == 'r14d, dword ptr [rdi + 0xec]' and creation_body[success + 1].mnemonic == 'jne'
cancel_address = creation_body[success + 1].operands[0].imm
cancel = next(index for index, entry in enumerate(creation_body) if entry.address == cancel_address)
remove = positions(creation_body, '?Remove@Conversation@@')[0]
assert cancel <= remove and creation_body[remove - 1].op_str == 'rcx, rsi', 'Revoked504 is not removed by its own pointer'
for operand, branch in (('byte ptr [rdi + 0xd2], 0', 'je'), ('byte ptr [rdi + 0xd1], 0', 'jne'),
                        ('dword ptr [rcx + 0xa18], 0', 'jne'), ('byte ptr [rcx + 0x39c5], 0', 'jne'),
                        ('eax, 0x17722', 'jne'), ('dword ptr [rax + 0xe0], 0x5d7', 'jne'),
                        ('qword ptr [rax + 8], r12', 'jne'), ('dword ptr [rax + 0x3b8], r15d', 'jne')):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and creation_body[index + 1].mnemonic == branch
               and creation_body[index + 1].operands[0].imm == cancel_address
               for index, entry in enumerate(creation_body[success:cancel], start=success)), f'Missing504 post-creation context gate {operand}'
world = positions(creation_body, '?IsInWorld@Object@@')[0]
assert creation_body[world + 2].operands[0].imm == cancel_address
revalidate = positions(creation_body, eligible)[0]
assert [entry.op_str for entry in creation_body[revalidate - 3:revalidate]] == ['r8d, 4', 'rdx, r13', 'rcx, rdi']
assert creation_body[revalidate + 2].operands[0].imm == cancel_address
assert creation_body[remove + 1].op_str == 'r14d, dword ptr [rdi + 0xec]' and creation_body[remove + 2].mnemonic == 'jne'
assert creation_body[remove + 3].op_str == 'byte ptr [rdi + 0xd2], 0' and creation_body[remove + 4].op_str == 'al, al', 'Cancellation lost epoch-guarded retry rollback/false result'
assert any(entry.mnemonic == 'inc' and entry.op_str == 'dword ptr [rcx + 0xec]' for entry in instructions('?Reset' + suffix)), 'Reset does not invalidate an in-flight creation'
factory = instructions('?CreateConversation@Conversation@@')
for dependency in ('?GenerateDynamicFieldData@Conversation@@', '??$AddToMap@VConversation@@@Map@@'):
    call = positions(factory, dependency)[0]
    assert factory[call + 1].mnemonic == 'test' and factory[call + 1].op_str == 'al, al'
    assert factory[call + 2].mnemonic in ('je', 'jne'), 'Native factory no longer consumes its generation/insertion result'
assert b'Magnar finale: conversation 504 creation failed' in native['binary'], 'Useful creation failure diagnostic missing'
assert b'Magnar finale: invalid conversation 504 handoff data' in native['binary'], 'Invalid schedule diagnostic missing'
assert any(entry.op_str == 'xmmword ptr [rdi + 0xf0], xmm0' for entry in creation_body), 'Successful504 GUID is not retained for scoped Reset cleanup'

handoff_body = instructions(handoff)
inactive = handoff_body[8].operands[0].imm
for operand, branch in (('byte ptr [rcx + 0xd1], 0', 'jne'), ('byte ptr [rcx + 0xd2], 0', 'je'),
                        ('dword ptr [rcx + 0xa18], 0', 'jne'), ('byte ptr [rcx + 0x39c5], 0', 'jne'),
                        ('eax, 0x17722', 'jne'), ('dword ptr [rcx + 0xe0], 0x5d7', 'jne'),
                        ('qword ptr [rcx + 8], 0', 'je'), ('dword ptr [rcx + 0x3b8], eax', 'jne')):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and handoff_body[index + 1].mnemonic == branch
               and handoff_body[index + 1].operands[0].imm == inactive
               for index, entry in enumerate(handoff_body[:35])), f'Missing pending/removal context guard {operand}'
assert handoff_body[31].op_str == 'eax, dword ptr [rdi + 0x114]', 'Removal no longer checks the original instance snapshot'
timer = next(index for index, entry in enumerate(handoff_body) if entry.op_str == 'eax, dword ptr [rdi + 0x110]')
assert handoff_body[timer + 1].op_str == 'eax, ebx' and handoff_body[timer + 3].op_str == 'eax, ebx'
assert handoff_body[timer + 2].mnemonic == 'jbe' and handoff_body[timer + 3].mnemonic == 'sub'
assert handoff_body[timer + 4].op_str == 'dword ptr [rdi + 0x110], eax'
expired = next(index for index, entry in enumerate(handoff_body) if entry.address == handoff_body[timer + 2].operands[0].imm)
assert any(entry.mnemonic == 'ret' for entry in handoff_body[timer + 5:expired]), 'Pending timer falls through into credit'
removal = positions(handoff_body, '?DespawnOrUnsummon@Creature@@')[0]
assert handoff_body[removal - 2].op_str == 'edx, edx', 'Removal must be immediate after the owned timer, not an uncancellable Creature event'
lookup = positions(handoff_body, '?GetPlayer@ObjectAccessor@@')[0]
assert any(entry.op_str == 'xmm0, xmmword ptr [rdi + 0x100]' for entry in handoff_body[removal:lookup]), 'Recipient GUID snapshot lost'
revalidate = positions(handoff_body, eligible)[0]
fallback = positions(handoff_body, recipient)[0]
credit = positions(handoff_body, '?UpdateAchievementCriteria@Player@@')[0]
assert lookup < revalidate < fallback < credit
assert handoff_body[revalidate - 4].op_str == 'r8d, 4' and handoff_body[fallback - 3].op_str == 'edx, edx'
assert handoff_body[fallback - 2].op_str == 'r8d, 4' and handoff_body[fallback + 2].op_str == 'rsi, rsi'
assert handoff_body[fallback + 3].mnemonic == 'je', 'Missing recipient consumes pending completion'
for operand in ('edx, 0x5c', 'r8d, 0xb2d8', 'rcx, rsi', 'byte ptr [rdi + 0x118], r14b'):
    assert any(entry.op_str == operand for entry in handoff_body[fallback:credit]), f'Missing native handoff credit/claim {operand}'
assert handoff_body[credit + 1].op_str == 'ebx, dword ptr [rdi + 0xec]' and handoff_body[credit + 2].mnemonic == 'jne', 'Reentrant Reset arms an old removal'
stages = [index for index, entry in enumerate(handoff_body) if entry.mnemonic == 'cmp' and entry.op_str == 'eax, 5']
assert len(stages) == 2 and stages[0] < lookup < credit < stages[1], 'Existing stage5 is not recognized without duplicate credit'
assert handoff_body[stages[0] + 3].op_str == 'word ptr [rdi + 0x118], 0x100'
assert handoff_body[stages[0] + 4].op_str == 'dword ptr [rdi + 0x110], 0x1770'
stage = stages[1]
assert handoff_body[stage + 1].mnemonic == 'jne'
assert handoff_body[stage + 2].op_str == 'byte ptr [rdi + 0x119], 1'
assert handoff_body[stage + 3].op_str == 'dword ptr [rdi + 0x110], 0x1770'
retry = next(entry for entry in handoff_body if entry.address == handoff_body[stage + 1].operands[0].imm)
assert retry.op_str == 'byte ptr [rdi + 0x118], 1', 'Unadvanced native stage does not remain retryable'
for operand, branch in (('byte ptr [rdi + 0xd1], r14b', 'jne'), ('dword ptr [rcx + 0xa18], r14d', 'jne'),
                        ('byte ptr [rcx + 0x39c5], r14b', 'jne'), ('eax, 0x17722', 'jne'),
                        ('dword ptr [rcx + 0xe0], 0x5d7', 'jne'), ('qword ptr [rcx + 8], rbp', 'jne'),
                        ('dword ptr [rcx + 0x3b8], eax', 'jne'), ('eax, 0x38d', 'jne')):
    assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and handoff_body[index + 1].mnemonic == branch
               and handoff_body[index + 1].operands[0].imm == retry.address
               for index, entry in enumerate(handoff_body[credit:stage], start=credit)), f'Missing post-credit retry guard {operand}'
assert handoff_body[credit + 2].operands[0].imm > retry.address, 'Old epoch may re-arm the new attempt'
assert not positions(handoff_body, '?Remove@Conversation@@'), 'Normal departure removes Hruthnir remaining dialogue'
reset_body = instructions('?Reset' + suffix)
retire = positions(reset_body, '?Remove@Conversation@@')[0]
assert positions(reset_body, '?GetConversation@ObjectAccessor@@')[0] < retire < positions(reset_body, '?Reset@EventMap@@')[0]
assert any(entry.op_str == 'word ptr [rbx + 0x118], ax' for entry in reset_body[:retire]), 'Reset leaves a pending/removal timer active'
assert any(entry.op_str == 'qword ptr [rbx + 0x110], rax' for entry in reset_body[:retire]), 'Reset leaves timer/instance state active'
assert any(entry.op_str == 'r8d, 0x2000180' for entry in reset_body[retire:]), 'Finale cancellation does not restore native duel flags while retaining NPC immunity'
updated = instructions('?UpdateAI' + suffix)
tick = positions(updated, handoff)[0]
victim = positions(updated, '?UpdateVictim@CreatureAI@@')[0]
assert tick < victim and updated[tick + 1].op_str == 'byte ptr [rbx + 0xd2], 0' and updated[tick + 2].mnemonic == 'jne', 'Victim/evade can cancel a still-running finale'

selection_body = instructions(recipient)
owner = positions(selection_body, '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')[0]
owner_check = positions(selection_body, eligible)[0]
assert selection_body[owner - 3].op_str == 'rdx, rdx' and selection_body[owner - 2].mnemonic == 'je'
assert selection_body[owner - 2].operands[0].imm > selection_body[owner].address, 'Null attacker reaches owner resolver'
assert selection_body[owner_check - 2].op_str == 'rdx, rbx', 'Eligibility is not checked on resolved owner'
assert any(entry.op_str == 'r15d, r8d' for entry in selection_body[:10])
assert selection_body[owner_check - 3].op_str == 'r8d, r15d', 'Owner gate lost the requested scenario step'
assert selection_body[owner_check + 1].op_str == 'al, al' and selection_body[owner_check + 2].mnemonic == 'je'
fallback = selection_body[owner_check + 2].operands[0].imm
assert any(entry.mnemonic == 'ret' and entry.address < fallback for entry in selection_body[owner_check + 3:]), 'Eligible owner no longer precedes fallback'
assert any(entry.op_str == 'rax, qword ptr [rax + 0x768]' for entry in selection_body), 'Fallback does not enumerate actual Map players'
assert len(positions(selection_body, '?GetDistance@WorldObject@@QEBAMPEBV1@@')) == 2
assert any(entry.mnemonic == 'comiss' and entry.op_str == 'xmm0, xmm6' and selection_body[index + 1].mnemonic == 'jbe'
           for index, entry in enumerate(selection_body[:-1])), 'Fallback no longer selects nearest eligible participant'

for gate_body, player_register, step_register in ((instructions(eligible), 'rbx', 'esi'), (selection_body, 'rsi', 'r15d')):
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
                            ('al, 1', 'jne'), ('eax, 1', 'jne'), ('eax, 0x38d', 'jne'), (f'eax, {step_register}', 'jne')):
        assert any(entry.mnemonic == 'cmp' and entry.op_str == operand and gate_body[index + 1].mnemonic == branch
                   and gate_body[index + 1].operands[0].imm == rejection for index, entry in enumerate(gate_body[:-1])), f'Missing gate: {operand}'
    assert len(positions(gate_body, '?GetQuestStatus@Player@@')) == 2
    if player_register == 'rbx':
        assert any(entry.op_str == 'esi, r8d' for entry in gate_body[:10]), 'Eligibility no longer compares the supplied step'
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
print('PASS: canonical Magnar post-slot retains scaled/owner-first policy and acknowledged504, snapshots actual four-line handoff/recipient/instance, and cannot immediately credit/despawn. Installed pending clock precedes victim/evade gates, revalidates the recipient, credits45784 before actual909/5 arms its cancellable6000ms removal, and Reset retires only owned504. Native factory failure/epoch/cancellation policy retained; client speech/departure VFX remain separate.')
