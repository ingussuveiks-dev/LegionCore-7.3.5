"""Bind the Pillik regression endpoints to the selected Release scripts."""
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM


def check(native, instructions):
    hr = '@npc_hrutnirAI@npc_hrutnir@@'
    shared = '@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'

    def calls(body, prefix):
        targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(prefix)}
        return [i for i, op in enumerate(body) if op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_IMM and native['resolve'](op.operands[0].imm) in targets]

    def values(body):
        return {operand.imm for op in body for operand in op.operands if operand.type == CS_OP_IMM}

    def leaf(prefix):
        start = native['symbol'](prefix)
        end = min(address for address in native['symbols'].values() if address > start)
        assert 0 < end - start <= 128, 'Unbounded scalar/identity getter'
        decoder = Cs(CS_ARCH_X86, CS_MODE_64)
        decoder.detail = True
        return list(decoder.disasm(native['binary'][native['offset'](start):native['offset'](end)], start))

    update = instructions('?UpdateAI' + hr)
    entry_poll = calls(update, '?TryPillikEntry' + hr)
    assert len(entry_poll) == 1, 'Actual update does not retry the recorded approach'
    movement = instructions('?MovementInform' + hr)
    assert calls(movement, '?TryPillikEntry' + hr), 'Actual waypoint does not attempt native entry'
    entry = instructions('?TryPillikEntry' + hr)
    assert {500, 46288, 92, 1060} <= values(entry), 'Native entry criterion/retry/epoch missing'
    for dependency in ('?IsPillikContext' + hr, '?IsPillikPlayer' + hr, '?UpdateAchievementCriteria@Player@@'):
        assert calls(entry, dependency), 'Missing selected entry admission/criterion: ' + dependency
    for dependency in ('?KilledMonsterCredit@Player@@', '?AddItem@Player@@', '?CompleteQuest@Player@@', '?TeleportTo@Player@@', '?NearTeleportTo@'):
        assert not calls(entry, dependency), 'Approach fabricates progress outside the native criterion'
    attempt = calls(update, '?TryPillikHandoff' + hr)
    if not attempt:
        poll = calls(update, '?UpdatePillikHandoff' + hr)
        assert len(poll) == 1
        attempt = poll
        assert calls(instructions('?UpdatePillikHandoff' + hr), '?TryPillikHandoff' + hr)
    assert attempt[0] < calls(update, '?UpdateMagnarIntro' + hr)[0] < calls(update, '?UpdateVictim@CreatureAI@@')[0]
    assert entry_poll[0] < attempt[0], 'Entry retry occurs after the death handoff'
    assert 500 in values(update + (instructions('?UpdatePillikHandoff' + hr) if any(name.startswith('?UpdatePillikHandoff' + hr) for name in native['symbols']) else []))
    begin = instructions('?TryPillikHandoff' + hr)
    assert calls(begin, '?ContinuePillikHandoff' + hr), 'Selected poll does not reach the real continuation'
    finish = instructions('?ContinuePillikHandoff' + hr)
    for dependency in ('?GetPlayer@ObjectAccessor@@', '?GetCreature@ObjectAccessor@@', '?IsPillikPlayer' + hr,
                       '?Talk@CreatureAI@@', '?MovePoint@MotionMaster@@', '?UpdateAchievementCriteria@Player@@'):
        assert calls(finish, dependency), 'Missing native handoff endpoint: ' + dependency
    assert calls(finish, '?Talk@CreatureAI@@')[0] < calls(finish, '?MovePoint@MotionMaster@@')[0] < max(calls(finish, '?GetPlayer@ObjectAccessor@@')) < calls(finish, '?UpdateAchievementCriteria@Player@@')[0]
    assert len(calls(finish, '?GetPlayer@ObjectAccessor@@')) >= 2, 'Recipient identity is not re-resolved after callbacks'
    assert {46289, 92, 1, 2} <= values(finish), 'Original criterion or native stage check missing'
    assert calls(instructions('?EnterEvadeMode' + hr), '?TryPillikHandoff' + hr)
    for dependency in ('?KilledMonsterCredit@Player@@', '?AddItem@Player@@', '?CompleteQuest@Player@@', '?TeleportTo@Player@@', '?NearTeleportTo@'):
        assert not calls(finish, dependency), 'Handoff fabricates a reward, quest or teleport'
    eligibility = instructions('?IsPillikPlayer' + hr)
    for dependency in ('?CanContact@Player@@', '?InSamePhase@WorldObject@@', '?IsWithinDistInMap@WorldObject@@',
                       '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@'):
        assert calls(eligibility, dependency)
    assert 39191 in values(eligibility)
    death = instructions('?RegisterPillikDeath' + shared)
    assert {94714, 909, 1} <= values(death)
    for dependency in ('?IsInWorld@Object@@', '?GetLootRecipient@Creature@@', '?InSamePhase@WorldObject@@',
                       '?GetCreatureListWithEntryInGrid@WorldObject@@', '?GetCreature@Map@@', '?Kill@Unit@@', '?DespawnOrUnsummon@Creature@@'):
        assert calls(death, dependency), 'Missing actual death/cleanup dependency: ' + dependency
    assert {96571, 1000} <= values(death)
    assert len(calls(death, '?GetCreature@Map@@')) >= 2, 'Scenery cleanup must re-resolve full GUIDs after death callbacks'
    assert not calls(death, '?UpdateAchievementCriteria@Player@@'), 'Death receipt itself grants scenario progress'
    assert calls(instructions('?OnUnitDeath' + shared), '?RegisterPillikDeath' + shared)
    creation = instructions('?OnCreatureCreate' + shared)
    assert {94714, 361361} <= values(creation), 'Original Pillik spawn identity is not bound'
    scalar = leaf('?GetData' + shared)
    assert any(op.op_str == 'eax, byte ptr [rcx + 0x384]' for op in scalar)
    identity = leaf('?GetGuidData' + shared)
    for field in (0x364, 0x374, 0x336):
        assert any(op.op_str == f'xmm0, xmmword ptr [rcx + {field:#x}]' for op in identity)
    # The existing shared guard certificate selects this exact factory table.
    # Include the new identity getter in that same selected table as well.
    factory = instructions('?GetInstanceScript@instance_shields_rest@@')
    getters = {native['symbol']('?GetData' + shared), native['symbol']('?GetGuidData' + shared), native['symbol']('?OnUnitDeath' + shared)}
    selected = 0
    for op in factory:
        if op.mnemonic != 'lea':
            continue
        for operand in op.operands:
            if operand.type == CS_OP_MEM and op.reg_name(operand.mem.base) == 'rip':
                table = native['offset'](op.address + op.size + operand.mem.disp)
                selected += getters <= set(struct.unpack_from('<64Q', native['binary'], table))
    assert selected == 1
    print('PASS selected Pillik death/loot-owner/full-GUID scenery cleanup, current typed player admission, native46289/909 stage1->2,500ms retry before victim gates and original movement/text endpoints; no death-hook credit/item/quest/teleport grant. Actual five-second weak cue/lifecycle and native Kill regressions remain separate checks; full client traversal pending.')
