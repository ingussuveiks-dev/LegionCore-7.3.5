from linked_player_abi import inherited_operand
"""Bind first-phase Reset recovery to the selected controllers in Release."""
from pathlib import Path
import runpy
import struct
import re
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(n, ins):
    hr = '@npc_hrutnirAI@npc_hrutnir@@'
    boss = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'

    def sites(body, prefix):
        targets = {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i, op in enumerate(body) if op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_IMM and n['resolve'](op.operands[0].imm) in targets]

    def has(body, *operands):
        for operand in operands:
            assert any(op.op_str == operand for op in body), 'Missing installed reset operand: ' + operand

    reset = ins('?Reset' + boss)
    cleanup, = sites(reset, '?DespawnAll@SummonList@@')
    notification, = sites(reset, '?ResetMagnarOffer' + hr)
    combat=any(name.startswith('?Restore@MagnarCombat@@') for name in n['symbols'])
    owner='rbx' if combat else 'rcx';flag='r15b' if combat else 'r14b';zero='r13' if combat else 'r12';generation='r12d' if combat else 'r15d'
    begin=sites(reset,'?Restore@MagnarCombat@@')[0]+1 if combat else 0
    has(reset[:cleanup], f'byte ptr [{owner} + 0xd1], 0',f'byte ptr [{owner} + 0xd5], 0',flag+', 1',flag+', '+flag)
    first=next(i for i,op in enumerate(reset) if i>=begin and op.mnemonic=='cmp' and op.op_str==f'byte ptr [{owner} + 0xd1], 0')
    releasing=next(i for i,op in enumerate(reset) if i>first and op.mnemonic=='cmp' and op.op_str==f'byte ptr [{owner} + 0xd5], 0')
    false=next(i for i,op in enumerate(reset) if i>releasing and op.mnemonic=='xor' and op.op_str==flag+', '+flag)
    first_branch=next(op for op in reset[first+1:releasing] if op.mnemonic=='je')
    assert first_branch.operands[0].imm==reset[false].address
    assert reset[releasing+1].mnemonic=='jne' and reset[releasing+1].operands[0].imm==reset[false].address
    assert not any(v.type==CS_OP_MEM and v.mem.disp==0xe8 for op in reset[first:false] for v in op.operands),'Fresh replacement still requires its lost local encounterStarted claim'
    has(reset[cleanup:notification],f'byte ptr [rbx + 0xd5], {zero}b',f'byte ptr [rbx + 0xe8], {zero}b',flag+', '+flag,
        f'byte ptr [rdi + 0x858], {zero}b','qword ptr [rdi + 0x9c0], rbx',f'dword ptr [rdi + 0xa18], {zero}d',
        inherited_operand(f'byte ptr [rdi + 0x39c5], {zero}b'),'eax, 0x17722','dword ptr [rdi + 0xe0], 0x5d7','r8d, 0x429')
    assert sites(reset[cleanup:notification],'?GetCreature@Map@@'),'Reset does not resolve the instance-owned companion'
    guards=[i for i,op in enumerate(reset) if op.mnemonic=='cmp' and op.op_str=='dword ptr [rbx + 0x208], '+generation]
    assert len(guards)==2 and guards[0]<cleanup<guards[1]<notification
    for index in guards:assert reset[index+1].mnemonic=='jne' and reset[index+1].operands[0].imm>reset[notification].address
    # The inlined bridge casts the actual current or suspended AI to Hruthnir.
    # The shared map reader retains code symbols; read the actual RTTI data
    # pointed to by dynamic_cast's R9 target argument instead of a data symbol.
    casts = []
    for op in reset[cleanup:notification]:
        if op.mnemonic == 'lea' and op.op_str.startswith('r9, '):
            for operand in op.operands:
                if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                    offset = n['offset'](op.address + op.size + operand.mem.disp) + 16
                    expected = b'.?AUnpc_hrutnirAI@npc_hrutnir@@\0'
                    if n['binary'][offset:offset + len(expected)] == expected:
                        casts.append(op)
    assert len(casts) == 2
    has(reset[notification - 2:notification], 'rdx, rdi', 'rcx, rax')
    tail=reset[notification+1:]
    if combat:
        assert tail[0].mnemonic=='mov' and tail[0].op_str=='r15, qword ptr [rsp + 0x168]'
        tail=tail[1:]
    assert [(op.mnemonic,op.op_str) for op in tail[:2]]==[('mov','rcx, qword ptr [rsp + 0x100]'),('xor','rcx, rsp')]
    # The shared symbol reader intentionally indexes C++ names only. Bind the
    # sole C stack-cookie helper from the same timestamp-verified linker map.
    cookies={n['resolve'](int(m[1],16)) for m in re.finditer(r'\b__security_check_cookie\s+([0-9a-fA-F]{16})\s+f', (n['args'].release/'worldserver.map').read_text(errors='replace'))}
    assert len(cookies)==1
    assert tail[2].mnemonic=='call' and n['resolve'](tail[2].operands[0].imm) in cookies
    assert all(op.mnemonic in ('add','pop','ret') for op in tail[3:]),'Notification can access a destroyed AI afterwards'
    factory = ins('?GetAI@npc_magnar_icebreaker@@')
    if sites(factory, '??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'):
        factory += ins('??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    tables = []
    for op in factory:
        for operand in op.operands:
            if op.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                offset = n['offset'](op.address + op.size + operand.mem.disp)
                if struct.unpack_from('<Q', n['binary'], offset + 0x28)[0] == n['symbol']('?Reset' + boss):
                    tables.append(offset)
    assert len(tables) == 1, 'Factory does not select tested Magnar Reset'
    initialization=n['symbol']('?InitializeAI@ScriptedAI@@')
    assert n['resolve'](struct.unpack_from('<Q',n['binary'],tables[0]+0x20)[0])==n['resolve'](initialization)
    init=ins('?AIM_Initialize@Creature@@');old,=sites(init,'?AIM_Destroy@Creature@@')
    installed=next(i for i,op in enumerate(init) if op.op_str=='qword ptr [rbx + 0x9c0], rdi')
    initialize,=[i for i,op in enumerate(init) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x20]']
    assert old<installed<initialize
    has(init[installed:initialize],'byte ptr [rbx + 0x858], 1','dword ptr [rbx + 0xa18], 0')
    lock=next(i for i,op in enumerate(init) if op.mnemonic=='cmp' and op.op_str==inherited_operand('byte ptr [rcx + 0x3a9d], 0'))
    assert init[lock+1].mnemonic=='je' and init[lock+1].operands[0].imm==init[old].address
    native_init=ins('?InitializeAI@ScriptedAI@@')
    assert any(op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x28]' for op in native_init),'Native InitializeAI does not call selected Reset'

    rearm = ins('?ResetMagnarOffer' + hr)
    cancel, = sites(rearm, '?CancelMagnarStart' + hr)
    remove, = sites(rearm, '?RemoveMagnarConversation' + hr)
    has(rearm[:cancel], 'rax, qword ptr [rcx + 0x60]', 'byte ptr [rax], 0', 'rcx, [rdi + 0x17c]',
        'qword ptr [rax + 0x9c0], rdi', 'qword ptr [rax + 0x9c8], rdi', 'eax, 0x178d4', 'eax, 0x17722',
        'dword ptr [rax + 0xe0], 0x5d7', 'qword ptr [rbx + 8], rax', 'dword ptr [rbx + 0x3b8], eax',
        'byte ptr [rbx + 0x858], 0', 'edx, 0x426', 'r8d, 0x429', 'eax, 0x38d', 'eax, 3')
    assert len(sites(rearm[:cancel], '??9ObjectGuid@@')) == 2, 'Exact boss/companion full GUID checks missing'
    assert sites(rearm[:cancel], '?HasMagnarChamberRequest' + hr)
    receipt, = [i for i, op in enumerate(rearm[:cancel]) if op.mnemonic == 'call' and op.op_str == 'qword ptr [rax + 0x50]']
    assert rearm[receipt + 1].op_str == 'eax, eax' and rearm[receipt + 2].mnemonic == 'jne'
    assert rearm[receipt + 2].operands[0].imm > rearm[remove].address
    assert cancel < remove
    has(rearm[cancel:remove], 'dword ptr [rdi + 0x178], 0')
    assert all(op.mnemonic in ('mov', 'add', 'pop', 'ret') for op in rearm[remove + 1:]), 'Post-cleanup access can touch a destroyed AI'
    start = ins('?ContinueMagnarStart' + hr)
    claim = next(i for i, op in enumerate(start) if op.op_str == 'byte ptr [rax], 1')
    store = next(i for i, op in enumerate(start) if op.op_str == 'xmmword ptr [rbx + 0x17c], xmm6')
    receipts = [i for i, op in enumerate(start) if op.mnemonic == 'call' and op.op_str == 'qword ptr [rax + 0x50]']
    assert len(receipts) == 2 and receipts[-1] < store < claim
    clear = ins('?CancelMagnarStart' + hr)
    has(clear, 'rcx, [rbx + 0x17c]')
    assert sites(clear, '?Clear@ObjectGuid@@')
    for body in (rearm,):
        for forbidden in ('?UpdateAchievementCriteria@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@', '?NearTeleportTo@', '?MovePoint@MotionMaster@@', '?Talk@CreatureAI@@'):
            assert not sites(body, forbidden), 'Reset release bypasses ordinary progression/arrival'
    print('PASS selected Magnar first-phase Reset/replacement: native unlocked AIM_Destroy/install/enabled/alive/InitializeAI/vtable Reset handoff; first-phase/not-releasing predicate no longer requires a fresh AI local encounterStarted. Acknowledged same-boss GUID still required by companion,real reserve cleanup/current-generation guards precede typed current/suspended companion release,real1062 false/guard epoch/scenario909-3/own497 cleanup last,no movement/progression grants. Native/source matrices and client traversal remain separate.')


if __name__ == '__main__':
    p = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(p['native'], p['instructions'])
