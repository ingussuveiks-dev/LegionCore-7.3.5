"""Select observed same-instance boss removal, ownership and ordinary reoffer."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(n, ins):
    hr = '@npc_hrutnirAI@npc_hrutnir@@'
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

    consume = ins('?ConsumeRecreation@MagnarStart@@')
    has(consume, 'rcx, rcx', 'r8, r8', 'dword ptr [rbx + 0xa18], 0',
        'byte ptr [rbx + 0x39c5], 0', 'byte ptr [rbx + 0x858], 0',
        'qword ptr [rbx + 0x9c0], rsi', 'eax, 0x178d4',
        'qword ptr [rbx + 0x3a88], 0x5835a', 'qword ptr [rbx + 8], rax',
        'dword ptr [rcx + 0x1c], 0x1b57', 'qword ptr [rdi + 0x9c0], 0',
        'rcx, [rsi + 0x336]', 'rcx, [rsi + 0x38c]', 'rax, rcx',
        'byte ptr [rsi + 0x3a0], 0', 'r14, [rsi + 0x470]',
        'rcx, [rsi + 0x1a]', 'rdx, rbp', 'rdx, [rdi + 0x1a]', 'rcx, rbp',
        'eax, 0x38d', 'eax, 3', 'rdx, rbp', 'rcx, r14', 'al, 1', 'al, al')
    assert len(sites(consume, '?Find@MagnarCombat@@')) == 1
    assert sites(consume, '?IsEmpty@ObjectGuid@@') and sites(consume, '??8ObjectGuid@@') and sites(consume, '??9ObjectGuid@@')
    target = b'.?AUinstance_shields_rest_InstanceMapScript@instance_shields_rest@@\0'
    casts = []
    for o in consume:
        if o.mnemonic == 'lea' and o.op_str.startswith('r9, '):
            for a in o.operands:
                if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP:
                    offset = n['offset'](o.address + o.size + a.mem.disp) + 16
                    if n['binary'][offset:offset + len(target)] == target:
                        casts.append(o)
    assert len(casts) == 1
    clear, = sites(consume, '?clear@?$_Tree@')
    old, = sites(consume, '?GetCreature@Map@@')
    world = sites(consume, '?IsInWorld@Object@@')[-1]
    assert old < world < clear
    assert consume[world+1].op_str == 'al, al' and consume[world+2].mnemonic == 'jne'
    failure = consume[world+2].operands[0].imm
    assert next(o for o in consume if o.address == failure).op_str == 'al, al'
    assert consume[clear+1].op_str == 'al, 1'

    removal = ins('?OnCreatureRemove' + shared)
    base, = sites(removal, '?OnCreatureRemove@InstanceScript@@')
    insert, = sites(removal, '?_Insert_node@?$_Tree_val@')
    cleanup, = sites(removal, '?Reset@MagnarCombat@@')
    assert base < insert < cleanup
    has(removal[base:insert], 'eax, 0x17722', 'qword ptr [rsi + 0x3a88], 0x583b8',
        'qword ptr [rsi + 8], rax', 'dword ptr [rcx + 0x1c], 0x1b57',
        'rcx, [rdi + 0x38c]', 'eax, dword ptr [rdi + 0x35c]',
        'dword ptr [rdi + 0x39c], eax', 'byte ptr [rdi + 0x356], 0',
        'byte ptr [rdi + 0x357], 0', 'byte ptr [rdi + 0x3a0], 0',
        'eax, 0x38d', 'eax, 3', 'r12, [rdi + 0x470]',
        'xmm0, xmmword ptr [r15]', 'xmmword ptr [rax + 0x1a], xmm0', 'rcx, r12')
    initialize = ins('?Initialize' + shared)
    retire, = sites(initialize, '?Reset@MagnarCombat@@')
    set_clear = sites(initialize, '?clear@?$_Tree@')[-1]
    assert set_clear < retire
    has(initialize[set_clear-5:set_clear], 'rcx, [rdi + 0x470]')

    summon = ins('?SummonTrash@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    track, = sites(summon, '?Remember@MagnarCombat@@')
    advance, = sites(summon, '?AdvanceWaveIfReady@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    assert track < advance
    has(summon[track-5:track], 'r9, [rdi + 0x1da]', 'r8, [rdi + 0x11a]', 'rdx, rdi')

    offer = ins('?UpdateMagnarReoffer' + hr)
    admit, = sites(offer, '?ConsumeRecreation@MagnarStart@@')
    cancel, = sites(offer, '?CancelMagnarStart' + hr)
    remove, = sites(offer, '?RemoveMagnarConversation' + hr)
    receipt, = [i for i, o in enumerate(offer) if o.mnemonic == 'call' and o.op_str == 'qword ptr [rax + 0x50]']
    assert receipt < admit < cancel < remove
    has(offer[:admit], 'rax, qword ptr [rcx + 0x60]', 'byte ptr [rax], 0',
        'qword ptr [rdi + 0x60], 0', 'r9, [rdi + 0x17c]', 'r8, rbx', 'rdx, rdi')
    has(offer[cancel:remove], 'dword ptr [rdi + 0x178], esi')
    assert offer[admit+1].op_str == 'al, al' and offer[admit+2].mnemonic == 'je'
    assert offer[remove+1].mnemonic == 'jmp' and offer[remove+1].operands[0].imm == offer[admit+2].operands[0].imm
    tail = [o for o in offer if o.address >= offer[remove+1].operands[0].imm]
    assert all(o.mnemonic in ('mov', 'movaps', 'add', 'pop', 'ret') for o in tail), 'Conversation cleanup can destroy this AI'
    for body in (consume, removal):
        for forbidden in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@',
                          '?AddItem@Player@@', '?NearTeleportTo@', '?SummonCreature@', '?Kill@Unit@@'):
            assert not sites(body, forbidden)
    print('PASS selected original boss recreation: actual native removal/fullGUID/epoch/phase/guard receipt recorded before owned cleanup;typed current original companion/new boss and old absence consume once;old start/owned497 retired with tail safety,ordinary offer path preserved. Provisional owned roster is not victory:only exact10 stamps arm combat. Native72/source66/reoffer125 remain distinct from client/full-map restart checks.')


if __name__ == '__main__':
    p = runpy.run_path(str(Path(__file__).with_name('verify_magnar_intro_release.py')))
    for name in ('verify_magnar_reset_release.py', 'verify_magnar_combat_release.py', 'verify_hruthnir_reoffer_release.py'):
        module = runpy.run_path(str(Path(__file__).with_name(name)))
        module['check'](p['native'], p['instructions'])
    check(p['native'], p['instructions'])
