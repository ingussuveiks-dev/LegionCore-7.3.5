"""Verify selected native goober/SpellScript receipt and accepted-departure path."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_malgalor_release.py')))
n, ins, refs, table, needs, values = (p[k] for k in ('n', 'ins', 'refs', 'table', 'needs', 'values'))
script = 'spell_kingslayers_return@@'
go = 'go_kingslayers_return@@'
get = '?GetSpellScript@?$GenericSpellScriptLoader@V' + script
factory = '?GetAI@?$GenericGameObjectScript@U' + go
registration = refs('?AddSC_Kingslayers@@')
assert any(n['symbol'](get) in table(a, 8) for a in registration)
assert any(n['symbol'](factory) in table(a, 14) for a in registration)
assert any(table(a, 10)[5] == n['symbol']('?Register@' + script) for a in refs(get))
callbacks = ('Check', 'Event', 'Hit', 'Depart')
for method in callbacks:
    assert n['symbol']('?' + method + '@' + script) in refs('?Register@' + script)
def selected(address, method):
    for value in table(address, 22):
        try:
            if n['resolve'](value) == n['symbol'](method):
                return True
        except (ValueError, IndexError):
            pass
    return False
assert any(all(selected(a, '?' + method + '@' + go)
               for method in ('Reset', 'UpdateAI', 'GossipUse')) for a in refs(factory))
needs('?GossipUse@' + go, '?Physical@KingslayersReturn@@', '?AddQuestDelayedEvent@Player@@', '?Award@KingslayersReturn@@')
assert 5000 in values('?GossipUse@' + go)
needs('?UpdateAI@' + go, '?Release@' + go)
assert n['symbol']('?Reset@' + go) == n['symbol']('?Release@' + go) or n['symbol']('?Release@' + go) in n['calls']('?Reset@' + go)
needs('?Release@' + go, '?SetLootState@GameObject@@', '?SetGoState@GameObject@@')
source = '?Source@KingslayersReturn@@'
if not any(s.startswith(source) for s in n['symbols']):
    source = '?Physical@KingslayersReturn@@' # exact Source predicate inlined
assert {1620, 251123, 134046} <= values(source)
assert {1123, 48591, 48593, 128870, 9, 4} <= values('?Award@KingslayersReturn@@')
needs('?Resolve@Exit@KingslayersReturn@@', '?SameAttempt@Exit@KingslayersReturn@@', '?GetGameObject@ObjectAccessor@@')
needs('?Load@' + script, '?GetGameObjectListWithEntryInGrid@WorldObject@@', '?Resolve@Exit@KingslayersReturn@@')
needs('?Check@' + script, '?Ready@' + script)
assert any(x.mnemonic in ('call', 'jmp', 'jne') and x.operands[0].type == CS_OP_IMM and
           n['resolve'](x.operands[0].imm) == n['symbol']('?PreventHitDefaultEffect@SpellScript@@')
           for x in ins('?Event@' + script)) # conditional tail jump in this leaf
needs('?Hit@' + script, '?PreventHitDefaultEffect@SpellScript@@', '?Ready@' + script)
depart = '?Depart@' + script
teleport = n['symbol']('?EffectTeleportUnits@Spell@@')
credit = n['symbol']('?UpdateAchievementCriteria@Player@@')
needs(depart, '?EffectTeleportUnits@Spell@@', '?UpdateAchievementCriteria@Player@@', '?SameAttempt@Exit@KingslayersReturn@@')
assert {1220, 51617, 73, 92, 48593} <= values(depart)
body = ins(depart)
def sites(target):
    return [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
            and n['resolve'](x.operands[0].imm) == target]
assert len(sites(teleport)) == 1 and len(sites(credit)) == 2 and sites(teleport)[0] < min(sites(credit))
# Physical goober is still used by the native engine; do not reject its own IN_USE.
needs('?Use@GameObject@@', '?CastSpell@GameObject@@')
needs('?CastSpell@GameObject@@', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@PEBVSpellInfo@@_N')
for method in ('?GossipUse@' + go, depart):
    for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?Reward@Scenario@@', '?KilledMonsterCredit@Player@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method)
print('PASS actual registered Kingslayers return GOAI/SpellScript/hooks, exact native source/quest receipt, goober activation release, native teleport before73/92 credit and no direct quest/item/scenario-reward shortcut. Client traversal separate.')
