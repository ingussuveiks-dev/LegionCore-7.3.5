"""Verify actual owned G'norz receipt, lifecycle credit and instance fallback."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
n=runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins,symbol=n['instructions'],n['symbol']
def sites(body,prefix):
 target=symbol(prefix)
 return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==target]
def locate(body,op,mnemonic=None):return next(i for i,x in enumerate(body) if x.op_str==op and (not mnemonic or x.mnemonic==mnemonic))
def table(factory,callback):
 result=[]
 for x in ins(factory):
  if x.mnemonic=='lea' and len(x.operands)==2 and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP:
   at=x.address+x.size+x.operands[1].mem.disp;slots=struct.unpack_from('<96Q',n['binary'],n['offset'](at))
   if symbol(callback) in slots:result.append(slots)
 assert len(result)==1
 return result[0]
suffix='@npc_travar_firstAI@npc_travar_first@@'
life='?UpdateLifecycle'+suffix;guid='?SetGUID'+suffix;dead='?SummonedCreatureDies'+suffix;registered='?JustSummoned'+suffix
slots=table('?GetAI@npc_travar_first@@',life)
assert slots[0x110//8]==symbol(life) and slots[0x140//8]==symbol(registered) and slots[0x158//8]==symbol(dead) and slots[0x80//8]==symbol(guid)
assert sites(ins(registered),'?Register@State@SilverHandFirstFight@@')
assert sites(ins(dead),'?ObserveDeath@State@SilverHandFirstFight@@')
body=ins(guid)
assert body[0].op_str=='r8d, 0x1a080' and body[1].mnemonic=='jne'
assert sites(body,'?GetCreature@Map@@') and sites(body,'?ObserveDeath@State@SilverHandFirstFight@@')
body=ins('?CanRun@SilverHandFirstFight@@')
for op in ('eax, 0x19fbd','dword ptr [rbx + 0xe0], 0x603','eax, 0x444','eax, 2'):
 i=locate(body,op,'cmp');assert body[i+1].mnemonic=='jne'
for p in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@','?GetCurrentStep@Scenario@@'):assert sites(body,p)
body=ins('?IsOwnedBoss@SilverHandFirstFight@@')
assert sites(body,'?CanRun@SilverHandFirstFight@@')
for op in ('eax, 0x1a080','qword ptr [rbx + 8], rax','dword ptr [rbx + 0x3b8], eax'):
 i=locate(body,op,'cmp');assert body[i+1].mnemonic=='jne'
assert any(x.op_str=='xmm0, xmmword ptr [rbx + 0x3b64]' for x in body)
assert sites(body,'??8ObjectGuid@@'), 'Owner identity is not compared with native full GUID'
body=ins('?ObserveDeath@State@SilverHandFirstFight@@')
assert sites(body,'?IsOwnedBoss@SilverHandFirstFight@@') and sites(body,'?Register@State@SilverHandFirstFight@@')
for op in ('byte ptr [rcx + 0x34], 0','byte ptr [rcx + 0x36], 0','byte ptr [rcx + 0x35], 0','dword ptr [rdi + 0xa18], 0'):assert any(x.op_str==op for x in body)
assert any(x.op_str=='rax, 0x3e8' for x in body) and any(x.op_str=='byte ptr [rbx + 0x35], 1' for x in body)
assert any(x.op_str=='qword ptr [rbx + 0x28], rax' for x in body)
body=ins('?Complete@State@SilverHandFirstFight@@')
quest,=sites(body,'?GetQuestStatus@Player@@');assert body[quest+1].op_str=='eax, 3' and body[quest+2].mnemonic=='je'
next_player=body[quest+3].address
for p,branch in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('?CanContact@Player@@','je'),('?InSamePhase@WorldObject@@','je'),('?IsWithinDistInMap@WorldObject@@','je'),('?GetQuestRewardStatus@Player@@','jne')):
 i,=sites(body,p);assert body[i+1].op_str=='al, al' and body[i+2].mnemonic==branch and body[i+2].operands[0].imm==next_player
for op in ('qword ptr [rdi + 8], rax','dword ptr [rdi + 0x3b8], eax','al, 2'):
 i=locate(body,op,'cmp');assert body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm==next_player
for op in ('byte ptr [rdi + 0x4ff0], 1','byte ptr [rdi + 0x6848], r15b','byte ptr [rdi + 0x6849], r15b'):
 i=locate(body,op);assert body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm==next_player
claim=locate(body,'byte ptr [rsi + 0x36], 1','mov');assert body[quest+2].operands[0].imm==body[claim].address and body[claim+1].op_str=='rax, rdi'
for op in ('byte ptr [rsi + 0x35], 0','qword ptr [rsi + 0x18], rax','bl, bl','byte ptr [r14 + 0x3140], 8'):assert any(x.op_str==op for x in body)
assert not any(x.op_str=='dword ptr [rdi + 0xa18], 0' for x in body), 'Dead eligible participant rejected'
body=ins(life)
need,=sites(body,'?NeedsSpawn@State@SilverHandFirstFight@@');complete,=sites(body,'?Complete@State@SilverHandFirstFight@@');credit,=sites(body,'?UpdateAchievementCriteria@Player@@')
assert need<complete<credit and any(x.op_str=='qword ptr [rbx + 0x198], rsi' for x in body[:need])
assert body[complete+2].op_str=='rax, rax' and body[complete+3].mnemonic=='je' and body[complete+3].operands[0].imm>body[credit].address
assert any(x.op_str=='r8d, 0xc6a2' for x in body[credit-7:credit]) and any(x.op_str=='edx, 0x5c' for x in body[credit-7:credit])
assert not sites(body,'?FindNearestPlayer@WorldObject@@')
spawn='?SpawnFirstFight'+suffix;body=ins(spawn)
summon_targets={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith('?SummonCreature@WorldObject@@')}
assert len(n['calls'](spawn)&summon_targets)==1 and sites(body,'?Spawned@State@SilverHandFirstFight@@')
arm,=sites(body,'?AddDelayedEvent@FunctionProcessor@@');assert body[arm-1].op_str=='edx, 0x7d0'
assert any(x.op_str=='rcx, [rbx + 0xf0]' for x in body[arm-6:arm]) and any(x.op_str=='rcx, qword ptr [rbx + 0x190]' for x in body[arm-12:arm])
assert any(x.op_str=='edx, 0x1a080' for x in body) and any(x.op_str=='r8d, 0x32d81' for x in body)
closures=[name for name in n['symbols'] if name.startswith('?_Do_call@') and 'SpawnFirstFight@npc_travar_firstAI@' in name];assert len(closures)==1
closure=ins(closures[0]);epoch=locate(closure,'rax, qword ptr [rdx + 0x190]','mov')
assert closure[epoch+1].op_str=='qword ptr [rcx + 0x10], rax' and closure[epoch+2].mnemonic=='jne'
ready=locate(closure,'byte ptr [rdx + 0x5c], 1','mov')
assert all(x.operands[0].imm>closure[ready].address for x in closure[:ready] if x.mnemonic in ('je','jne'))
motion='?MovementInform'+suffix
assert slots[0x1a8//8]==symbol(motion) and sites(ins(motion),spawn) and sites(ins(motion),'?CanRun@SilverHandFirstFight@@')
instance='?CreatureDies@instance_silver_hand_InstanceMapScript@';inst_slots=table('?GetInstanceScript@instance_silver_hand@@',instance);slot=inst_slots.index(symbol(instance))
body=ins(instance)
assert sites(body,'?IsOwnedBoss@SilverHandFirstFight@@')
assert sum(x.op_str=='rbx, qword ptr [rax + 0x80]' for x in body)==2
assert any(x.op_str=='rsi, qword ptr [rbx + 0x9c8]' for x in body) and any(x.op_str=='r14, qword ptr [rbx + 0x9c0]' for x in body)
i=locate(body,'rsi, r14','cmp');assert body[i+1].mnemonic=='je'
kill=ins('?Kill@Unit@@');dispatch=locate(kill,f'r9, qword ptr [rcx + {slot*8:#x}]','mov')
assert kill[dispatch-1].op_str=='rcx, qword ptr [rax]'
assert [(x.mnemonic,x.op_str) for x in kill[dispatch+1:dispatch+5]]==[
 ('mov','r8, r13'),('mov','rdx, r15'),('mov','rcx, rax'),('call','r9')]
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x140 for x in ins('?InitSummon@TempSummon@@'))
for method in (guid,dead,registered,instance,'?EnterEvadeMode'+suffix):assert not sites(ins(method),'?UpdateAchievementCriteria@Player@@')
print('PASS actual first-fight selected callbacks/owned full-GUID death/native instance fallback, primary+suspended delivery,1092/2/active42120 participant,1s death deadline+2s epoch guard,one lifecycle50850 after claim. No evade/spawn/registration credit; client movement/choreography remains separate.')
