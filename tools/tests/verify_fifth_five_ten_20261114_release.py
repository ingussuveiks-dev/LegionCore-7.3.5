"""Selected50 registered operations and native travel terrain/result paths."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from ashbringer_storage_pe import reaches
p=runpy.run_path(str(Path(__file__).with_name('verify_fourth_five_ten_20261114_release.py')))
n,ins,sites,refs=p['n'],p['ins'],p['sites'],p['refs'];registry=refs(ins('?GetCommands@go_commandscript@@'))
methods=('Creature','Graveyard','Grid','Object','Taxinode','Trigger','ZoneXY','XYZ','Ticket','Z')
for name in methods:
 prefix='?HandleGo'+name+'Command@go_commandscript@@';assert n['symbol'](prefix)in registry,name;body=ins(prefix)
 if name=='Ticket':assert sites(body,'?TeleportTo@GmTicket@@')and sites(body,'??$ParseCommandInteger@I@Trinity@@');continue
 teleport,=sites(body,'?TeleportTo@Player@@')
 if name!='XYZ':assert not any(x.mnemonic=='mov'and x.op_str in ('al, 1','eax, 1')for x in body[teleport+1:]),'Forced success after native teleport: '+name
 if name in ('Graveyard','Taxinode','Trigger'):
  parse,=sites(body,'??$ParseCommandInteger@I@Trinity@@');assert body[parse+1].mnemonic=='test'and body[parse+2].mnemonic=='je'
  assert not reaches(body,body[parse+2].operands[0].imm,body[teleport].address),name
 if name in ('Creature','Object'):
  parser=ins('??$ParseCommandInteger@_K@Trinity@@')if sites(body,'??$ParseCommandInteger@_K@Trinity@@')else body
  assert any(x.mnemonic=='movabs'and x.operands[-1].type==CS_OP_IMM and x.operands[-1].imm==0x1999999999999999 for x in parser),'Full UInt64 parser bound: '+name

for name in ('Quest','Grid','ZoneXY','XYZ'):
 body=ins('?HandleGo'+name+'Command@go_commandscript@@');height,=sites(body,'?GetHeight@Map@@');recall,=sites(body,'?SaveRecallPosition@Player@@');cleanup,=sites(body,'?CleanupAfterTaxiFlight@Player@@');assert height<recall and height<cleanup
 guards=[];constant_regs=set()
 for i,x in enumerate(body):
  if x.mnemonic in ('movss','movaps')and len(x.operands)==2:
   dst=x.reg_name(x.operands[0].reg);src=x.operands[1];constant=False
   if src.type==CS_OP_MEM and src.mem.base==X86_REG_RIP:
    try:constant=struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+src.mem.disp))[0]==-100000.
    except Exception:pass
   elif src.type!=CS_OP_MEM:constant=x.reg_name(src.reg)in constant_regs
   if constant:constant_regs.add(dst)
   else:constant_regs.discard(dst)
  if x.mnemonic in ('comiss','ucomiss')and any(x.reg_name(o.reg)in constant_regs for o in x.operands if o.type!=CS_OP_MEM):guards.append(i)
 assert guards,'Native INVALID_HEIGHT ground boundary: '+name
 for at in guards:
  branch=body[at+1];assert branch.mnemonic in ('jbe','ja','jae','jb')
  reject=branch.operands[0].imm if branch.mnemonic in ('jbe','jae')else body[at+2].address
  if name!='Quest':assert not reaches(body,reject,body[recall].address)and not reaches(body,reject,body[cleanup].address),name
print('PASS50 selected registered operations/native core;strict full-width travel identifiers,retained native teleport results and actual INVALID_HEIGHT ground gates before recall/taxi cleanup. Quest skips bad POIs and may continue to a legitimate next point. Source ASAN/native catalog/client scopes separate.')
