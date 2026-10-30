"""Existing native dispatch consumed by the Priest-only SQL condition repair."""
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
 ('?SetScript9@SmartAI@@','?SetScript9@SmartScript@@'),
 ('?SetScript9@SmartScript@@','?GetScript@SmartAIMgr@@'),
 ('?SetScript9@SmartScript@@','?InitTimer@SmartScript@@'),
 ('?UpdateTimer@SmartScript@@','?ProcessEvent@SmartScript@@'),
):assert n['symbol'](q) in n['calls'](p),(p,q)
a=n['symbol']('?SetData@SmartAI@@');i=bisect.bisect_right(n['starts'],a-n['base'])-1;b,e,u=n['functions'][i]
body=list(d.disasm(n['binary'][n['offset'](a):n['offset'](a)+n['base']+e-a],a))
# ProcessEventsFor(this RCX, event EDX, unit R8, var0 R9, ...).
assert any(x.mnemonic=='xor' and len(x.operands)==2 and all(o.type==CS_OP_REG and o.reg==X86_REG_R8D for o in x.operands) for x in body),'SetData no longer passes actual null Unit'
print('PASS actual Release SmartAI null-Unit SetData, root precheck, native condition-target/nearest-creature and native timed-list initialization/update dispatch. SQL shapes and scoped ASAN host verify Priest data; actual client handoff remains separate.')
