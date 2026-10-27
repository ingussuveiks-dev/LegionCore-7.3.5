"""Prove the existing native mechanisms required by the data-only retry."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

p = runpy.run_path(str(Path(__file__).with_name('verify_bilaal_combat_release.py')))
n, ins, needs = p['n'], p['ins'], p['p']['needs']
reset = ins('?OnReset@SmartScript@@')
# The actual flags field3c bit256 skips InitTimer AND the runOnce99 clear.
guard = next(i for i, x in enumerate(reset) if x.mnemonic == 'test' and
             x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0x3c and
             x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 256)
assert reset[guard+1].mnemonic == 'jne'
clear = next(x for x in reset[guard+2:] if x.mnemonic == 'mov' and
             x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0x99)
assert reset[guard+1].operands[0].imm > clear.address
assert any(x.mnemonic == 'mov' and x.operands[-1].type == CS_OP_IMM and x.operands[-1].imm == 25 for x in reset)
needs('?OnReset@SmartScript@@', '?InitTimer@SmartScript@@', '?ProcessEventsFor@SmartScript@@')
needs('?ProcessEventsFor@SmartScript@@', '?IsObjectMeetingSmartEventConditions@ConditionMgr@@', '?ProcessEvent@SmartScript@@')
needs('?ProcessEvent@SmartScript@@', '?GetConditionsForSmartEvent@ConditionMgr@@',
      '?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@', '?ProcessAction@SmartScript@@')

all_action = ins('?ProcessAction@SmartScript@@')
first = p['case'](50)
end = min(p['case'](i) for i in range(1,225) if p['case'](i) > first)
body = [x for x in all_action if first <= x.address < end]
assert body[0].operands[1].type == CS_OP_MEM and body[0].operands[1].mem.disp == 0x60
assert body[1].mnemonic == 'test' and body[1].operands[1].imm == 2
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and
           x.operands[0].mem.disp == 0x70 and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 8 for x in body[:8])
summon = n['symbol']('?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM')
site = next(i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) == summon)
assert body[site+1].mnemonic == 'test' and body[site+2].mnemonic == 'je'
failure = next(x for x in all_action if x.address == body[site+2].operands[0].imm)
assert failure.mnemonic == 'mov' and failure.operands[0].type == CS_OP_MEM and failure.operands[0].mem.disp == 0x99
if failure.operands[1].type == CS_OP_REG:
    name = failure.reg_name(failure.operands[1].reg).rstrip('bdw')
    assert any(x.mnemonic == 'xor' and x.operands[0].type == CS_OP_REG and
               x.operands[0].reg == x.operands[1].reg and x.reg_name(x.operands[0].reg).rstrip('bdw') == name
               for x in all_action[:p['maximum']])
else:
    assert failure.operands[1].imm == 0

needs('?Meets@Condition@@', '?FindNearestGameObject@WorldObject@@')
needs('??RNearestGameObjectEntryInObjectRangeCheck@Trinity@@', '?IsWithinDistInMap@WorldObject@@')
needs('?IsWithinDistInMap@WorldObject@@', '?InSamePhase@WorldObject@@QEBA_NPEBV1@@')
print('PASS existing canonical action50 actual retry2/fixed-target/null-result runOnce99 recovery, creator4 native argument, DONT_RESET256 skip before timer/latch clearing and real RESET25/condition dispatch; native near-GO uses map/phase/range predicate. Loaded SQL/native host supplies full graph,2s/Reset/dedup/target1 coverage. Data only; no new EXE or progression path.')
