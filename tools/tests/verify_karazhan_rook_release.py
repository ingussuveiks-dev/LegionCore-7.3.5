"""Linked rook turn: native arrival, pose, successful cast and cardinal fields."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_thirtysixth_ten_release.py')))
n=p['n']; ins=p['ins']; sites=p['sites']; values=p['values']
def body(method): return ins('?'+method+'@npc_karazhan_chessAI@npc_karazhan_chess@@')
def op(rows,m,s): return any(x.mnemonic==m and x.op_str==s for x in rows)
hit=body('SpellHit'); arrival=body('MovementInform'); update=body('UpdateAI')
assert {229468,229567,229570,115407} <= set(values(hit))
jump,=sites(hit,'?MoveJump@MotionMaster@@QEAAXMMMMMMI_N')
assert 229329 in values(hit[max(0,jump-30):jump]),'rook-specific jump acknowledgement ID'
ground=sites(hit,'?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMI_N')
assert len(ground)>=2
for at in ground[:2]:
 assert op(hit[max(0,at-16):at],'mov','byte ptr [rsp + 0x28], 1'),'native child fields must be triggered during parent hit'
 assert 229570 in values(hit[max(0,at-16):at])
assert op(hit,'cmp','bl, 8') and op(hit,'add','r15b, 2'),'bounded distinct cardinal rays'
assert 229329 in values(arrival) and 115407 in values(arrival)
assert op(arrival,'test','byte ptr [rdx + 0x68], 0x20'),'native spline finalized gate'
assert sum(x.mnemonic=='subss' for x in arrival)==3,'authored spline endpoint XYZ comparisons'
assert not sites(arrival,'?CastSpell@Unit@@'),'arrival cannot cast from a queued old position'
cast,=sites(update,'?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@IW4TriggerCastFlags')
distance,=sites(update,'?GetExactDist@Position@@')
assert distance<cast and 229567 in values(update[distance:cast])
home,=sites(update,'?SetHomePosition@Creature@@')
assert distance<home<cast,'rook home must use committed landing pose'
assert any(x.mnemonic=='ja' and x.operands[0].imm>update[cast].address for x in update[distance:cast]),'uncommitted pose skips cast'
assert op(update[distance:cast],'xor','r9d, r9d'),'parent retains native500ms nontriggered cast'
assert sites(body('Reset'),'?InterruptNonMeleeSpells@Unit@@')
print('PASS linked Karazhan rook: reachable acknowledged jump, spline identity/completion/XYZ, map-committed pose before native500ms cast, self-hit-only triggered distinct bounded fields. Prior367 checks retained; controlled native/source tests and actual client combat remain separate.')
