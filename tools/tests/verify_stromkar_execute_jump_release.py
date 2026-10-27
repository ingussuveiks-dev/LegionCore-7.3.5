"""Verify the linked scoped Execute destination-jump and arrival consumers."""
from pathlib import Path
import bisect
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

p = runpy.run_path(str(Path(__file__).with_name('verify_condition_search_mask_release.py')))
n = p['n']
d = Cs(CS_ARCH_X86, CS_MODE_64)
d.detail = True

def instructions(method):
    address = n['symbol'](method)
    index = bisect.bisect_right(n['starts'], address - n['base']) - 1
    begin, end, unwind = n['functions'][index]
    assert begin <= address - n['base'] < end, method
    origin = n['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in n['functions'][index + 1:]:
        if tail_begin != end or n['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    at = n['offset'](address)
    return list(d.disasm(n['binary'][at:at + n['base'] + end - address], address))

def values(method):
    return {op.imm for x in instructions(method) for op in x.operands if op.type == CS_OP_IMM}

def calls(method, *targets):
    for target in targets:
        assert n['symbol'](target) in n['calls'](method), (method, target)

jump = '?EffectJumpDest@Spell@@'
move = '?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@'
arrival = '?MovementInform@EffectMovementGenerator@@'
for method in (jump, move, arrival):
    assert {206455, 206456, 1539} <= values(method), method
calls(jump, '?GetGuidValue@Object@@', '?CalculateJumpSpeeds@Spell@@', move)
calls(move, '?Launch@MoveSplineInit@Movement@@')
calls(arrival, '?GetUnit@ObjectAccessor@@', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
cast = '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N'
assert any(x.mnemonic == 'test' and x.op_str == 'rdx, rdx' for x in instructions(cast)), 'Native null-target refusal changed'
print('PASS linked206455/206456/map1539 scoped self GUID, native launch and arrival consumers; actual native host verifies zero launch, interruption/death/map/cleanup, retry and foreign pairs. Client finale remains separate.')
