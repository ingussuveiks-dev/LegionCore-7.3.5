"""Check unchanged native phasing consumers in the canonical executable."""
from pathlib import Path
import runpy
from capstone import CS_OP_MEM

retreat = runpy.run_path(str(Path(__file__).with_name('verify_warswords_retreat_release.py')))
native = retreat['native']
for prefix in ('?Recalculate@PhaseMgr@@', '?SetPhaseId@Unit@@',
               '?InSamePhase@WorldObject@@QEBA_NPEBV1@@Z', '?InSamePhaseId@WorldObject@@QEBA_NPEBV1@@Z', '?NotifyConditionChanged@PhaseMgr@@',
               '?UpdatePhasing@InstanceScript@@', '?LoadGameobjects@ObjectMgr@@'):
    assert retreat['instructions'](prefix), f'Canonical native phase consumer missing: {prefix}'
for dependency in ('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEB',
                   '?GetConditionsForPhaseDefinition@ConditionMgr@@'):
    assert native['symbol'](dependency) in native['calls']('?Recalculate@PhaseMgr@@'), f'Actual inlined phase conditions not dispatched: {dependency}'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xa8
           for entry in retreat['instructions']('?Recalculate@PhaseMgr@@')), 'Native phase set virtual call missing'
assert native['symbol']('?SetPhaseId@WorldObject@@') in native['calls']('?SetPhaseId@Unit@@'), 'Unit phase update lost object phase-ID storage'
assert native['symbol']('?InSamePhaseId@WorldObject@@QEBA_NPEBV1@@Z') in native['calls']('?InSamePhase@WorldObject@@QEBA_NPEBV1@@Z')
print('PASS: canonical phase definition/condition/recalculation/intersection consumers present; source-extracted native test and deployed SQL supply the weapon-only policy. Not client traversal or cached-GUID interaction certification.')
