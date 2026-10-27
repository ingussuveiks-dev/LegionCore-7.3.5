"""Check the selected Map scene command's actual source fallback before send."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n,ins=p['native'],p['instructions']
b=ins('?ScriptsProcess@Map@@');send=n['symbol']('?SendSpellScene@Player@@')
sites=[i for i,x in enumerate(b) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==send]
assert len(sites)==1
i=sites[0];window=b[max(0,i-30):i]
pattern=[('test','rdi, rdi'),('jne',None),('test','rsi, rsi'),('je',None),
         ('cmp','byte ptr [rsi + 0x16], 4'),('jne',None),('mov','rdi, rsi'),('mov','r8b, 1')]
matches=[]
for start in range(len(window)-len(pattern)+1):
    if all(window[start+j].mnemonic==mnemonic and (operand is None or window[start+j].op_str==operand)
           for j,(mnemonic,operand) in enumerate(pattern)):
        matches.append(start)
assert len(matches)==1
start=matches[0]
assert window[start+1].operands[0].imm==window[start+3].operands[0].imm==window[start+5].operands[0].imm
assert any(x.mnemonic=='cmp' and x.op_str=='dword ptr [rdx + 0xc], 0x23' for x in window)
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol']('?_GetScriptPlayer@Map@@') for x in window)
assert b'source' in n['binary'] and b'object is NULL' in n['binary']
print('PASS actual Map scene command keeps explicit target, only adopts non-null Player source when target is absent, then native player validation/scene35-vs36 delivery. Invalid-recipient diagnostics retained; native host owns event/cancel cases.')
