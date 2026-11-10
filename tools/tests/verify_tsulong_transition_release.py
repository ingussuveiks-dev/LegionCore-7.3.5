"""Native full-width complements at the two actual phase actions."""
from pathlib import Path
import re,runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_karazhan_rook_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values']
r=ins('?DoAction@boss_tsulongAI@boss_tsulong@@');health=sites(r,'?SetHealth@Unit@@');assert len(health)==2
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
for at,subtract,arg in [(health[0],'rax, rbx','rdx, rax'),(health[1],'rbx, rax','rdx, rbx')]:
 window=r[max(0,at-18):at]
 reads=sites(window,'?GetUInt64Value@Object@@');assert len(reads)==2
 assert {0x38,0x40} <= set(values(window)),'native health/maxhealth64 field reads at transition'
 assert op(window,'sub',subtract) and op(window,'mov',arg),'64bit complement argument'
 assert not any(x.mnemonic=='movsxd' for x in window),'signed32 cache conversion returned'
phase=[]
for i,x in enumerate(r[:-1]):
 match=re.fullmatch(r'byte ptr \[rcx \+ (0x[0-9a-f]+)\], ([23])',x.op_str) if x.mnemonic=='cmp' else None
 if match:
  phase.append(match[1]);assert r[i+1].mnemonic=='jne'
  expected=health[0] if match[2]=='3' else health[1]
  assert r[i+1].operands[0].imm>r[expected].address,'wrong/repeated phase must skip inversion'
assert len(phase)==2 and phase[0]==phase[1],'both transitions must gate same native phase member'
reset=ins('?Reset@boss_tsulongAI@boss_tsulong@@')
assert any(x.mnemonic=='mov' and re.fullmatch(r'byte ptr \[r\w+ \+ '+phase[0]+r'\], 0',x.op_str) for x in reset),'reset phase checkpoint'
print('PASS linked Tsulong:two current native64 health/maxhealth field pairs,64bit subtraction/SetHealth arguments,same phase member gates reject replay before inversion,and reset clears phase. Complete prior Rook+367 checks retained;actual native phase body tested separately,full encounter/client remains pending.')
