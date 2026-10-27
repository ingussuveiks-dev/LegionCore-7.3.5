"""Verify the actual scoped self target and guarded native arrival consumers."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

p=runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_return_release.py')))
n,ins,needs,values=(p[k] for k in ('n','ins','needs','values'))
jump='?EffectJumpDest@Spell@@'
needs(jump,'?GetGuidValue@Object@@','?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@',
      '?GetSpellTriggerDelay@Spell@@','?CalculateJumpSpeeds@Spell@@')
assert {1479,203921,203920,191474,193335,205814,225114,225162,225219,225232,236618,234642,241931}<=values(jump)
b=ins(jump);guid=n['symbol']('?GetGuidValue@Object@@')
sites=[i for i,x in enumerate(b) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==guid]
assert len(sites)>=2 # new direct native self target plus existing delayed fallback
first=sites[0]
assert b[first-1].op_str=='rcx, r8' and b[first-2].op_str=='edx, edx'
assert b[first+1].mnemonic=='movups' and b[first+1].op_str=='xmm0, xmmword ptr [rax]'
assert b[first+2].op_str=='xmmword ptr [rsp + 0x74], xmm0' # full target GUID in actual descriptor
move='?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@'
assert {1479,203921,203920}<=values(move)
needs(move,'?Launch@MoveSplineInit@Movement@@')
arrival='?MovementInform@EffectMovementGenerator@@'
needs(arrival,'?GetUnit@ObjectAccessor@@','?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert {1479,203921,203920,94954,52174}<=values(arrival)
# Keep the existing native null/cleanup refusal. The repaired caller supplies
# a real self target; it does not weaken Unit::CastSpell validation.
cast='?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N'
assert any(x.mnemonic=='test' and x.op_str=='rdx, rdx' for x in ins(cast))
print('PASS linked destination jump stores full native self GUID for scoped Skyhold pairs; original MoveJump launch/arrival and Unit null guard retained, existing Heroic landing gate preserved. Native host covers branch behavior; client transfer pending.')
