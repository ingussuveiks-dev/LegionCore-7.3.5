"""Select the stage-aware Hunter branch of the registered shared cemetery."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_graveyard_release.py')))
native,body,prefix=n['native'],n['body'],n['prefix']
values={v.imm for x in body for v in x.operands if v.type==CS_OP_IMM}
n['needs']('?GetCurrentStep@Scenario@@')
def at(mnemonic,operands):
    matches=[i for i,x in enumerate(body) if x.mnemonic==mnemonic and x.op_str==operands]
    assert len(matches)==1, 'Expected one '+mnemonic+' '+operands
    return matches[0]
# MSVC lowers the two scenario labels to (id-944)==0 or29, and the two
# adjacent cemetery IDs to 5296-CF. Check that real data flow, not imagined
# literal973/5295 instructions which the compiler does not emit.
base=at('sub','eax, 0x3b0');delta=at('cmp','eax, 0x1d')
assert delta==base+2 and body[base+1].mnemonic=='je' and body[delta+1].mnemonic=='jne'
fury=at('mov','ebx, 0x146c')
assert body[base+1].operands[0].imm==body[fury].address
reject=body[delta+1].operands[0].imm
assert any(x.address==reject and x.mnemonic=='xor' and x.op_str=='eax, eax' for x in body)
step=native['symbol']('?GetCurrentStep@Scenario@@')
sites=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and native['resolve'](x.operands[0].imm)==step]
assert len(sites)==1 and delta<sites[0]
i=sites[0]
assert [(x.mnemonic,x.op_str) for x in body[i+1:i+4]]==[('cmp','eax, 7'),('sbb','ebx, ebx'),('add','ebx, 0x14b0')], 'Native step>=7 must choose5296;step<7 chooses5295'
lookup=at('mov','ecx, ebx');read=at('mov','rdi, qword ptr [rax + rcx*8]')
assert i+3<fury<lookup<read
assert b'Survival graveyard: invalid native WorldSafeLocs %u for instance %u' in native['binary'], 'Native Hunter source diagnostic removed'
print('PASS selected shared-instance Survival cemetery:actual973 and native5295/5296,real current-step comparison at harpoon6->7 boundary,registered native dispatch/coordinate guards and retained Fury5228;no callback progression/direct resurrection. Source58/native110/release104 and full client remain separate.')
