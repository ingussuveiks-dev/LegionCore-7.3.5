"""Actual first30 registration,core money and selected mutation guards."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from ashbringer_storage_pe import reaches
p=runpy.run_path(str(Path(__file__).with_name('verify_second_five_ten_20261114_release.py')))
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
registry=refs(ins('?GetCommands@modify_commandscript@@'))
methods=('Drunk','HP','Mana','Faction','Currency','ArtifactXP','Morph','Phase','PhaseIDs')
for name in methods:assert n['symbol']('?HandleModify'+name+'Command@modify_commandscript@@')in registry,name

def failed(body,index):
 assert body[index+1].mnemonic=='test' and body[index+1].op_str=='al, al'
 branch=body[index+2];assert branch.mnemonic in ('je','jne')
 return branch.operands[0].imm if branch.mnemonic=='je'else body[index+3].address

def rejects_before(body,index,writes):
 assert writes
 assert all(not reaches(body,failed(body,index),body[x].address)for x in writes)

for name,prefixes in [('Drunk',('?SetDrunkValue@Player@@',)),('HP',('?SetMaxHealth@Unit@@','?SetHealth@Unit@@')),('Mana',('?SetMaxPower@Unit@@','?SetPower@Unit@@')),('Currency',('?ModifyCurrency@Player@@',)),('Faction',('?setFaction@Unit@@','?SetUInt32Value@Object@@'))]:
 body=ins('?HandleModify'+name+'Command@modify_commandscript@@');writes=[x for prefix in prefixes for x in sites(body,prefix)];parsers=sites(body,'??$ParseCommandInteger@')
 assert parsers,name
 for at in parsers:rejects_before(body,at,writes)
 if name!='Faction':
  security,=sites(body,'?HasLowerSecurity@ChatHandler@@');branch=body[security+2];assert branch.mnemonic in ('je','jne')
  reject=branch.operands[0].imm if branch.mnemonic=='jne'else body[security+3].address
  assert all(not reaches(body,reject,body[x].address)for x in writes),name

core=ins('?ModifyMoney@Player@@');write,=sites(core,'?SetMoney@Player@@');hook,=sites(core,'?OnPlayerMoneyChanged@ScriptMgr@@')
cap,=[i for i,x in enumerate(core)if x.mnemonic=='movabs'and x.operands[-1].type==CS_OP_IMM and x.operands[-1].imm==99999999999]
assert hook<cap<write
assert core[cap+1].mnemonic=='cmp'and core[cap+2].mnemonic=='ja'
assert not reaches(core,core[cap+2].operands[0].imm,core[write].address),'Above-cap subtraction must not wrap into accepted write'
assert any(x.mnemonic=='test'and core[i+1].mnemonic=='je'and not reaches(core,core[i+1].operands[0].imm,core[write].address)for i,x in enumerate(core[hook+1:cap],hook+1)),'Recheck callback amount zero'

artifact=ins('?HandleModifyArtifactXPCommand@modify_commandscript@@');write,=sites(artifact,'?SetUInt64Value@Object@@');state,=sites(artifact,'?SetState@Item@@');owner,=sites(artifact,'?GetArtifactWeapon@Player@@');security,=sites(artifact,'?HasLowerSecurity@ChatHandler@@')
assert security<owner<write<state
bound,=[i for i,x in enumerate(artifact)if x.mnemonic=='not'and x.op_str=='rcx'];assert artifact[bound+1].mnemonic=='cmp'and artifact[bound+2].mnemonic=='ja'
assert not reaches(artifact,artifact[bound+2].operands[0].imm,artifact[write].address)
assert any(x.mnemonic=='cmp'and x.op_str=='al, 0x2d'for x in artifact[:owner]),'Inline UInt64 parser rejects minus'
currency=ins('?HandleModifyCurrencyCommand@modify_commandscript@@');convert,=[i for i,x in enumerate(currency)if x.mnemonic=='cvttsd2si'];setter,=sites(currency,'?ModifyCurrency@Player@@')
assert sum(x.mnemonic=='comisd'for x in currency[:convert])==2 and convert<setter and sites(currency,'?GetCurrencyPrecision@DB2Manager@@')
for name in ('Morph','Phase','PhaseIDs'):
 body=ins('?HandleModify'+name+'Command@modify_commandscript@@');assert sites(body,'?HasLowerSecurity@ChatHandler@@'),name
 if name!='Morph':assert sites(body,'??$ParseCommandInteger@I@Trinity@@'),name
 else:assert sites(body,'?SetDisplayId@Unit@@')or any(x.mnemonic=='call'and x.operands[0].type==CS_OP_MEM for x in body)
print('PASS first30 selected handlers/native money:registered paths,typed parse/authority reject CFG before writes,post-hook money cap/zero,artifact UInt64 sign/addition bound and double currency conversion bounds. Native/client scopes separate.')
