"""Check selected native Magnar batch readiness and generation-safe activation."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
from capstone.x86_const import X86_REG_RIP

def check(n,ins):
 suffix='@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
 def method(name):return '?'+name+suffix
 def has(prefix):return any(k.startswith(prefix) for k in n['symbols'])
 def sites(caller,dependency):
  targets={n['resolve'](a) for k,a in n['symbols'].items() if k.startswith(dependency)}
  return [i for i,x in enumerate(ins(caller)) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
 def needs(caller,dependency):assert sites(caller,dependency),caller+' missing '+dependency
 update,roster,launch,activate,death,reset=[method(k) for k in ['UpdateAI','SummonTrash','LaunchEventWave','ActivateWaveActor','SummonedCreatureDies','Reset']]
 reserve=method('UpdateReserve') if has(method('UpdateReserve')) else update
 companion=method('GetWaveCompanion');ready=method('IsWaveActorReady')
 factory=ins('?GetAI@npc_magnar_icebreaker@@')
 ctor='??0npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
 if sites('?GetAI@npc_magnar_icebreaker@@',ctor):factory+=ins(ctor)
 tables=[]
 for x in factory:
  for o in x.operands:
   if x.mnemonic=='lea' and o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP:
    a=x.address+x.size+o.mem.disp;off=n['offset'](a)
    if struct.unpack_from('<Q',n['binary'],off+0x158)[0]==n['symbol'](death):tables.append(off)
 assert len(tables)==1
 table=tables[0]
 assert struct.unpack_from('<Q',n['binary'],table+0x48)[0]==n['symbol'](method('DoAction'))
 assert struct.pack('<Q',n['symbol'](update)) in n['binary'][table:table+0x400]
 needs(update,'?UpdateVictim@CreatureAI@@')
 if reserve==update:before=sites(update,roster)[0]
 else:before=sites(update,reserve)[0]
 assert before<sites(update,'?UpdateVictim@CreatureAI@@')[0]
 for a,b in [(reserve,roster),(reserve,launch),(reserve,activate),(roster,activate),(launch,activate),(activate,companion),(launch,companion),(reset,'?DespawnAll@SummonList@@')]:needs(a,b)
 body=ins(activate);lookups=sites(activate,'?GetCreature@Map@@');flag=sites(activate,'?RemoveFlag@Object@@')[0];aura=sites(activate,'?RemoveAura@Unit@@')[0]
 assert len(lookups)>=4 and lookups[0]<flag<lookups[1]<aura<lookups[2]<lookups[-1], 'Callbacks do not re-resolve actual full GUID'
 virtuals=[i for i,x in enumerate(body) if (x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x10) or (x.mnemonic=='mov' and len(x.operands)==2 and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.disp==0x10 and x.operands[0].size==8)]
 assert len(virtuals)>=2 and lookups[2]<virtuals[-2]<lookups[-1]<virtuals[-1], 'Native two-sided AttackStart lost guarded owner resolution'
 readiness=ins(ready) if has(ready) else body
 assert any(x.mnemonic in ('cmp','test') and any(o.type==CS_OP_MEM and o.mem.disp==0x858 for o in x.operands) for x in readiness),'IsAIEnabled check absent'
 assert any(x.mnemonic in ('cmp','test') and any(o.type==CS_OP_MEM and o.mem.disp==0x9c0 for o in x.operands) for x in readiness),'Actual AI pointer check absent'
 reset_body=ins(reset)
 generations=[o.mem.disp for x in reset_body if x.mnemonic in ('inc','add') for o in x.operands if o.type==CS_OP_MEM and o.size==4 and o.mem.disp>0x1c0]
 assert len(set(generations))==1,'Cannot identify actual reset generation'
 generation=generations[0]
 guards=[i for i,x in enumerate(body) if x.mnemonic=='cmp' and any(o.type==CS_OP_MEM and o.mem.disp==generation for o in x.operands)]
 assert len(guards)>=3 and any(flag<i<lookups[1] for i in guards) and any(aura<i<lookups[2] for i in guards) and any(virtuals[-2]<i<lookups[-1] for i in guards),'Stale activation survives callback reset'
 for name in (roster,reserve,launch,activate):
  for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):assert not sites(name,forbidden),'Recovery grants progression/item/teleport'
 dispatch=sites(death,method('AdvanceWaveIfReady'))[0]
 killer={'r8'}
 for x in ins(death)[:dispatch]:
  if x.mnemonic=='mov' and x.operands[0].type==CS_OP_REG:
   destination=x.reg_name(x.operands[0].reg)
   if x.operands[1].type==CS_OP_REG and x.reg_name(x.operands[1].reg) in killer:killer.add(destination)
   else:killer.discard(destination)
  elif x.mnemonic=='call':killer-={'rax','rcx','rdx','r8','r9','r10','r11'}
 assert 'rdx' in killer,'Actual native death killer lost across pending/active GUID accounting'
 print('PASS selected Magnar factory/update/native activation: actual AI-enabled and pointer guards, same1s pending roster, four full-GUID lookups around flags/aura/AttackStart, reset-generation refusals, native death-to-credit path and no recovery grants. Full two-pass batch/source and native Kill regressions recorded separately.')

if __name__=='__main__':
 p=runpy.run_path(str(Path(__file__).with_name('verify_magnar_finale_release.py')));check(p['native'],p['instructions'])
