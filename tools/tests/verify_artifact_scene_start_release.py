"""Check the actual source17 consumer in the current canonical x64 PE."""
from pathlib import Path
import runpy
from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM,CS_OP_REG
from capstone.x86_const import X86_REG_R8D
n=runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
caller='?CheckCast@Spell@@'
lookup=n['symbol']('?GetConditionsForNotGroupedEntry@ConditionMgr@@')
assert lookup in n['calls'](caller),'Spell::CheckCast lost actual source17 lookup'
assert n['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@') in n['calls'](caller),'Spell cast does not evaluate the returned caster/target conditions'
address=n['symbol'](caller)
# Matching map bound, including all MSVC unwind fragments, is provided by the
# same decoded call resolver used for the shared lifecycle PE checks.
following=min(v for v in n['symbols'].values() if v>address)
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
at=n['offset'](address)
body=list(decoder.disasm(n['binary'][at:at+following-address],address))
sites=[i for i,v in enumerate(body) if v.mnemonic=='call' and v.operands[0].type==CS_OP_IMM and n['resolve'](v.operands[0].imm)==lookup]
assert len(sites)==1,('Unexpected source lookup count',len(sites))
# MSVC puts the returned ConditionList address in RDX after this/RCX;
# ConditionSourceType is therefore the third register argument, R8D.
assert any(v.mnemonic=='mov' and len(v.operands)==2 and v.operands[0].type==CS_OP_REG and v.operands[0].reg==X86_REG_R8D and v.operands[1].type==CS_OP_IMM and v.operands[1].imm==17 for v in body[max(0,sites[0]-12):sites[0]]),'Actual condition lookup source argument is not17'
print('PASS actual canonical Spell::CheckCast fetches source17 and evaluates caster/target ConditionSourceInfo; native host separately executes unchanged action85 delay and the extracted actual bridge against SQL rows.')
