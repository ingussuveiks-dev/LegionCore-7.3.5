"""Inspect linked per-aura Burst damage, vehicle health and NPC exit cleanup."""
from pathlib import Path
import runpy

from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import sites, values, fields

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
burst = n['instructions']('?OnTick@spell_challengers_burst@@')
assert sites(n, burst, '?GetTarget@AuraScript@@')
assert not sites(n, burst, '?GetAuraCount@Unit@@')
assert 243237 in values(burst)
assert any(i.mnemonic == 'movzx' and any(o.type == CS_OP_MEM and o.mem.disp == 0x128
                                        for o in i.operands) for i in burst)
assert sites(n, burst, '?CastCustomSpell@Unit@@')

health = n['instructions']('?HandleAuraModIncreaseHealthPercent@AuraEffect@@')
# Vehicle bit plus every owned/summoned type bit. Only a plain vehicle may
# enter the exception; all four GUID receipts and player control stay guarded.
assert {0xc3ff, 0x20} <= values(health)
assert {0x30e8, 0x86a} <= fields(health)
assert len(sites(n, health, '?GetGuidValue@Object@@')) >= 4
assert len(sites(n, health, '?IsEmpty@ObjectGuid@@')) >= 4
assert sites(n, health, '?GetAnyOwner@Unit@@')
assert sites(n, health, '?HandleStatModifier@Unit@@')

combat = n['instructions']('?ClearInCombat@Unit@@')
# The ordinary non-pet/non-Hati NPC branch must land on shared flag cleanup,
# rather than the old charmer test/return before aura interrupt processing.
hati, = [i for i, op in enumerate(combat) if op.mnemonic == 'cmp'
         and any(o.type == CS_OP_MEM and o.mem.disp == 0x3d86 for o in op.operands)]
assert combat[hati + 1].mnemonic == 'je'
common = combat[hati + 1].operands[0].imm
at, = [i for i, op in enumerate(combat) if op.address == common]
assert 0x800 in values(combat[at:at + 5])
assert sites(n, combat[at:at + 5], '?RemoveFlag@Object@@')
assert sites(n, combat[at:], '??$RemoveAurasWithInterruptFlags@W4SpellAuraInterruptFlags@@@Unit@@')
assert 0x80000000 in values(combat[at:])
assert any(op.mnemonic == 'cmp' and op.op_str == 'byte ptr [rbx + 0x16], 4'
           for op in combat[at:])  # player-only encounter deferral retained
print('PASS linked Burst uses its own aura stack; autonomous NPC-vehicle health '
      'retains summon/owner/control guards; ordinary NPC combat exit reaches '
      'shared interrupt cleanup with player encounter deferral. Native/client checks remain separate.')
