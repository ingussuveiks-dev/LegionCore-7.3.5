"""Bind genuinely earned same-epoch whole-boss duel recovery to Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP, X86_REG_RBX


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

    removal = ins('?OnCreatureRemove' + shared)
    presentation, = sites(removal, '?RetirePresentation@MagnarCombat@@')
    owned, = sites(removal, '?Reset@MagnarCombat@@')
    has(removal[:presentation], 'eax, 0x17722', 'qword ptr [rsi + 0x3a88], 0x583b8',
        'qword ptr [rsi + 8], rax', 'dword ptr [rcx + 0x1c], 0x1b57',
        'rcx, [rdi + 0x38c]', 'eax, dword ptr [rdi + 0x35c]',
        'dword ptr [rdi + 0x39c], eax', 'byte ptr [rdi + 0x356], 0',
        'byte ptr [rdi + 0x357], 0', 'eax, 0x38d', 'eax, 4',
        'xmmword ptr [rdi + 0x480], xmm0', 'xmmword ptr [rdi + 0x490], xmm0',
        'qword ptr [rdi + 0x4a0], xmm1', 'edx, 0x38', 'edx, 0x40')
    earned = find(removal, 'byte ptr [rdi + 0x3a0], 0', 'cmp')
    assert removal[earned+1].mnemonic == 'jne'
    capture = next(i for i, o in enumerate(removal) if o.address == removal[earned+1].operands[0].imm)
    assert capture < presentation < owned
    has(removal[capture:presentation], 'eax, 4', 'dword ptr [rsp + 0x40], eax')
    assert len(sites(removal[capture:presentation], '?GetUInt64Value@Object@@')) == 2
    initialize = ins('?Initialize' + shared)
    clear, = sites(initialize, '?Reset@MagnarCombat@@')
    has(initialize[:clear], 'xmmword ptr [rdi + 0x480], xmm0',
        'xmmword ptr [rdi + 0x490], xmm1', 'qword ptr [rdi + 0x4a0], xmm0')

    rebind = ins('?Rebind@MagnarCombat@@')
    assert len(sites(rebind, '?Find@MagnarCombat@@')) == 1
    has(rebind, 'byte ptr [rax + 0x14], 0', 'rbx, [r13 + 0xd5]', 'rsi, [r13 + 0x15]',
        'byte ptr [rbx], 0', 'edi, 0xc', 'xmm0, xmmword ptr [rax + 0x480]',
        'xmm1, xmmword ptr [rax + 0x490]', 'xmm0, qword ptr [rax + 0x4a0]',
        'eax, dword ptr [r15 + 0x35c]', 'dword ptr [rsp + 0x40], eax',
        'rax, qword ptr [rsp + 0x48]', 'rcx, qword ptr [rsp + 0x50]',
        'rax, rcx', 'eax, 0x38d', 'eax, 4', 'edx, 0x40')
    assert sites(rebind, '?IsEmpty@ObjectGuid@@') and sites(rebind, '??8ObjectGuid@@')
    previous, = sites(rebind, '?GetCreature@Map@@')
    world, = sites(rebind, '?IsInWorld@Object@@')
    publication = find(rebind, 'byte ptr [r13 + 0x14], 1', 'mov')
    consume = find(rebind, 'xmmword ptr [r15 + 0x480], xmm0', 'movups')
    assert previous < world < consume < publication
    assert rebind[world+1].op_str == 'al, al' and rebind[world+2].mnemonic == 'jne'
    has(rebind[consume:publication], 'xmmword ptr [r15 + 0x490], xmm1', 'qword ptr [r15 + 0x4a0], xmm0')
    has(rebind[publication:], 'xmmword ptr [r13 + 0x15], xmm0',
        'qword ptr [r13 + 0xd5], xmm0', 'dword ptr [r13 + 0xdd], eax', 'al, 1')
    # Exact-max equality takes the direct saved integer health path. Otherwise
    # unsigned conversions and upper/lower clamps precede the floating cast.
    has(rebind, 'r8, rax', 'qword ptr [r12], rax', 'qword ptr [r12], rdx',
        'xmm1, xmm0', 'edx, 1', 'rdx, xmm1')
    upper = find(rebind, 'xmm1, xmm0', 'comisd')
    convert = find(rebind, 'rdx, xmm1', 'cvttsd2si')
    assert upper < convert and rebind[upper+1].mnemonic == 'jae'
    bound = rebind[upper+1].operands[0].imm
    assert next(o for o in rebind if o.address == bound).op_str == 'qword ptr [r12], rdx'
    assert find(rebind, 'edx, 1', 'mov') < convert

    update = ins('?UpdateAI' + boss)
    acquire, = sites(update, '?Rebind@MagnarCombat@@')
    health, = sites(update, '?SetHealth@Unit@@')
    ordinary, = sites(update, '?UpdateFinale' + boss)
    reset_sites = [i for i,o in enumerate(update) if o.mnemonic == 'call' and o.op_str == 'qword ptr [rax + 0x28]']
    assert len(reset_sites) == 2
    reset, resumed_reset = reset_sites
    resume, = sites(update, '?NeedsReset@MagnarCombat@@')
    assert acquire < health < reset < resume < resumed_reset < ordinary
    has(update[:acquire], 'byte ptr [rcx + 0xd1], r12b', 'rdx, rcx')
    assert update[acquire+1].op_str == 'al, al' and update[acquire+2].mnemonic == 'je'
    assert update[acquire+2].operands[0].imm <= update[ordinary].address
    has(update[health+1:reset], 'qword ptr [rdi + 0x9c0], rbx', 'byte ptr [rdi + 0x858], r12b',
        'dword ptr [rdi + 0xa18], r12d', 'byte ptr [rdi + 0x39c5], r12b')
    world_after_health, = sites(update[health+1:reset], '?IsInWorld@Object@@')
    world_after_health += health+1
    assert update[reset+1].mnemonic == 'jmp'
    exit_address=update[reset+1].operands[0].imm
    for guard in (find(update, 'qword ptr [rdi + 0x9c0], rbx', 'cmp'),
                  find(update, 'byte ptr [rdi + 0x858], r12b', 'cmp'),
                  find(update, 'dword ptr [rdi + 0xa18], r12d', 'cmp'),
                  find(update, 'byte ptr [rdi + 0x39c5], r12b', 'cmp'),
                  find(update, 'byte ptr [rbx + 0xd1], r12b', 'cmp')):
        assert health < guard < reset and update[guard+1].operands[0].imm == exit_address
    assert update[world_after_health+2].mnemonic == 'je' and update[world_after_health+2].operands[0].imm == exit_address
    tail=[o for o in update if o.address>=exit_address]
    assert all(o.mnemonic in ('lea','mov','add','pop','ret') for o in tail), 'Recovery accesses its old AI after virtual Reset'
    # Readmission uses already earned current binding, never replays SetHealth;
    # both Reset calls immediately return without old-controller access.
    assert update[resume+1].op_str == 'al, al' and update[resume+2].mnemonic == 'je'
    assert update[resume+2].operands[0].imm == update[ordinary-2].address
    has(update[reset+1:resume], 'byte ptr [rbx + 0xd1], r12b', 'rdx, rbx', 'rcx, rdi')
    assert update[resumed_reset+1].mnemonic == 'jmp'
    resumed_exit = update[resumed_reset+1].operands[0].imm
    assert all(o.mnemonic in ('lea','mov','add','pop','ret') for o in update if o.address >= resumed_exit)
    ready = ins('?NeedsReset@MagnarCombat@@')
    find_ready, = sites(ready, '?Find@MagnarCombat@@')
    has(ready[find_ready+1:], 'byte ptr [rax + 0x14], 0', 'eax, 0x38d', 'eax, 4')
    scenario_ready, = sites(ready, '?GetScenario@ScenarioMgr@@')
    assert find_ready < find(ready, 'byte ptr [rax + 0x14], 0', 'cmp') < scenario_ready
    assert not sites(ready, '?SetHealth@') and not sites(ready, '?Reset@')

    retire = ins('?RetirePresentation@MagnarCombat@@')
    has(retire, 'rcx, qword ptr [rbx + 0x9c0]', 'rcx, qword ptr [rbx + 0x9c8]',
        'qword ptr [rbx + 8], 0', 'eax, 0x17722', 'qword ptr [rbx + 0x3a88], 0x583b8',
        'dword ptr [rbx + 0xe0], 0x5d7')
    target = b'.?AUnpc_magnar_icebreakerAI@npc_magnar_icebreaker@@\0'
    casts = []
    for o in retire:
        if o.mnemonic == 'lea' and o.op_str.startswith('r9, '):
            for a in o.operands:
                if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP:
                    offset = n['offset'](o.address+o.size+a.mem.disp)+16
                    if n['binary'][offset:offset+len(target)] == target:
                        casts.append(o)
    assert len(casts) == 2
    scene, = sites(retire, '?Remove@Conversation@@')
    loop = find(retire, 'xmm0, xmmword ptr [rdi]', 'movups')
    has(retire[:loop], 'rcx, [rbx + 0xf0]', 'rcx, [rbx + 0x100]',
        'dword ptr [rbx + 0xec]', 'dword ptr [rbx + 0x208]',
        'byte ptr [rbx + 0xe8], 1', 'byte ptr [rbx + 0xd5], 1',
        'byte ptr [rbx + 0xd0], 1', 'byte ptr [rbx + 0xd2], 1',
        'word ptr [rbx + 0x118], r12w', 'qword ptr [rbx + 0x110], r12')
    assert loop < scene
    ai_fields = {0xf0, 0x100, 0xec, 0x208, 0xe8, 0xd5, 0xd0, 0xd2, 0x118, 0x110}
    assert not any(a.type == CS_OP_MEM and a.mem.base == X86_REG_RBX and a.mem.disp in ai_fields for o in retire[loop:] for a in o.operands), 'AI fields accessed after callback-capable scene cleanup'
    for body in (rebind, retire):
        for forbidden in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@',
                          '?AddItem@Player@@', '?NearTeleportTo@', '?SummonCreature@', '?Kill@Unit@@', '?MoveJump@MotionMaster@@'):
            assert not sites(body, forbidden)
    print('PASS selected earned4 whole Creature recovery: observed old earned binding/epoch/fullGUID and raw health precede native scene/owned cleanup;fresh typed binding,old absence/4/fresh ownership,health bounds and once-only ticket consumption;actual SetHealth proc-boundary currentAI/enabled/world/alive/despawn guards,reentrant first_phase guard;earned current-binding readmission without HP replay;both virtual Reset paths return safely;both AI presentations frozen before captured-map504 removal. No inferred saved-step authority,forced motion or progression grant. Native132/source110/7 callback cases and client traversal remain separate.')


if __name__ == '__main__':
    p = runpy.run_path(str(Path(__file__).with_name('verify_magnar_intro_release.py')))
    for name in ('verify_magnar_reset_release.py', 'verify_magnar_combat_release.py', 'verify_magnar_recreation_release.py'):
        module = runpy.run_path(str(Path(__file__).with_name(name)))
        module['check'](p['native'], p['instructions'])
    check(p['native'], p['instructions'])
