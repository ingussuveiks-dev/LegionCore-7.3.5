"""Existing native dispatch consumed by the separate Mage/Priest SQL repairs."""
from pathlib import Path
import runpy,bisect
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_R8D
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
d=Cs(CS_ARCH_X86,CS_MODE_64);d.detail=True
for p,q in (
 ('?SetData@SmartAI@@','?ProcessEventsFor@SmartScript@@'),
 ('?ProcessEventsFor@SmartScript@@','?IsObjectMeetingSmartEventConditions@ConditionMgr@@'),
 ('?ProcessEvent@SmartScript@@','?GetConditionsForSmartEvent@ConditionMgr@@'),
 ('?IsObjectMeetToConditionList@ConditionMgr@@','?Meets@Condition@@'),
 ('?Meets@Condition@@','?FindNearestCreature@WorldObject@@'),
 ('?Meets@Condition@@','?GetScenario@ScenarioMgr@@'),
 ('?Meets@Condition@@','?GetCriteriaTree@AchievementGlobalMgr@@'),
 ('?Meets@Condition@@','?GetAchievementMgr@Scenario@@'),
 ('?Meets@Condition@@','?IsCompletedCriteriaTree@?$AchievementMgr@VScenario@@@@'),
 ('?SetScript9@SmartAI@@','?SetScript9@SmartScript@@'),
 ('?SetScript9@SmartScript@@','?GetScript@SmartAIMgr@@'),
 ('?SetScript9@SmartScript@@','?InitTimer@SmartScript@@'),
 ('?UpdateTimer@SmartScript@@','?ProcessEvent@SmartScript@@'),
):assert n['symbol'](q) in n['calls'](p),(p,q)
a=n['symbol']('?SetData@SmartAI@@');i=bisect.bisect_right(n['starts'],a-n['base'])-1;b,e,u=n['functions'][i]
body=list(d.disasm(n['binary'][n['offset'](a):n['offset'](a)+n['base']+e-a],a))
# ProcessEventsFor(this RCX, event EDX, unit R8, var0 R9, ...).
assert any(x.mnemonic=='xor' and len(x.operands)==2 and all(o.type==CS_OP_REG and o.reg==X86_REG_R8D for o in x.operands) for x in body),'SetData no longer passes actual null Unit'
print('PASS actual Release null-Unit SetData, condition targets/nearest-creature, real scenario/criteria-tree completion and native timed-list dispatch. Separate scoped hosts verify Mage1101 and Priest1065 data; actual client handoff remains separate.')
