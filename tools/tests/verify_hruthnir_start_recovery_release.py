"""Selected Release endpoints for retryable497 -> actual Magnar acceptance.

Native/source matrices separately exercise branches. This certificate binds
those endpoints to the installed factories, scheduler and shared entry hook.
"""
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(native, instructions):
    hr = '@npc_hrutnirAI@npc_hrutnir@@'
    boss = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'

    def calls(body, dependency):
        targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(dependency)}
        return [i for i, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) in targets]

    def has(body, *operands):
        for operand in operands:
            assert any(entry.op_str == operand for entry in body), 'Missing installed operand: ' + operand

    continuation = instructions('?ContinueMagnarStart' + hr)
    for dependency in ('?HasMagnarChamberRequest' + hr, '?GetPlayer@ObjectAccessor@@', '?CanContact@Player@@',
                       '?GetQuestCancellationReceipt@Player@@', '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@',
                       '?StartMagnarConversation' + hr, '?ScheduleMagnarStart' + hr,
                       '?FindNearestCreature@WorldObject@@', '?GetCreature@ObjectAccessor@@', '?CancelMagnarStart' + hr):
        assert calls(continuation, dependency), 'Missing selected start dependency: ' + dependency
    has(continuation, 'eax, 0x178d4', 'dword ptr [rcx + 0xe0], 0x5d7', 'eax, 0x38d', 'eax, 3',
        'edx, 0x1770', 'edx, 0x3e8', 'edx, 0x17722', 'r8d, 0x9917',
        'byte ptr [rbx + 0x161], 1', 'byte ptr [rbx + 0x162], 0',
        'qword ptr [rax + 0x9c0], rbx', 'qword ptr [r15 + 0x9c8], rbx',
        'qword ptr [rbx + 0x60], rax', 'qword ptr [rdi + 0x9c0], rsi')
    command = next(i for i, entry in enumerate(continuation) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x48]')
    receipts = [i for i, entry in enumerate(continuation) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x50]']
    lookup = calls(continuation, '?GetCreature@ObjectAccessor@@')[0]
    claim = next(i for i, entry in enumerate(continuation) if entry.op_str == 'byte ptr [rax], 1')
    assert len(receipts) == 2 and receipts[0] < command < lookup < receipts[1] < claim
    for receipt in receipts:
        assert any(entry.op_str == 'edx, 0x426' for entry in continuation[receipt - 5:receipt])
        assert continuation[receipt + 1].op_str == 'eax, eax'
    assert continuation[receipts[1] + 2].mnemonic == 'je' and continuation[receipts[1] + 2].operands[0].imm < continuation[command].address
    assert continuation[receipts[0] + 2].mnemonic == 'jne' and continuation[receipts[0] + 2].operands[0].imm > continuation[command].address
    has(continuation[lookup:claim], 'dword ptr [rdi + 0xa18], 0', 'byte ptr [rdi + 0x39c5], 0', 'byte ptr [rdi + 0x858], 0')
    factory = calls(continuation, '?StartMagnarConversation' + hr)[0]
    assert continuation[factory + 1].op_str == 'al, al' and continuation[factory + 2].mnemonic == 'je'
    assert continuation[factory + 2].operands[0].imm > continuation[factory + 3].address
    for name in ('?ContinueMagnarStart' + hr, '?ScheduleMagnarStart' + hr, '?RejoinMagnarStart' + hr):
        for forbidden in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@', '?CompleteQuest@Player@@', '?NearTeleportTo@'):
            assert not calls(instructions(name), forbidden), 'Start recovery grants progress or moves the player'
    schedule = instructions('?ScheduleMagnarStart' + hr)
    assert calls(schedule, '?AddDelayedEvent@FunctionProcessor@@')
    has(schedule, 'byte ptr [rcx + 0x163], 0', 'byte ptr [rcx + 0x163], 1', 'rcx, [r9 + 0x36a0]')
    assert any(entry.mnemonic == 'lock inc' and entry.op_str.endswith('+ 0xc]') for entry in schedule), 'Queue lost native weak ownership'
    callback = instructions('??$invoke@AEAV<lambda_1>@?1??ScheduleMagnarStart' + hr)
    assert calls(callback, '?TryMagnarStart' + hr)
    has(callback, 'byte ptr [rax], 0')
    assert any(entry.mnemonic == 'lock cmpxchg' for entry in callback), 'Queue lost native weak lock'
    attempt = instructions('?TryMagnarStart' + hr)
    has(attempt, 'byte ptr [rcx + 0x160], 0', 'byte ptr [rcx + 0x164], 0', 'byte ptr [rcx + 0x164], 1', 'qword ptr [rdi + 0x60], rsi')
    assert calls(attempt, '?ContinueMagnarStart' + hr)
    has(attempt, 'rbp, qword ptr [rcx + 0x20]', 'qword ptr [rbp + 0x9c0], rdi', 'qword ptr [rbp + 0x9c8], rdi')
    # Captured Creature identity gates all post-callback accesses to this AI.
    assert sum(entry.op_str == 'qword ptr [r15 + 0x9c8], rbx' for entry in continuation) == 2
    rejoin = instructions('?RejoinMagnarStart' + hr)
    assert calls(rejoin, '?AdoptQuestCancellationReceipt@Player@@')
    has(rejoin, 'rdx, [rdi + 0x140]', 'eax, 3', 'eax, 1', 'edx, 0x9917')
    entry = instructions('?OnPlayerEnter@instance_shields_rest_InstanceMapScript@instance_shields_rest@@')
    assert calls(entry, '?GetCreature@Map@@') and calls(entry, '?Rejoin@MagnarStart@@')
    rejoin_bridge = instructions('?Rejoin@MagnarStart@@')
    assert calls(rejoin_bridge, '?RejoinMagnarStart' + hr), 'Entry does not reach selected Hruthnir metadata adoption'
    receipt_body = instructions('?GetData' + boss)
    has(receipt_body, 'edx, 0x426', 'byte ptr [rcx + 0xe8], 0', 'eax, 0x17722', 'dword ptr [rax + 0xe0], 0x5d7', 'eax, 0x38d', 'eax, 3')
    assert not calls(receipt_body, '?DoAction')
    destructor = instructions('??_Gnpc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    has(destructor, 'byte ptr [rcx + 0xd1], 0', 'qword ptr [rax + 8], 0')
    assert calls(destructor, '?DespawnAll@SummonList@@'), 'AI replacement loses owned reserve cleanup'
    # Bind acknowledgement to the vtable actually constructed by the factory.
    selected = []
    factory_body = instructions('?GetAI@npc_magnar_icebreaker@@')
    if calls(factory_body, '??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'):
        factory_body += instructions('??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@')
    for entry in factory_body:
        for operand in entry.operands:
            if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                offset = native['offset'](entry.address + entry.size + operand.mem.disp)
                if struct.unpack_from('<Q', native['binary'], offset + 0x50)[0] == native['symbol']('?GetData' + boss):
                    selected.append(offset)
    assert len(selected) == 1, 'Factory does not select the real acknowledgement'
    movement = instructions('?MovementInform' + hr)
    assert calls(movement, '?TryMagnarStart' + hr) and not calls(movement, '?StartMagnarConversation' + hr)
    assert calls(instructions('??_Gnpc_hrutnirAI@npc_hrutnir@@'), '??1npc_hrutnirAI@npc_hrutnir@@')
    for name in ('?Reset' + hr, '??1npc_hrutnirAI@npc_hrutnir@@', '?DoAction' + hr):
        assert calls(instructions(name), '?CancelMagnarStart' + hr), 'Old request survives lifecycle/epoch reset'
    print('PASS selected Hruthnir retry: acknowledged497 -> original6000ms cue, weak Unit queue/1000ms retry, original GUID/quest receipt/current-or-suspended identity, same-boss native1062 before claim, re-entry receipt adoption and first-phase owned-reserve destructor. No recovery progress/item/teleport grants; source/native matrices and client traversal remain separate.')
