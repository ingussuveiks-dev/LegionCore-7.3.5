"""Verify linked Burden admission and Grievous amount/damage/healing hooks."""
from pathlib import Path
import runpy

from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
burden = lambda name: '?' + name + '@spell_challengers_burden@@'
wound = lambda name: '?' + name + '@spell_challengers_grievous_wound@@'


def references(body, target):
    address = n['symbol'](target)
    return [i for i, instruction in enumerate(body) if instruction.mnemonic == 'lea'
            and any(operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
                    and instruction.address + instruction.size + operand.mem.disp == address
                    for operand in instruction.operands)]


registration = n['instructions'](burden('Register'))
amounts = references(registration, burden('CalculateAmount'))
assert len(amounts) == 3
third = registration[amounts[2] - 1:amounts[2] + 8]
assert any(i.mnemonic == 'mov' and i.op_str == 'r9d, 0xe2' for i in third)
assert any(i.mnemonic == 'mov' and i.op_str == 'r8b, 3' for i in third)
assert sites(n, third, '??0EffectCalcAmountHandler@AuraScript@@')
calculate = n['instructions'](burden('CalculateAmount'))
assert sites(n, calculate, '?isInTankSpec@Player@@')
assert {1, 2, 12} <= values(calculate)

registration = n['instructions'](wound('Register'))
for method, constructor, hook in (
        ('CalculateAmount', 'EffectCalcAmountHandler', 'rcx, [rbx + 0x240]'),
        ('CalculateTickDamage', 'EffectChangeTickDamageHandler', 'rcx, [rbx + 0x270]'),
        ('OnTick', 'EffectPeriodicHandler', 'rcx, [rbx + 0x1c8]')):
    at, = references(registration, wound(method))
    body = registration[max(0, at - 1):at + 13]
    assert 89 in values(body)
    assert sites(n, body, '??0' + constructor + '@AuraScript@@')
    assert any(i.mnemonic == 'lea' and i.op_str == hook for i in body)

calculate = n['instructions'](wound('CalculateAmount'))
assert 10.0 in floats(n, calculate)
assert any(i.mnemonic == 'divss' for i in calculate)
damage = n['instructions'](wound('CalculateTickDamage'))
assert {'cvttss2si', 'mulsd', 'divsd'} <= {i.mnemonic for i in damage}
tick = n['instructions'](wound('OnTick'))
owner, = sites(n, tick, '?GetUnitOwner@AuraScript@@')
health, = sites(n, tick, '?HealthBelowPct@Unit@@')
prevent, = sites(n, tick, '?PreventDefaultAction@AuraScript@@')
stack, = sites(n, tick, '?ModStackAmount@Aura@@')
assert owner < health < prevent < stack and 90 in values(tick)
assert any(i.mnemonic == 'jmp' and i.op_str == 'qword ptr [rax + 0x18]'
           for i in tick[prevent:stack])
print('PASS linked Burden effect3 calculation, native tank-role guard, Grievous /10 amount, '
      'double-precision fractional correction and owner 90-percent veto/removal before stacking. '
      'Native damage/healing probes and full client combat remain separate.')
