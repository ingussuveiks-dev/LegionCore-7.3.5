"""Inspect post-damage Raging and scoped native evade cleanup in the final PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM

from sharasdal_erunak_entry_pe import sites, values, fields

n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
damage=n['instructions']('?DealDamage@Unit@@')
gate,=[i for i,op in enumerate(damage) if 206150 in values([op])]
block=damage[gate-3:gate+72]
assert 2 in values(block) and 228318 in values(block)
assert sites(n,block,'?GetAuraEffect@Unit@@')
assert sites(n,block,'?GetAuraApplication@Unit@@')
assert sites(n,block,'?CastSpell@Unit@@')
assert any(i<gate for i in sites(n,damage,'?ModifyHealth@Unit@@'))
instance=sites(n,damage,'?GetInstanceScript@WorldObject@@')
assert any(i<gate for i in instance)
health=max(i for i in sites(n,damage,'?ModifyHealth@Unit@@') if i<gate)
# Encounter and final AI policy hooks use virtual dispatch in this core.
assert any(op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM
           for op in damage[min(instance):health])
# Actual alive state, effect amount and dynamic disabled-affix mask are kept.
assert {0xa18,0x1c,0x3dc8}<=fields(block) and 0x40 in values(block)
# Two unsigned /10 operations form floor(3*maximum/10) without overflow;
# health is read as uint64 after the damage, not projected from its receipt.
assert sum(op.mnemonic=='mul' for op in block)==2
assert 0xcccccccccccccccd in {value & ((1<<64)-1) for value in values(block)}
assert {0x38,0x40}<=values(block)
assert any(op.mnemonic=='cmp' and op.op_str=='r8, rdx' for op in block)
compare=next(i for i,op in enumerate(block) if op.mnemonic=='cmp' and op.op_str=='r8, rdx')
assert block[compare+1].mnemonic=='ja'  # inclusive30-percent threshold

proc=n['instructions']('?HandleDummyAuraProc@Unit@@')
might,=[i for i,op in enumerate(proc) if 206150 in values([op])]
case=proc[might:might+14]
effect,=[i for i,op in enumerate(case) if op.mnemonic=='cmp' and 2 in values([op])]
assert case[effect+1].mnemonic=='je'
false=case[effect+1].operands[0].imm
at=next(i for i,op in enumerate(proc) if op.address==false)
assert any(op.mnemonic=='xor' and op.op_str in ('al, al','eax, eax') for op in proc[at:at+5])

evade=n['instructions']('?_EnterEvadeMode@CreatureAI@@')
assert {206150,228318,2}<=values(evade)
assert sites(n,evade,'?GetAuraEffect@Unit@@')
assert sites(n,evade,'?RemoveAurasDueToSpell@Unit@@')
assert sites(n,evade,'?RemoveAllAurasExceptType@Unit@@')
assert sites(n,evade,'?CombatStop@Unit@@')
print('PASS linked Raging:final surviving health after scaling/AI policies, '
      'native Might effect2/amount/disable guards,inclusive exact uint64 '
      '30-percent threshold,one228318 self cast,outgoing proc veto and scoped '
      'evade cleanup preserving ordinary/vehicle native aura handling. '
      'Actual client combat remains separate.')
