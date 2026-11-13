"""Actual first20 command paths,including finite float and native64 money flow."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from ashbringer_storage_pe import reaches
p=runpy.run_path(str(Path(__file__).with_name('verify_five_ten_20261114_release.py')))
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
registry=refs(ins('?GetCommands@modify_commandscript@@'))
methods=('MoneyCommand','ASpeedCommand','SpeedCommand','SwimCommand','BWalkCommand','FlyCommand','ScaleCommand','ItemsLevel','BitCommand','PowerCommand')
for method in methods:
    assert n['symbol']('?HandleModify'+method+'@modify_commandscript@@') in registry,method

for method,setter in (
    ('ASpeedCommand','?SetSpeed@Unit@@'),('SpeedCommand','?SetSpeed@Unit@@'),
    ('SwimCommand','?SetSpeed@Unit@@'),('BWalkCommand','?SetSpeed@Unit@@'),
    ('FlyCommand','?SetSpeed@Unit@@'),('ScaleCommand','?SetObjectScale@Unit@@'),
    ('ItemsLevel','?SetFloatValue@Object@@'),
):
    body=ins('?HandleModify'+method+'@modify_commandscript@@')
    finite,=[i for i,x in enumerate(body)if x.mnemonic=='cmp' and any(o.type==CS_OP_IMM and o.imm==0x7f800000 for o in x.operands)]
    assert body[finite+1].mnemonic=='setne' and body[finite+2].op_str=='al, 1' and body[finite+3].mnemonic=='jne'
    reject=body[finite+3].operands[0].imm
    selected=sites(body,setter)
    if method=='ScaleCommand':
        selected=[i for i,x in enumerate(body)if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x28 and i>=3 and [y.op_str for y in body[i-3:i]]==['rax, qword ptr [rdi]','xmm1, xmm6','rcx, rdi']]
        assert len(selected)==1,'Actual unit-receiver virtual scale slot28 required'
        assert n['symbol']('?SetFloatValue@Object@@') in n['calls']('?SetObjectScale@Object@@'),'Native Object scale setter retained'
    assert selected and all(not reaches(body,reject,body[i].address)for i in selected),'Nonfinite parse must reject before '+method
    assert sites(body,'?HasLowerSecurity@ChatHandler@@'),method
    # Parser's remaining characters may contain only native ASCII whitespace.
    assert any(x.mnemonic=='test' and x.op_str in ('dl, dl','al, al') and body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm==reject for i,x in enumerate(body[finite+4:],finite+4)),'Complete float token required: '+method

item=ins('?HandleModifyItemsLevel@modify_commandscript@@')
for prefix in ('?HasPvpStatsScalingEnabled@Player@@','?SendUpdateStat@Player@@','?SendUpdateCR@Player@@'):
    assert sites(item,prefix)
security,=sites(item,'?HasLowerSecurity@ChatHandler@@')
setter,=sites(item,'?SetFloatValue@Object@@')
assert security<setter

money=ins('?HandleModifyMoneyCommand@modify_commandscript@@')
parse,=sites(money,'??$ParseCommandInteger@_J@Trinity@@')
assert money[parse+1].op_str=='al, al' and money[parse+2].mnemonic=='je'
writes=sites(money,'?SetMoney@Player@@')+sites(money,'?ModifyMoney@Player@@')
assert writes and all(not reaches(money,money[parse+2].operands[0].imm,money[i].address)for i in writes),'Invalid signed64 must not alter money'
modify,=sites(money,'?ModifyMoney@Player@@')
assert money[modify+1].op_str=='al, al' and money[modify+2].mnemonic=='je'
messages=sites(money,'??$PSendSysMessage@_KPEBD@ChatHandler@@')+sites(money,'??$PSendSysMessage@PEBD_K@ChatHandler@@')
assert messages and all(not reaches(money,money[modify+2].operands[0].imm,money[i].address)for i in messages),'Refused native change must not report a give'
assert b'GM money change: before=%u delta=%d after=%u' in n['binary'],'Retain signed64 diagnostic'

bit=ins('?HandleModifyBitCommand@modify_commandscript@@')
parse,=sites(bit,'??$ParseCommandInteger@I@Trinity@@')
assert bit[parse+1].op_str=='al, al' and bit[parse+2].mnemonic=='je'
flag_writes=sites(bit,'?RemoveFlag@Object@@')+sites(bit,'?SetFlag@Object@@')
assert flag_writes and all(not reaches(bit,bit[parse+2].operands[0].imm,bit[i].address)for i in flag_writes)
assert any(x.mnemonic=='cmp' and x.op_str=='di, r10w' for x in bit),'Typed16-bit field conversion bound is installed'

power=ins('?HandleModifyPowerCommand@modify_commandscript@@')
unsigned,=sites(power,'??$ParseCommandInteger@I@Trinity@@')
signed,=sites(power,'??$ParseCommandInteger@H@Trinity@@')
security,=sites(power,'?HasLowerSecurity@ChatHandler@@')
setter,=sites(power,'?SetPower@Unit@@')
assert security<unsigned<signed<setter
assert power[signed+1].op_str=='al, al' and power[signed+2].mnemonic=='je'
assert not reaches(power,power[signed+2].operands[0].imm,power[setter].address)
assert any(x.mnemonic=='cmp' and x.op_str=='ebx, 0x13' for x in power),'Native MAX_POWERS19 retained'
print('PASS selected first20 registered handlers;IEEE finite/whole-token rejection CFG,original movement/scale/stat endpoints,actual signed64 money parse/accepted-write receipt and64-bit messages,typed bit field and signed power/permission paths;controlled ASAN/native formatter/client scopes remain separate.')
