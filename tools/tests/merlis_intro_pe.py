"""Certify the actual registered Merlis state machine in the selected PE."""
import re
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats


def reachable(body, start):
    indexed = {op.address: i for i, op in enumerate(body)}
    pending, seen = [start], set()
    while pending:
        at = pending.pop()
        if at in seen or at not in indexed:
            continue
        seen.add(at)
        i = indexed[at]
        op = body[i]
        if op.mnemonic.startswith('ret') or op.mnemonic in ('int3', 'ud2'):
            continue
        if op.mnemonic.startswith('j') and op.operands[0].type == CS_OP_IMM:
            pending.append(op.operands[0].imm)
            if op.mnemonic == 'jmp':
                continue
        if i + 1 < len(body):
            pending.append(body[i + 1].address)
    return seen


def verify(n):
    ins = n['instructions']
    method = lambda name: '?' + name + '@npc_merlis_malaganAI@npc_merlis_malagan@@'
    eligible, voice, scene, tick, receipt, destination, update, commit = [ins(method(x)) for x in
        ('IntroPlayer', 'IntroPresentation', 'IntroScene', 'UpdateIntro', 'IntroReceipt', 'IntroDestination', 'UpdateAI', 'IntroCommit')]
    pairs = lambda body: {(x.mnemonic, x.op_str) for x in body}
    assert {107831, 1620, 1123, 48587, 42504, 42627} <= values(eligible)
    for prefix, count in [('?IsInWorld@Object@@', 2), ('?IsPreDelete@Object@@', 2),
                          ('?IsQuestObjectExpired@WorldObject@@', 2), ('?CanContact@Player@@', 1),
                          ('?InSamePhase@WorldObject@@', 1), ('?CanTargetQuestObject@WorldObject@@', 2),
                          ('?GetQuestRewardStatus@Player@@', 1), ('?GetQuestStatus@Player@@', 2)]:
        assert len(sites(n, eligible, prefix)) == count, (prefix, count)
    assert {('cmp', 'dword ptr [rcx + 0xa18], 0'), ('cmp', 'dword ptr [rbx + 0xa18], 0'),
            ('test', 'byte ptr [rbx + 0x4ff0], 1'), ('cmp', 'al, 4')} <= pairs(eligible)
    assert 70.0 in floats(n, eligible) and sites(n, eligible, '?CanUpdateCriteria@Scenario@@')
    assert sites(n, eligible, method('IntroReceipt')) and ('cmp', 'qword ptr [rbx + 8], rax') in pairs(eligible)
    assert {1123, 48587} <= values(receipt)
    assert sites(n, receipt, '?GetCriteriaProgressMap@') and ('cmp', 'qword ptr [rcx + 0x28], 0') in pairs(receipt)
    assert 1620 in values(destination) and {-8342.62, 271.15, 155.34, 0.1} <= floats(n, destination)
    assert {2484, 1000} <= values(voice) and {214942, 1000} <= values(scene)
    create, = sites(n, voice, '?CreateConversation@Conversation@@')
    assert voice[create + 1].mnemonic == 'test' and voice[create + 2].mnemonic == 'je'
    accepted = voice[create + 3].address
    assert voice[create + 3].mnemonic == 'mov' and voice[create + 3].op_str == 'byte ptr [rdi + 0x10e], 1'
    assert accepted not in reachable(voice, voice[create + 2].operands[0].imm)
    assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0 for x in voice[create + 4:])
    aura, = sites(n, scene, '?HasAura@Unit@@'); cast, = sites(n, scene, '?CastSpell@Unit@@')
    assert aura < cast and scene[cast + 1].op_str == 'eax, eax'
    award, = sites(n, commit, '?UpdateAchievementCriteria@Player@@')
    check, = sites(n, commit, method('IntroReceipt'))
    assert award < check and not sites(n, tick, '?UpdateAchievementCriteria@Player@@')
    assert len(sites(n, tick, method('IntroReceipt'))) == 1
    assert {51614, 92} <= values(commit[max(0, award - 9):award])
    assert commit[check + 1].mnemonic == 'test' and commit[check + 2].mnemonic == 'jne'
    checkpoint = commit[check + 2].operands[0].imm
    assert any(x.address == checkpoint and x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
               and x.operands[0].mem.disp == 0x38 and x.operands[1].imm == 1 for x in commit)
    assert checkpoint not in reachable(commit, commit[check + 3].address), 'Rejected event must retain retry and actors'
    cleanup = sites(n, commit, '?DespawnOrUnsummon@Creature@@')
    assert len(cleanup) == 2 and all(i > check for i in cleanup)
    assert {108333, 64367} <= values(commit[check:cleanup[-1]])
    assert len(sites(n, commit, '?GetCreature@ObjectAccessor@@')) == 2
    assert len(sites(n, commit, '?GetPlayer@ObjectAccessor@@')) == 1
    assert len(sites(n, tick, '?GetPlayer@ObjectAccessor@@')) >= 4
    assert sites(n, tick, '?GetPlayerListInGrid@WorldObject@@') and not sites(n, tick, '?FindNearestPlayer@WorldObject@@')
    assert {5000, 1000} <= values(tick) and {70.0, 50.0} <= floats(n, tick)
    assert {1620} <= values(commit) and {-8342.62, 271.15, 155.34, 0.1, 2.0} <= floats(n, commit)
    # A committed native queue needs no client ACK: preserve stage7 for an
    # actual pet/guardian boss killing blow while the client is transferring.
    teleport, = sites(n, tick, '?TeleportTo@Player@@')
    commits = sites(n, tick, method('IntroCommit'))
    assert len(commits) == 2 and commits[0] < teleport < commits[1]
    later_destination, = [i for i in sites(n, tick, method('IntroDestination')) if i > teleport]
    arriving, = [i for i, x in enumerate(tick) if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
                 and x.operands[0].mem.disp == 0x10d and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 1]
    assert teleport < later_destination < arriving < commits[1]
    assert tick[later_destination + 1].mnemonic == 'test' and tick[later_destination + 2].mnemonic == 'je'
    denied = tick[later_destination + 2].operands[0].imm
    assert tick[arriving].address not in reachable(tick, denied) and tick[commits[1]].address not in reachable(tick, denied)
    # Retain actual pending flags, freshly re-resolved qualified Player and
    # our exact destination before the accepted-queue receipt/commit, including
    # compatibility wrapper early-success paths that never delegate travel.
    assert sites(n, tick[teleport:later_destination], method('IntroPlayer'))
    assert sites(n, tick[teleport:later_destination], '?GetPlayer@ObjectAccessor@@')
    for offset in (0x6848, 0x6849, 0x6851):
        assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == offset for x in tick[teleport:later_destination])
    distances = sites(n, commit, '?GetExactDist@Position@@')
    assert len(distances) >= 2 and all(i < award for i in distances)
    invalid_pose = commit[distances[-1] + 2]
    assert invalid_pose.mnemonic == 'ja' and commit[award].address not in reachable(commit, invalid_pose.operands[0].imm)
    assert not any(name.startswith('??R<lambda') and '?UpdateAI@npc_merlis_malaganAI@npc_merlis_malagan@@' in name for name in n['symbols'])
    for body in (eligible, voice, scene, tick, receipt, destination, commit):
        for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?KilledMonsterCredit@Player@@', '?AddDelayedEvent@Unit@@'):
            assert not sites(n, body, forbidden)
    assert not {51615, 51616}.intersection(values(tick+commit)), 'Intro must not grant boss death or pickup'
    poll, = sites(n, update, method('UpdateIntro')); victim, = sites(n, update, '?UpdateVictim@CreatureAI@@')
    assert poll < victim
    text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
    table, = re.findall(r'\s+\w+:\w+\s+\?\?_7npc_merlis_malaganAI@npc_merlis_malagan@@6B@\s+([0-9a-fA-F]{16})', text)
    address = int(table, 16)
    refs = {x.address + x.size + op.mem.disp for x in ins('?GetAI@npc_merlis_malagan@@')
            for op in x.operands if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}
    assert address in refs
    slots = struct.unpack_from('<82Q', n['binary'], n['offset'](address))
    slot, = [i for i, value in enumerate(slots) if n['resolve'](value) == n['symbol'](method('UpdateAI'))]
    assert any(x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == slot * 8 for x in ins('?Update@Creature@@'))
    print('PASS selected actual Merlis factory/update vtable, qualified Rogue/quest/map/phase/scope/native1123 gates, owned full-GUID five-second state, independent2484/214942 retries, verified native pending/destination before sole51614, preserving pet/guardian victory before independent client ACK; receipt before scoped original-actor cleanup; no raw Player callback, boss/pickup/item/quest grants. Client sequence remains separate.')
