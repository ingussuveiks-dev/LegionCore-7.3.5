"""Follow actual registrations, aura completion and map-owned creation in Release."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP, X86_REG_RSP

p = runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_chest_lifetime_release.py')))
n, ins = p['n'], p['ins']

def values(method):
    return {op.imm for x in ins(method) for op in x.operands if op.type == CS_OP_IMM}

def references(method):
    return {x.address + x.size + op.mem.disp for x in ins(method) for op in x.operands
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}

def table(address, count=40):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))

def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)

def hook(register, callback):
    assert n['symbol'](callback) in references(register), (register, callback)

# Factory-produced vtables, not merely map symbols or unused helper bodies.
factory = '?GetAI@?$GenericCreatureScript@Unpc_lights_wrath_takeover@@'
lifecycle = '?UpdateLifecycle@npc_lights_wrath_takeover@@'
reset = '?Reset@npc_lights_wrath_takeover@@'
assert any(table(a)[34] == n['symbol'](lifecycle) and n['symbol'](reset) in table(a)
           for a in references(factory))
registration = references('?AddSC_artifact_acquisition@@')
for name, kind in (('request', 'Spell'), ('channel', 'Aura'), ('result', 'Spell')):
    script = 'spell_lights_wrath_control_' + name + '@@'
    get = '?Get' + kind + 'Script@?$Generic' + kind + 'ScriptLoader@V' + script
    register = '?Register@' + script
    assert any(table(a, 10)[5] == n['symbol'](register) for a in references(get))
    assert any(n['symbol'](get) in table(a, 8) for a in registration)
    if name != 'channel':
        hook(register, '?Check@' + script)
        hook(register, '?Hit@' + script)
    else:
        for callback in ('Apply', 'Tick', 'Removed'):
            hook(register, '?' + callback + '@' + script)
        assert {9, 23, 26} <= values(register)  # real/reapply, periodic trigger, root
hook('?Register@spell_lights_wrath_control_result@@', '?After@spell_lights_wrath_control_result@@')
assert any(n['symbol'](factory) in table(a, 16) for a in registration)

scope = 'State@LightsWrathTakeover@@'
update, spawn = '?Update@' + scope, '?Spawn@' + scope
needs(lifecycle, update, '?Cleanup@' + scope, '?Earned@LightsWrathTakeover@@')
assert {6, 7, 1065, 47119} <= values(lifecycle)
assert {1583, 104985, 370524} <= values('?Scoped@LightsWrathTakeover@@')
assert {5, 1065, 41631, 47117} <= values('?Eligible@LightsWrathTakeover@@')
needs('?Current@' + scope, '?Original@' + scope, '?Eligible@LightsWrathTakeover@@')
needs('?Original@' + scope, '?getQuestStatus@Player@@', '?GetGuidValue@Object@@')
request = '?Check@spell_lights_wrath_control_request@@'
needs(request, '?AddQuestDelayedEvent@Player@@', '?getQuestStatus@Player@@',
      '?Current@' + scope, '?Eligible@LightsWrathTakeover@@', '?canSeeOrDetect@WorldObject@@', '?IsWithinLOSInMap@WorldObject@@')
assert {207949, 207956, 207996, 5000} <= values(update)
needs(update, spawn, '?Current@' + scope, '?FindPlayer@LightsWrathTakeover@@',
      '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N', '?HasAura@Unit@@')
removed = '?Removed@spell_lights_wrath_control_channel@@'
needs(removed, '?GetTargetApplication@AuraScript@@', '?GetDuration@AuraScript@@',
      '?GetMaxDuration@AuraScript@@', '?GetAura@AuraScript@@', '?Current@' + scope)
assert {4, 30000, 200, 207956} <= values(removed)
# Native Aura::m_allDuration inlined getter: enforce elapsed >=30s in addition
# to mode EXPIRE/max duration/zero remaining, before entering Ready.
b = ins(removed)
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and
           x.operands[0].mem.disp == 0x118 and x.operands[1].type == CS_OP_IMM and
           x.operands[1].imm == 30000 and b[i+1].mnemonic == 'jl' for i, x in enumerate(b[:-1]))
tick = '?Tick@spell_lights_wrath_control_channel@@'
needs(tick, '?GetUnitOwner@AuraScript@@', '?Current@' + scope)
assert n['symbol']('?GetTargetApplication@AuraScript@@') not in n['calls'](tick)
result = '?After@spell_lights_wrath_control_result@@'
needs(result, '?Original@' + scope, '?Earned@LightsWrathTakeover@@')
assert 47119 in values(result)

# Real map insertion with original lifetime0 and hasCreator=false. Inspect the
# actual native x64 argument stores rather than accepting a matching hash.
assert {1065, 6, 7, 47119, 248572, 104985, 2000} <= values(spawn)
summon = '?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM'
needs(spawn, summon, '?isSpawned@GameObject@@', '?DespawnOrUnsummon@Creature@@',
      '?GetGameObjectListWithEntryInGrid@WorldObject@@', '?Earned@LightsWrathTakeover@@')
b = ins(spawn)
sites = [i for i, x in enumerate(b) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
         and n['resolve'](x.operands[0].imm) == n['symbol'](summon)]
assert len(sites) == 1
window = b[max(0, sites[0]-45):sites[0]]
for displacement in (0x68,):
    assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM and
               x.operands[0].mem.base == X86_REG_RSP and x.operands[0].mem.disp == displacement and
               x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 0 for x in window), hex(displacement)
# respawnTime follows eight floats at stack50; MSVC reuses zeroed EBX.
stores = [i for i, x in enumerate(window) if x.mnemonic == 'mov' and
          x.operands[0].type == CS_OP_MEM and x.operands[0].mem.base == X86_REG_RSP and
          x.operands[0].mem.disp == 0x50 and x.operands[1].type == CS_OP_REG]
assert len(stores) == 1
i = stores[0]
assert any(x.mnemonic == 'xor' and x.operands[0].type == CS_OP_REG and
           x.operands[0].reg == window[i].operands[1].reg == x.operands[1].reg for x in window[:i])

# The temporary weak authorization is consumed during synchronous preparation;
# later native spell checks/effects own a weak bound receipt, tested separately.
cast = '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@AEBVSpellCastTargets@@PEBVSpellInfo@@PEBVCustomSpellValues@@AEAUTriggerCastData@@'
needs(cast, '?prepare@Spell@@')
needs('?prepare@Spell@@', '?CheckCast@Spell@@', '?LoadScripts@Spell@@')
needs('?CheckCast@Spell@@', '?_PrepareScriptCall@SpellScript@@',
      '?Call@CheckCastHandler@SpellScript@@', '?_FinishScriptCall@SpellScript@@')
needs('?EffectSendEvent@Spell@@', '?UpdateAchievementCriteria@Player@@')
for method in (update, spawn, lifecycle, request, result, removed,
               '?Check@spell_lights_wrath_control_result@@', '?Hit@spell_lights_wrath_control_result@@'):
    for forbidden in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@',
                      '?CompleteQuest@Player@@', '?AddItem@Player@@', '?SetHealth@Unit@@', '?TeleportTo@Player@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method), (method, forbidden)
print('PASS actual four registrations/factory vtables/hooks, original quest/native scope, genuine30s aura expiry and200ms/result acceptance, native synchronous preparation, map-owned lifetime0 chest and failed creation retry. No direct progress/item/health/teleport grant. Native host owns stale/reentrant/AI/mode cases; actual combat/channel/client pending.')
