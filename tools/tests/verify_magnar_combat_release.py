from linked_player_abi import inherited_operand
"""Bind the instance-owned, earned Magnar combat handoff to Release code."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(n, ins):
    boss = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
    shared = '@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'

    def sites(body, prefix):
        targets = {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i, o in enumerate(body) if o.mnemonic in ('call', 'jmp') and
                o.operands[0].type == CS_OP_IMM and n['resolve'](o.operands[0].imm) in targets]

    def find(body, text, mnemonic=None):
        return next(i for i, o in enumerate(body) if o.op_str == text and
                    (mnemonic is None or o.mnemonic == mnemonic))

    def has(body, *texts):
        for text in texts:
            find(body, text)

    lookup = ins('?Find@MagnarCombat@@')
    has(lookup, 'rcx, rcx', 'dword ptr [rbx + 0xa18], 0', inherited_operand('byte ptr [rbx + 0x39c5], 0'),
        'dword ptr [rbx + 0xe0], 0x5d7', 'eax, 0x17722',
        inherited_operand('qword ptr [rbx + 0x3a88], 0x583b8'), 'byte ptr [rbx + 0x858], 0',
        'qword ptr [rbx + 0x9c0], rdi', 'dword ptr [rcx + 0x1c], 0x1b57',
        'byte ptr [rax + 0x356], 0', 'byte ptr [rax + 0x357], 0',
        'rcx, [rdi + 0x38c]', 'ecx, dword ptr [rdi + 0x35c]',
        'dword ptr [rdi + 0x39c], ecx', 'rax, [rdi + 0x38c]', 'eax, eax')
    assert sites(lookup, '?IsInWorld@Object@@') and sites(lookup, '??9ObjectGuid@@')
    target = b'.?AUinstance_shields_rest_InstanceMapScript@instance_shields_rest@@\0'
    casts = []
    for o in lookup:
        if o.mnemonic == 'lea' and o.op_str.startswith('r9, '):
            for a in o.operands:
                if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP:
                    offset = n['offset'](o.address + o.size + a.mem.disp) + 16
                    if n['binary'][offset:offset + len(target)] == target:
                        casts.append(o)
    assert len(casts) == 1, 'Combat state must belong to the actual shared instance'

    recreation = any(name.startswith('?ConsumeRecreation@MagnarStart@@') for name in n['symbols'])
    remember = ins('?Remember@MagnarCombat@@')
    assert len(sites(remember, '?Find@MagnarCombat@@')) == 1
    has(remember, 'byte ptr [rdi + 0x14], 0', 'eax, 0x38d', 'eax, 3',
        'byte ptr [r13 + rdi], 0', 'r15d', 'edi, 0xc', 'r15d, 0xa',
        'byte ptr [rax + 0x30e8], 1', inherited_operand('xmm0, xmmword ptr [rax + 0x3b64]'),
        'esi, 0x1725d', 'esi, 0x171f3', 'esi, 0x17b63', 'eax, esi',
        'dword ptr [rbx + 0xa18], 0', 'byte ptr [rcx + 0x14], al' if recreation else 'byte ptr [rcx + 0x14], 1')
    assert sites(remember, '?IsEmpty@ObjectGuid@@') and sites(remember, '??9ObjectGuid@@')
    count = find(remember, 'r15d, 0xa', 'cmp')
    assert remember[count + 1].mnemonic == ('ja' if recreation else 'jne')
    store = find(remember, 'byte ptr [rcx + 0x14], al' if recreation else 'byte ptr [rcx + 0x14], 1', 'mov')
    assert count < find(remember, 'xmmword ptr [rcx + 0x15], xmm0', 'movups') < store
    if recreation:
        assert remember[store-1].mnemonic == 'sete' and remember[store-1].op_str == 'al'
        assert all(o.mnemonic in ('mov','movups','movsd') for o in remember[count+2:store-1]), 'Provisional ownership must not change the exact10 earned predicate'

    restore = ins('?Restore@MagnarCombat@@')
    assert len(sites(restore, '?Find@MagnarCombat@@')) == 1
    has(restore, 'byte ptr [rbp + 0x14], 0', 'eax, 0x38d', 'eax, 4',
        'xmm0, xmmword ptr [rbp + 0x15]', 'xmm0, qword ptr [rbp + 0xd5]',
        'eax, dword ptr [rbp + 0xdd]', 'rbp, [rbx + 0xc0]',
        'byte ptr [rax + 0x30e8], 1', inherited_operand('xmm0, xmmword ptr [rax + 0x3b64]'),
        'rdx, rdi', 'rcx, r14', 'rbx, 0x10', 'al, 1')
    assert len(sites(restore, '?Summon@SummonList@@')) == 1
    assert sites(restore, '?GetCreature@Map@@') and sites(restore, '??8ObjectGuid@@')

    advance = ins('?AdvanceWaveIfReady' + boss)
    receipt, = sites(advance, '?Remember@MagnarCombat@@')
    credit, = sites(advance, '?UpdateAchievementCriteria@Player@@')
    assert receipt < credit
    has(advance[receipt - 5:receipt], 'r9, [rbx + 0x1da]', 'r8, [rbx + 0x11a]', 'rdx, rbx')
    has(advance[receipt:credit], 'edx, 0x5c', 'r8d, 0xb55b', 'rcx, rdi')
    assert [(o.mnemonic, o.op_str) for o in advance[credit + 1:]] == [
        ('mov', 'rbx, qword ptr [rsp + 0x50]'), ('add', 'rsp, 0x40'), ('pop', 'rdi'), ('ret', '')], 'Native step callback may replace this AI'

    reset = ins('?Reset' + boss)
    adoption, = sites(reset, '?Restore@MagnarCombat@@')
    has(reset[adoption:adoption + 12], 'r14b, 1', 'byte ptr [rbx + 0xd1], 0', 'byte ptr [rbx + 0xd3], 7')
    has(reset, 'r8d, 0x2000180', 'rdx, qword ptr [rax + 0xa10]', 'qword ptr [rax + 0x128]')
    combat, = sites(reset, '?isInCombat@Unit@@')
    has(reset[combat - 4:combat], 'r14b, r14b')
    assert reset[combat + 1].op_str == 'al, al' and reset[combat + 2].mnemonic == 'je'

    # Bind is inlined in the real OnCreatureCreate; verify the binding precedes
    # cleanup of its old local copy, preserving reentrant binding.
    create = ins('?OnCreatureCreate' + shared)
    retire, = sites(create, '?Reset@MagnarCombat@@')
    has(create[:retire], 'r14, [rcx + 0x38c]', 'r15d, dword ptr [rcx + 0x35c]',
        'eax, 0x17722', inherited_operand('qword ptr [rbx + 0x3a88], 0x583b8'),
        'qword ptr [rbx + 8], r12', 'dword ptr [r14 + 0x10], r15d',
        'xmmword ptr [r14], xmm0', 'rdx, [rsp + 0x20]', 'rcx, r12')
    assert sites(create[:retire], '??8ObjectGuid@@')
    remove = ins('?OnCreatureRemove' + shared)
    base, = sites(remove, '?OnCreatureRemove@InstanceScript@@')
    clear, = sites(remove, '?Reset@MagnarCombat@@')
    assert base < clear
    has(remove, *(['rbx, [rdi + 0x38c]', 'rbx, 0x15', 'rdi, [rbx + 0xc0]'] if recreation else ['rdi, [rbx + 0x38c]', 'rbx, [rdi + 0x15]', 'rdi, [rbx + 0xc0]']))
    assert sites(remove, '?Clear@ObjectGuid@@') and len(sites(remove, '??8ObjectGuid@@')) == (3 if recreation else 2)
    initialize = ins('?Initialize' + shared)
    assert sites(initialize, '?Reset@MagnarCombat@@')
    has(initialize, 'rdx, [rdi + 0x38c]')
    cleanup = ins('?Reset@MagnarCombat@@')
    despawn, = sites(cleanup, '?DespawnOrUnsummon@Creature@@')
    has(cleanup[:despawn], 'byte ptr [rax + 0x30e8], 1', inherited_operand('xmm0, xmmword ptr [rax + 0x3b64]'))
    assert sites(cleanup[:despawn], '??8ObjectGuid@@')
    destructor = ins('??_Gnpc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    clear_conversation, = sites(destructor, '?Clear@ObjectGuid@@')
    get_conversation, = sites(destructor, '?GetConversation@')
    remove_conversation, = sites(destructor, '?Remove@Conversation@@')
    assert clear_conversation < get_conversation < remove_conversation
    has(destructor[:clear_conversation], 'rcx, [rdi + 0xf0]')
    for body in (lookup, remember, restore, cleanup):
        for forbidden in ('?SummonCreature@', '?Kill@Unit@@', '?UpdateAchievementCriteria@Player@@',
                          '?CompleteQuest@Player@@', '?AddItem@Player@@', '?NearTeleportTo@'):
            assert not sites(body, forbidden), 'Combat recovery must not manufacture progression or actors'
    print('PASS selected earned Magnar combat: actual typed instance/fullGUID/epoch/guard/phase context, exactly10 native death stamps before46427, step4-only owned roster adoption, combat timer rearm/player surrender-mask cleanup with original NPC immunity preserved, original504 retirement, newGUID/epoch/removal cleanup. Native60 and the source bridge matrix cover behavior separately; full client traversal remains pending.')


if __name__ == '__main__':
    p = runpy.run_path(str(Path(__file__).with_name('verify_magnar_intro_release.py')))
    reset = runpy.run_path(str(Path(__file__).with_name('verify_magnar_reset_release.py')))
    reset['check'](p['native'], p['instructions'])
    check(p['native'], p['instructions'])
