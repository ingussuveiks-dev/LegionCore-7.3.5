"""Bind first-phase Reset recovery to the selected controllers in Release."""
from pathlib import Path
import runpy
import struct
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
    has(reset[:cleanup], 'byte ptr [rcx + 0xd1], 0', 'byte ptr [rcx + 0xe8], 0', 'byte ptr [rcx + 0xd5], 0', 'r14b, 1', 'r14b, r14b')
    has(reset[cleanup:notification], 'byte ptr [rbx + 0xd5], r12b', 'byte ptr [rbx + 0xe8], r12b', 'r14b, r14b',
        'byte ptr [rdi + 0x858], r12b', 'qword ptr [rdi + 0x9c0], rbx', 'dword ptr [rdi + 0xa18], r12d',
        'byte ptr [rdi + 0x39c5], r12b', 'eax, 0x17722', 'dword ptr [rdi + 0xe0], 0x5d7', 'r8d, 0x429')
    assert sites(reset[cleanup:notification], '?GetCreature@Map@@'), 'Reset does not resolve the instance-owned companion'
    guards = [i for i, op in enumerate(reset) if op.mnemonic == 'cmp' and op.op_str == 'dword ptr [rbx + 0x208], r15d']
    assert len(guards) == 2 and guards[0] < cleanup < guards[1] < notification
    for index in guards:
        assert reset[index + 1].mnemonic == 'jne' and reset[index + 1].operands[0].imm > reset[notification].address
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
    print('PASS selected Magnar first-phase Reset: acknowledged same-boss GUID stored before claim, real Reset/reserve cleanup/current-generation guards then instance-owned typed companion release, suspended AI handling, real1062 false/guard epoch/scenario909-3, own497 cleanup last, no movement/progression grants. Native/source matrices and client wipe traversal remain separate.')


if __name__ == '__main__':
    p = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(p['native'], p['instructions'])
