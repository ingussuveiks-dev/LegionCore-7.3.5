"""Inspect the actual Release Might hook and its single aura-owned cooldown."""
from pathlib import Path
import runpy

from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
method = lambda name: '?' + name + '@spell_challengers_might@@'
registration = n['instructions'](method('Register'))
cooldown = n['instructions'](method('UpdateNecroticCooldown'))
proc = n['instructions'](method('OnProc'))
explosives = n['instructions'](method('OnUpdate'))


def references(body, target):
    address = n['symbol'](target)
    return [i for i, instruction in enumerate(body) if instruction.mnemonic == 'lea'
            and any(operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
                    and instruction.address + instruction.size + operand.mem.disp == address
                    for operand in instruction.operands)]


# The actual member pointer goes through the AuraUpdate handler constructor,
# then into the whole-aura hook list. Effect updates retain only Explosives12.
whole, = references(registration, method('UpdateNecroticCooldown'))
effect, = references(registration, method('OnUpdate'))
assert whole < effect
assert sites(n, registration[whole:effect], '??0AuraUpdateHandler@AuraScript@@')
assert any(i.mnemonic == 'lea' and i.op_str == 'rcx, [rdi + 0x1e0]'
           for i in registration[whole:effect])
tail = registration[effect:]
assert tail[1].mnemonic == 'mov' and tail[1].op_str == 'r8b, 0xc'
assert sites(n, tail, '??0EffectUpdateHandler@AuraScript@@')
assert any(i.mnemonic == 'lea' and i.op_str == 'rcx, [rdi + 0x1f8]' for i in tail)
assert 1000 in values(proc)
assert any(i.mnemonic == 'mov' and i.op_str == 'dword ptr [rcx + 0x368], 0x3e8' for i in proc)
assert any(i.mnemonic == 'mov' and i.op_str == 'dword ptr [rcx + 0x368], 0' for i in cooldown)
assert any(i.mnemonic == 'sub' and i.op_str == 'eax, edx' for i in cooldown)
assert not any(operand.type == CS_OP_MEM and operand.mem.disp == 0x368
               for i in explosives for operand in i.operands)
print('PASS linked Might: one whole-aura cooldown hook, sole effect12 Explosives update, '
      '1000ms proc reset and saturating countdown; Explosives does not decrement Necrotic. '
      'Native timing/debuff checks and actual client combat remain separate.')
