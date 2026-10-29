"""Follow the registered Monk kite spell factory and all three live hooks."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_takeover_release.py')))
n, ins, references, table, needs, hook = (p[k] for k in ('n', 'ins', 'references', 'table', 'needs', 'hook'))

def values(method):
    return {op.imm for instruction in ins(method) for op in instruction.operands if op.type == CS_OP_IMM}

factory = '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_monk_kite_travel@@'
register = '?Register@spell_monk_kite_travel@@'
validate = '?Validate@spell_monk_kite_travel@@'
assert any(n['symbol'](factory) in table(address, 8) for address in references('?AddSC_artifact_acquisition@@'))
assert any(n['symbol'](register) in table(address, 10) and n['symbol'](validate) in table(address, 10)
           for address in references(factory)), 'Registered factory does not return the tested spell vtable'
for method in ('Check', 'Guard', 'After'):
    hook(register, '?' + method + '@spell_monk_kite_travel@@')
assert {202475,202474,102531,140,90} <= values(validate)
eligible = '?Eligible@MonkKiteTravel@@'
assert {40633,1514,10} <= values(eligible)
current = '?Current@spell_monk_kite_travel@@'
check = '?Check@spell_monk_kite_travel@@'
guard = '?Guard@spell_monk_kite_travel@@'
after = '?After@spell_monk_kite_travel@@'
needs(check, eligible, '?getQuestStatus@Player@@')
needs(current, eligible, '?getQuestStatus@Player@@')
needs(guard, current, '?PreventHitDefaultEffect@SpellScript@@')
needs(after, current, '?GetQuestObjectiveData@Player@@QEBAHII@Z')
assert {202481,40633,102531} <= values(after)
after_calls = n['calls'](after)
assert any(n['resolve'](address) in after_calls for name, address in n['symbols'].items()
           if name.startswith('?CastSpell@Unit@@')), 'Actual accepted-hit native teleport cast missing'
for prefix in ('?KilledMonsterCredit@Player@@', '?RewardPlayerAndGroupAtEvent@Player@@', '?AddItem@Player@@', '?TeleportTo@Player@@'):
    assert not any(n['resolve'](address) in after_calls for name, address in n['symbols'].items() if name.startswith(prefix)), prefix
print('PASS actual registered Monk kite spell factory/vtable/Validate/Check/effect Guard/AfterHit: native202475/202474/personal102531, active Mon1514/40633 quest lease, stale-hit suppression and native202481 only after actual objective; no direct item, group/quest credit or teleport API. Native host covers rejects/deferred/reentry/denial; actual client travel remains pending.')
