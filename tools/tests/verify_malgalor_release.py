"""Follow selected Malgalor AI/aura/instance hooks in the actual linked PE."""
from pathlib import Path
import runpy
import struct
import re
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n = p['native']

def ins(method):
    try:
        return p['instructions'](method)
    except AssertionError as error:
        if 'Missing runtime bounds' not in str(error):
            raise
        address = n['symbol'](method)
        end = min(n['resolve'](a) for a in n['symbols'].values() if n['resolve'](a) > address)
        assert end - address <= 128, (method, 'unbounded leaf')
        start = n['offset'](address)
        return list(p['decoder'].disasm(n['binary'][start:start + end - address], address))

def refs(method):
    return {x.address + x.size + op.mem.disp for x in ins(method) for op in x.operands
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP}

def table(address, count=80):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))

def needs(method, *targets):
    for target in targets:
        matching = {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(target)}
        assert matching & n['calls'](method), (method, target)

def values(method):
    return {op.imm for x in ins(method) for op in x.operands if op.type == CS_OP_IMM}

ai = 'npc_malgalorAI@npc_malgalor@@'
factory = '?GetAI@npc_malgalor@@'
death, damage, update = ('?' + name + '@' + ai for name in ('JustDied', 'DamageTakenAfterScaling', 'UpdateAI'))
selected = [table(a) for a in refs(factory) if n['symbol'](death) in table(a)]
assert len(selected) == 1 and all(n['symbol'](m) in selected[0] for m in (damage, update))
for method, dispatch in ((death, '?Kill@Unit@@'), (damage, '?DealDamage@Unit@@')):
    slot = selected[0].index(n['symbol'](method)) * 8
    assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == slot
               for x in ins(dispatch)), (dispatch, slot)
needs(death, '?Death@MalgalorFinale@@')
needs(damage, '?Capture@MalgalorFinale@@', '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
assert {5500, 186382} <= values(damage)
assert {24000} <= values('?DoAction@' + ai)
assert {6000, 97166} <= values(update)
needs(update, '?Kill@Unit@@')

scope = 'instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
callback = '?CreatureDies@' + scope
assert any(n['symbol'](callback) in table(a) for a in refs('?GetInstanceScript@instance_broken_shore@@'))
needs(callback, '?MalgalorDeath@' + scope)
needs('?MalgalorDeath@' + scope, '?Death@MalgalorFinale@@', '?Capture@MalgalorFinale@@')
needs('?Death@MalgalorFinale@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@',
      '?Original@MalgalorFinale@@', '?AddQuestDelayedEvent@Player@@', '?AddDelayedEvent@FunctionProcessor@@')
assert {103661, 46881, 191979, 4000, 8000} <= values('?Death@MalgalorFinale@@')
needs('?Depart@MalgalorFinale@@', '?TeleportTo@Player@@', '?RemoveAurasDueToSpell@Unit@@', '?Original@MalgalorFinale@@')
needs('?ExitCompletedBrokenShoreLogin@Player@@', '?TeleportTo@Player@@', '?GetQuestObjectiveData@Player@@')
assert {1500, 933, 38904, 42815, 103661, 1479} <= values('?ExitCompletedBrokenShoreLogin@Player@@')

script = 'spell_warrior_malgalor_arrival@@'
get = '?GetAuraScript@?$GenericAuraScriptLoader@V' + script
register = '?Register@' + script
assert any(n['symbol'](get) in table(a, 8) for a in refs('?AddSC_wars_intro_scenario@@'))
assert any(table(a, 10)[5] == n['symbol'](register) for a in refs(get))
for method in ('Apply', 'Removed'):
    assert n['symbol']('?' + method + '@' + script) in refs(register)
# Hide-list insertion/removal inline; visibility remains a virtual call.
needs('?Apply@' + script, '?GetUnitOwner@AuraScript@@', '?FindNearestCreature@WorldObject@@')
needs('?Removed@' + script, '?FindMap@MapManager@@', '?GetCreature@Map@@')
for method in ('?Apply@' + script, '?Removed@' + script):
    assert any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0xf0
               for x in ins(method)), (method, 'visibility virtual dispatch')
creature_table = re.search(r'\?\?_7Creature@@6BUnit@@@\s+([0-9a-fA-F]{16})',
                          (n['args'].release / 'worldserver.map').read_text(errors='replace'))
assert creature_table and table(int(creature_table.group(1), 16))[0xf0 // 8] == n['symbol']('?UpdateObjectVisibility@Unit@@')
assert {1500, 933, 361167, 92608} <= values('?Apply@' + script)
for method in ('?Apply@' + script, '?Removed@' + script):
    for forbidden in ('?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@', '?TeleportTo@Player@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method)
print('PASS selected Malgalor AI post-scale/death/update, shared instance fallback, native credit/departure/login recovery and registered per-viewer scene aura hooks. Client animation/traversal remains separate.')
