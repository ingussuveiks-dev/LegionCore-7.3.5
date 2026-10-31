"""Prove selected Kraliss roar uses its native AI EventMap, retaining victory."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_kraliss_release.py')))
n=retained['n'];instructions=n['instructions'];sites=retained['sites'];values=retained['values']
table=retained['table'];suffix='@npc_kralissAI@npc_kraliss@@'
for method,slot in [('UpdateAI',0x18),('Reset',0x28)]:
 assert struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?'+method+suffix)
combat=instructions('?EnterCombat'+suffix)
combat_slots=[at for at in range(0,0x290,8) if struct.unpack_from('<Q',n['binary'],table+at)[0]==n['symbol']('?EnterCombat'+suffix)]
assert len(combat_slots)==1
update=instructions('?UpdateAI'+suffix);reset=instructions('?Reset'+suffix)
def scheduled(body):
 result=[]
 for at in sites(body,'?ScheduleEvent@EventMap@@'):
  previous=max((i for i,x in enumerate(body[:at]) if x.mnemonic=='call'),default=-1)
  args={}
  for x in body[previous+1:at]:
   if x.mnemonic=='mov' and x.operands[1].type==CS_OP_IMM and x.op_str.split(',')[0] in ('edx','r8d'):
    args[x.op_str.split(',')[0]]=x.operands[1].imm
  result.append((args['edx'],args['r8d']))
 return result
assert scheduled(combat)==[(1,18000),(2,20000)]
assert scheduled(update)==[(2,20000),(3,1000),(1,18000)]
assert len(sites(update,'?CancelEvent@EventMap@@'))==3
victim,=sites(update,'?UpdateVictim@CreatureAI@@')
execute,=sites(update,'?ExecuteEvent@EventMap@@')
assert victim<execute
assert any(x.mnemonic=='add' and x.op_str=='dword ptr [rbx + 0x38], esi' for x in update[victim:execute])
assert any(x.mnemonic=='test' and x.op_str=='dword ptr [rax + 0x3140], 0x8000' for x in update[victim:execute])
cast,=sites(update,'?DoCast@UnitAI@@QEAAXI@Z')
# ExecuteEvent result is reduced twice, then only original event3 reaches roar.
assert [(x.mnemonic,x.op_str) for x in update[cast-8:cast]]==[
 ('sub','eax, 1'),('je',hex(update[cast-7].operands[0].imm)),
 ('sub','eax, 1'),('je',hex(update[cast-5].operands[0].imm)),
 ('cmp','eax, 1'),('jne',hex(update[cast-3].operands[0].imm)),
 ('mov','edx, 0x33354'),('mov','rcx, rbx')]
talk,=sites(update,'?Talk@CreatureAI@@')
assert talk<sites(update,'?ScheduleEvent@EventMap@@')[1]
melee,=sites(update,'?DoMeleeAttackIfReady@UnitAI@@')
assert update[cast-7].operands[0].imm==update[talk-5].address
assert update[cast-5].operands[0].imm==update[cast+2].address
assert update[cast-3].operands[0].imm==update[melee-2].address
assert {209764,209755}<=values(update) and sites(update,'?SelectTarget@UnitAI@@')
assert reset[0].mnemonic=='add' and reset[0].op_str=='rcx, 0x38' and sites(reset,'?Reset@EventMap@@')==[1]
for body in (combat,update,reset):
 for prefix in ('?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@','?AddFunction@FunctionProcessor@@','?KillAllFunctions@FunctionProcessor@@','?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
  assert not sites(body,prefix)
print('PASS selected actual Kraliss factory/vtable UpdateAI/Reset/EnterCombat: native AI EventMap38 owns18s speech/1s event3 roar209748,20s Wreck targets and reset; native victim/casting guards precede dispatch. No Unit/FunctionProcessor raw-AI callback/global queue clear/progression grant; prior native victory/owner/quest/wall/heal/rescue PE retained. Client castbar/audio/combat/full1066 remain separate')
