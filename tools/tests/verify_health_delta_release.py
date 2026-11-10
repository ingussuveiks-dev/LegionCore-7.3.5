"""Unsigned native current health with bounded signed application/prediction."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_freya_elder_health_release.py')))
ins,sites,values=p['ins'],p['sites'],p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
r=ins('?ModifyHealth@Unit@@');assert len(sites(r,'?SetHealthScal@Unit@@'))==2 and len(sites(r,'?SetHealth@Unit@@'))==1
for m,s in [('sub','rcx, rdi'),('cmp','rax, rdi'),('cmovbe','rcx, r12'),('cmp','rcx, rsi'),('cmovb','rsi, rcx'),('lea','rdx, [rsi + rdi]'),('cmp','rax, rbx'),('cmovb','rbx, rax'),('sub','rdi, rbx')]:assert op(r,m,s),(m,s)
assert sites(r,'?GetHealth@Unit@@') and sites(r,'?GetMaxHealth@Unit@@'),'retain native target-relative services'
gain=ins('?GetHealthGain@Unit@@');assert {0x38,0x40}<=set(values(gain))
for m,s in [('sub','rcx, rdi'),('cmp','rax, rdi'),('cmovbe','rcx, rdx'),('cmovb','rbx, rcx'),('cmp','rax, rbx'),('cmovb','rbx, rax')]:assert op(gain,m,s),(m,s)
damage=ins('?DealDamage@Unit@@');at,=sites(damage,'?ModifyHealth@Unit@@');window=damage[at-8:at]
assert op(window,'mov','edx, r10d') and op(window,'neg','rdx') and not any(x.mnemonic=='movsxd' for x in window),'zero-extend normalized32 damage before64bit negation'
print('PASS linked native health deltas:unsigned64 current/max comparisons,remaining-room clamp before addition,bounded negative magnitude before subtraction,prediction with native38/40fields and real nonlethal damage64negation. Complete Freya/counter/Tsulong/Rook/prior367 chain retained;native280API and four normalized damage boundaries separate from client/full spell-healing scope.')
