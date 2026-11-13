"""Selected first40 registered/native paths,including persistent NPC commands."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from ashbringer_storage_pe import reaches
p=runpy.run_path(str(Path(__file__).with_name('verify_third_five_ten_20261114_release.py')))
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs']
registry=refs(ins('?GetCommands@npc_commandscript@@'))
methods=('SetLevel','SetFactionId','SetFlag','SetData','PlayEmote','SetModel','SetPhase','SetSpawnDist','SetSpawnTime','SetSize')
for name in methods:assert n['symbol']('?HandleNpc'+name+'Command@npc_commandscript@@')in registry,name
for name in methods:
 body=ins('?HandleNpc'+name+'Command@npc_commandscript@@')
 if name in ('SetSize','SetSpawnDist'):
  finite,=[i for i,x in enumerate(body)if x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm==0x7f800000 for o in x.operands)]
  assert body[finite+1].mnemonic=='setne'and body[finite+3].mnemonic=='jne';reject=body[finite+3].operands[0].imm
  setters=sites(body,'?SetRespawnRadius@Creature@@')+sites(body,'?SaveToDB@Creature@@')+sites(body,'?setFloat@PreparedStatementBase@@')
  assert setters and all(not reaches(body,reject,body[i].address)for i in setters),name
 else:
  assert sites(body,'??$ParseCommandInteger@')or any(x.mnemonic=='cmp'and any(o.type==CS_OP_IMM and o.imm in (0x19,0x1999)for o in x.operands)for x in body)or any(x.mnemonic=='cmp'and x.op_str=='di, r10w'for x in body),name
  for at in sites(body,'??$ParseCommandInteger@'):
   assert body[at+1].mnemonic=='test'and body[at+2].mnemonic in ('je','jne'),name
   branch=body[at+2];reject=branch.operands[0].imm if branch.mnemonic=='je'else body[at+3].address
   setters=sites(body,'?SaveToDB@Creature@@')+sites(body,'?SetUInt32Value@Object@@')+sites(body,'?SetPhaseMask@WorldObject@@')
   assert all(not reaches(body,reject,body[i].address)for i in setters),name
for name in ('SetSpawnDist','SetSpawnTime'):
 body=ins('?HandleNpc'+name+'Command@npc_commandscript@@')
 # Native prepared-statement setUInt64 requires the full GUID in r8.
 setter,=sites(body,'?setUInt64@PreparedStatementBase@@')
 assert body[setter-3].mnemonic=='mov'and body[setter-3].op_str in ('r8, rbp','r8, rdi'),'Native full-width GUID operand required'
body=ins('?HandleNpcSetDataCommand@npc_commandscript@@')
assert any(x.mnemonic=='test'and x.op_str in ('rax, rax','rcx, rcx')and body[i+1].mnemonic=='je'for i,x in enumerate(body[:-1])),'Native AI pointer guard retained'
print('PASS first40 selected registered paths;native NPC finite/typed guards before persistence,full64-bit prepared spawn GUIDs and AI admission;source/native/client scopes separate.')
