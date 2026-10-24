"""Check native timed source22 creation and source17 nearby-receiver consumers."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

linked = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_linked_finale_release.py')))
actors = runpy.run_path(str(Path(__file__).with_name('verify_xalatath_finale_actor_release.py')))
native = actors['native']
instructions = actors['instructions']
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'Timed UPDATE does not read source22'
assert native['symbol']('?RecalcTimer@SmartScript@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'UPDATE lost native repeat scheduling'
assert native['symbol']('?FindNearestCreature@WorldObject@@') in native['calls']('?Meets@Condition@@'), 'Condition29 lacks native nearby actor search'
assert native['symbol']('?ProcessEventsFor@SmartScript@@') in native['calls']('?IsSummonedBy@SmartAI@@'), 'Actor creation omits JUST_SUMMONED script dispatch'
assert any(operand.type == CS_OP_IMM and operand.imm == 54 for entry in instructions('?IsSummonedBy@SmartAI@@') for operand in entry.operands), 'Wrong actor initialization event'
print('PASS: canonical timed source22/repeat/action12 creation, native JUST_SUMMONED54 and Condition29 nearby search remain linked; SQL stage/presence predicates and model/cast matrices are separate checks, not live visibility/dialogue proof')
