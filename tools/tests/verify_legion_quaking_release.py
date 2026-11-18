"""Verify the existing Quaking controller and native trigger/interrupt chain."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM

from sharasdal_erunak_entry_pe import sites,values,fields

n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
update=n['instructions']('?Update@Challenge@@')
assert 20000 in values(update)
assert sites(n,update,'?ApplyOnEveryPlayer@Map@@')
name,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??Update@Challenge@@' in name]
admit=n['instructions'](name)
assert {240447,0x800000}<=values(admit)
assert 0x30d8 in fields(admit)  # real native vehicle receipt
for target in ('?CanContact@Player@@','?HasAura@Unit@@','?HasFlag@Object@@','?AddAura@Unit@@'):
    assert sites(n,admit,target),target
trigger=n['instructions']('?HandlePeriodicTriggerSpellAuraTick@AuraEffect@@')
assert sites(n,trigger,'?CastSpell@Unit@@')
interrupt=n['instructions']('?EffectInterruptCast@Spell@@')
# GetCurrentSpell is inlined as an indexed native Unit spell-slot read.
assert any(o.type==CS_OP_MEM and o.mem.disp==0xb00 and o.mem.scale==8
           for op in interrupt for o in op.operands)
assert sites(n,interrupt,'?InterruptSpell@Unit@@')
print('PASS linked existing Quaking:20-second controller,actual contact/'
      'existing-aura/no-actions/vehicle gates,self warning240447,native '
      'periodic triggered CastSpell and effect68 current-spell interruption. '
      'Matching DB2/native regressions verify2500ms,20percent,5yards and '
      '5-second lock;client traversal remains separate.')
