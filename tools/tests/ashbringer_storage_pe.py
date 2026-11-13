"""Selected paired loader, hooks and real storage/receipt call paths in the PE."""
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def reaches(body, start, destination):
    positions = {entry.address: i for i, entry in enumerate(body)}
    pending, seen = [start], set()
    while pending:
        at = pending.pop()
        if at == destination:
            return True
        if at in seen or at not in positions:
            continue
        seen.add(at)
        entry = body[positions[at]]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j') and entry.operands[0].type == CS_OP_IMM:
            pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return False


def verify(n, ins):
    from ashbringer_storage_scope import verify as source_scope
    source_scope()

    def sites(body, prefix):
        targets = {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(prefix)}
        return [i for i, x in enumerate(body) if x.mnemonic in ('call', 'jmp') and
                x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]

    def refs(body):
        return [x.address + x.size + o.mem.disp for x in body for o in x.operands
                if x.mnemonic == 'lea' and o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP]

    def values(body):
        return {o.imm for x in body for o in x.operands if o.type == CS_OP_IMM}

    # Actual paired loader selected by AddSC, then the actual script vtables.
    pair = '?$GenericSpellAndAuraScriptLoader@Vspell_bs_ashbringer_storage@@Vaura_bs_ashbringer_storage@@@@'
    spell, aura = '?GetSpellScript@' + pair, '?GetAuraScript@' + pair
    add = ins('?AddSC_paladin_broken_shore@@')
    assert any(n['symbol'](spell) in struct.unpack_from('<12Q', n['binary'], n['offset'](at)) and
               n['symbol'](aura) in struct.unpack_from('<12Q', n['binary'], n['offset'](at)) for at in refs(add))
    for factory, klass, hooks in (
        (spell, 'spell_bs_ashbringer_storage', ('Validate', 'Register')),
        (aura, 'aura_bs_ashbringer_storage', ('Load', 'Register')),
    ):
        assert any(all(n['symbol']('?' + hook + '@' + klass + '@@') in
                           struct.unpack_from('<12Q', n['binary'], n['offset'](at)) for hook in hooks)
                   for at in refs(ins(factory)))
    for klass, hooks in (
        ('spell_bs_ashbringer_storage', ('Check', 'Create', 'Launch', 'Hit', 'After')),
        ('aura_bs_ashbringer_storage', ('Applied', 'Removed', 'Update')),
    ):
        selected = refs(ins('?Register@' + klass + '@@'))
        for hook in hooks:
            assert n['symbol']('?' + hook + '@' + klass + '@@') in selected, (klass, hook)

    body = ins('?OnLootStateChanged@go_bs_the_ashbringer@@')
    cast, = sites(body, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
    capacity, = sites(body, '?CanStoreItem@Player@@')
    error, = sites(body, '?SendEquipError@Player@@')
    receipt, = sites(body, '?Inventory@AshbringerStorage@@')
    nearest, = sites(body, '?FindNearestCreature@WorldObject@@')
    despawn, = sites(body, '?DespawnOrUnsummon@Creature@@')
    delete, = sites(body, '?Delete@GameObject@@')
    reset = sites(body, '?SetLootState@GameObject@@')
    assert len(reset) == 2 and capacity < error < reset[0] < cast < receipt < reset[1] < nearest < despawn < delete
    assert not sites(body, '?DoUpdateAchievementCriteria@InstanceScript@@'), 'No manual duplicate46130'
    assert {180850, 120978, 103454} <= values(body)
    assert all(any(x.op_str == operand for x in body[cast - 7:cast]) for operand in ('r8d, 0x2c272', 'r9b, 1', 'rdx, rbx', 'rcx, rbx'))
    assert body[cast + 2].op_str == 'eax, eax' and body[cast + 3].mnemonic == 'je'
    assert body[cast + 3].operands[0].imm == body[receipt - 1].address
    assert body[receipt + 1].op_str == 'rax, rax' and body[receipt + 2].mnemonic == 'jne'
    assert body[receipt + 2].operands[0].imm == body[nearest - 4].address
    for start in (body[capacity + 3].address, body[cast + 4].address, body[reset[1]].address):
        assert not any(reaches(body, start, body[i].address) for i in (nearest, despawn, delete))
    assert body[despawn + 1].op_str == 'esi, 2' and body[despawn + 2].mnemonic == 'jne'

    create = ins('?Create@spell_bs_ashbringer_storage@@')
    assert sites(create, '?PreventHitDefaultEffect@SpellScript@@') and sites(create, '?GetItemByEntry@Player@@')
    assert sites(create, '?GetOwnerGUID@Item@@') and sites(create, '??8ObjectGuid@@')
    assert sites(create, '?Cast@Trigger@AshbringerStorage@@') and 199827 in values(create)
    stored = ins('?Stored@spell_bs_ashbringer_storage@@')
    assert sites(stored, '?getQuestStatus@Player@@') and sites(stored, '?GetOwnerGUID@Item@@')
    assert len(sites(stored, '??8ObjectGuid@@')) == 3, 'Owner/player/full item GUID guards'
    after = ins('?After@spell_bs_ashbringer_storage@@')
    assert sites(after, '?Stored@spell_bs_ashbringer_storage@@') and sites(after, '?GetHitAura@SpellScript@@')
    assert sites(after, '?AddQuestDelayedEvent@Player@@')
    run = ins('?Run@Delivery@AshbringerStorage@@')
    assert len(sites(run, '?Receipt@AshbringerStorage@@')) == 2, 'Check real46130 before and after native cast'
    assert {38376, 1500, 775, 190777, 29, 1000} <= values(run)
    for target in ('?getQuestStatus@Player@@', '?GetItemByEntry@Player@@', '?GetQuestRewardStatus@Player@@',
                   '?CanContact@Player@@', '?GetScenarioId@Scenario@@', '?Cast@Trigger@AshbringerStorage@@',
                   '?AddQuestDelayedEvent@Player@@'):
        assert sites(run, target), target
    trigger = ins('?Cast@Trigger@AshbringerStorage@@')
    assert sites(trigger, '?GetItemByGuid@Player@@')
    assert sites(trigger, '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@AEBVSpellCastTargets@@')
    update = ins('?Update@aura_bs_ashbringer_storage@@')
    assert sites(update, '?Valid@aura_bs_ashbringer_storage@@') and sites(update, '?Cleanup@aura_bs_ashbringer_storage@@')
    valid = ins('?Valid@aura_bs_ashbringer_storage@@')
    assert sites(valid, '?GetUnitOwner@AuraScript@@') and not sites(valid, '?GetTarget@AuraScript@@')
    cleanup = ins('?Cleanup@aura_bs_ashbringer_storage@@')
    assert sites(cleanup, '?RemoveAurasDueToSpell@Unit@@') and {186987, 192145} <= values(cleanup)
    assert b"Ashbringer pickup: stored weapon's native follow-up %u was not acknowledged" in n['binary']
    print('PASS selected paired Ashbringer storage hooks/native199827 before hit effects, owned full item and quest-attempt guards, receipt190777/retry and aura-owner cleanup; GO capacity/cast/storage rejection cannot reach cleanup/Delete, no manual46130. Native42 and client traversal remain separate evidence.')
