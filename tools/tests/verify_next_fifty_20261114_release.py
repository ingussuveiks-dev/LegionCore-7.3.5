"""Verify new selected registrations/guards and preserve the previous50 PE proofs."""
from pathlib import Path
import hashlib,runpy,re
from capstone import CS_OP_IMM,CS_OP_MEM
from ashbringer_storage_pe import reaches
import prepare_five_ten_20261114 as previous
from prepare_next_fifty_20261114 import verify,PLAN,ROOT

def verify_previous_with_declared_new_delta():
 verify()
 for path,record in previous.PLAN['files'].items():
  value=(ROOT/path).read_text(encoding='utf-8-sig')
  for change in reversed(PLAN['changes']+PLAN['extra_edits']):
   if change['path']==path:assert value.count(change['new'])==1;value=value.replace(change['new'],change['old'],1)
  assert hashlib.sha256(value.encode()).hexdigest()==record['after_sha256'],('Prior source outside declared new scope',path)
previous.verify=verify_previous_with_declared_new_delta
p=runpy.run_path(str(Path(__file__).with_name('verify_fifth_five_ten_20261114_release.py')))
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
for row in PLAN['changes']:
 if row['kind']=='guidhelper':continue
 klass=re.search(r'class (\w+)\s*:\s*public CommandScript',(ROOT/row['path']).read_text())[1]
 assert n['symbol']('?'+row['method']+'@'+klass+'@@')in refs(ins('?GetCommands@'+klass+'@@')),row['key']
for method,klass,endpoint in [('HandleCastDestCommand','cast','?CastSpell@Unit@@'),('HandleGameObjectTurnCommand','gobject','?SaveToDB@GameObject@@'),('HandleGameObjectMoveCommand','gobject','?SaveToDB@GameObject@@'),('HandleGameObjectSetSizeCommand','gobject','?SaveToDB@GameObject@@')]:
 body=ins('?'+method+'@'+klass+'_commandscript@@');writes=sites(body,endpoint);assert writes
 classifiers=[i for i,x in enumerate(body)if x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==0x7f800000 for o in x.operands)]
 assert classifiers,method
 for at in classifiers:
  assert body[at+1].mnemonic=='setne'and body[at+3].mnemonic=='jne';reject=body[at+3].operands[0].imm
  assert all(not reaches(body,reject,body[w].address)for w in writes),method
for method,klass,parser,endpoint in [('HandleCastVisualCommand','cast','@H@','?PlaySpellVisual@Unit@@'),('HandleCastOrphanVisualCommand','cast','@H@','?PlayOrphanSpellVisual@Unit@@'),('HandleModifyHonorCommand','modify','@H@','?RewardHonor@Player@@'),('HandleModifyRepCommand','modify','@H@','?ModifyReputation@ReputationMgr@@')]:
 body=ins('?'+method+'@'+klass+'_commandscript@@');parse,=sites(body,'??$ParseCommandInteger'+parser+'Trinity@@');writes=sites(body,endpoint);assert writes
 assert body[parse+1].mnemonic=='test'and body[parse+2].mnemonic in ('je','jne'),method
 reject=body[parse+2].operands[0].imm if body[parse+2].mnemonic=='je'else body[parse+3].address
 assert all(not reaches(body,reject,body[w].address)for w in writes),method
honor=ins('?HandleModifyHonorCommand@modify_commandscript@@');reward,=sites(honor,'?RewardHonor@Player@@');assert honor[reward+1].op_str=='al, al'and honor[reward+2].mnemonic=='je'
for at in sites(honor,'??$PSendSysMessage@'):assert not reaches(honor,honor[reward+2].operands[0].imm,honor[at].address)
custom=ins('?HandleCastCustomCommand@cast_commandscript@@');assert sites(custom,'?CastCustomSpell@Unit@@');assert any(x.mnemonic=='cvttsd2si'for x in custom)or sites(custom,'?ParseCommandDouble@Trinity@@')or any('HandleCastCustomCommand' in k and 'lambda' in k for k in n['symbols'])
guids=ins('?GetGuidLowFromArgsOrLastTargetedGo@gobject_commandscript@@');assert sites(guids,'??$ParseCommandInteger@I@Trinity@@')or any(o.type==CS_OP_IMM and o.imm==0x19999999 for x in guids for o in x.operands),'Native UInt32 parse inline bound required'
if len(PLAN['changes'])>=20:
 mount=ins('?HandleModifyMountCommand@modify_commandscript@@');assert sites(mount,'?SetFlag@Object@@')and not sites(mount,'?SetUInt32Value@Object@@'),'Mount must preserve other flags'
 for method,klass in [('HandleGameObjectNearCommand','gobject'),('HandleNpcNearCommand','npc')]:
  body=ins('?'+method+'@'+klass+'_commandscript@@');writes=sites(body,'?setFloat@PreparedStatementBase@@');assert writes and any(x.mnemonic=='mulsd'for x in body)and any(x.mnemonic=='comisd'for x in body[:min(writes)])
 state=ins('?HandleGameObjectSetStateCommand@gobject_commandscript@@');assert sites(state,'?SetByteValue@Object@@')and sites(state,'?SendCustomAnim@GameObject@@');assert any(o.type==CS_OP_IMM and o.imm==255 for x in state for o in x.operands)
 damage=ins('?HandleDamageObjectCommand@gobject_commandscript@@');assert sites(damage,'?ModifyHealth@GameObject@@')and any(o.type==CS_OP_IMM and o.imm in (-2147483648,2147483648)for x in damage for o in x.operands)
 add=ins('?HandleHonorAddCommand@honor_commandscript@@');at,=sites(add,'?RewardHonor@Player@@');assert not any(x.mnemonic=='mov'and x.op_str in ('al, 1','eax, 1')for x in add[at+1:])
if len(PLAN['changes'])>=30:
 timer=ins('?HandleBattlefieldTimer@bf_commandscript@@');values={o.imm for x in timer for o in x.operands if o.type==CS_OP_IMM};assert {4294967,1000}<=values and sites(timer,'?SendInitWorldStatesToAll@Battlefield@@'),'Native timer bound/millisecond conversion/broadcast'
 for method in ('HandleAccountAddonCommand','HandleAccountSetAddonCommand'):
  body=ins('?'+method+'@account_commandscript@@');setter,=sites(body,'?setUInt8@PreparedStatementBase@@');assert any(o.type==CS_OP_IMM and o.imm==6 for x in body[:setter]for o in x.operands),'Legion expansion bound precedes native byte write'
print('PASS selected new operations registered operations:finite inputs before cast/GO persistence,native signed visual/honor/reputation parse CFG,rejected honor receipt and checked GO helper;previous50 PE proofs retained under exact declared new source delta. Source/native/client scopes separate;second group checks mount flag preservation,squared-distance bounds,native GO byte/custom widths,damage negation and honor-add receipt when present.')
