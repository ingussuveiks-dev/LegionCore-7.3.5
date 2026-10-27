"""Follow action50 flag4 into the actual native hasCreator argument."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RSP

p=runpy.run_path(str(Path(__file__).with_name('verify_lights_wrath_admission_release.py')))
n,ins=p['n'],p['ins']
body=ins('?ProcessAction@SmartScript@@')
target=n['symbol']('?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM')
sites=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==target]
assert len(sites)==4, 'Native retry/fixed/unit/random summons must all retain their creator policy'
for i in sites:
 window=body[max(0,i-32):i]
 stores=[j for j,x in enumerate(window) if x.mnemonic=='mov' and len(x.operands)==2 and
         x.operands[0].type==CS_OP_MEM and x.operands[0].mem.base==X86_REG_RSP and
         x.operands[0].mem.disp==0x68 and x.operands[0].size==1 and x.operands[1].type==CS_OP_REG]
 assert len(stores)==1
 j=stores[0];store=window[j]
 shift,negate,mask=window[j-3:j]
 assert shift.mnemonic=='shr' and shift.operands[1].type==CS_OP_IMM and shift.operands[1].imm==2
 assert negate.mnemonic=='not' and negate.operands[0].reg==store.operands[1].reg
 assert mask.mnemonic=='and' and mask.operands[0].reg==store.operands[1].reg and mask.operands[1].imm==1
 # Trace the actual SQL action_param3 field rather than a nearby constant bit.
 earlier=body[max(0,i-90):i]
 assert any(x.mnemonic=='mov' and x.operands[0].type==CS_OP_REG and x.operands[0].reg==shift.operands[0].reg and
            x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==0x60 for x in earlier)
assert n['symbol']('?AddGameObject@Unit@@') in n['calls']('?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM')
assert n['symbol']('?Delete@GameObject@@') in n['calls']('?RemoveAllGameObjects@Unit@@')
assert n['symbol']('?RemoveAllGameObjects@Unit@@') in n['calls']('?RemoveFromWorld@Unit@@')
print('PASS actual action50 four native summon calls pass hasCreator=!(action_param3&4) at x64 stack68 from actual flags field60; native creator list/cleanup still deletes owned objects. Native full ownership/cleanup host verifies flag4 retains lifetime0/summoner/phase and preserves ordinary/retry modes; client chest pending.')
