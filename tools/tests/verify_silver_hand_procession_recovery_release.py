"""Verify selected original-GUID procession retry and native phase ordering."""
from pathlib import Path
import runpy
import struct
previous=runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_second_gossip_release.py')))
native,instructions,sites,reaches=(previous[k] for k in ('native','instructions','sites','reaches'))
def locate(body,op,mnemonic=None):return next(i for i,x in enumerate(body) if x.op_str==op and (not mnemonic or x.mnemonic==mnemonic))
suffix='@npc_travar_secondAI@npc_travar_second@@'
movement='?MovementInform'+suffix;life='?UpdateLifecycle'+suffix;consumer='?CompleteProcession'+suffix
table,=previous['tables']
for method,slot in ((movement,0x1a8),(life,0x110)):
 assert struct.unpack_from('<Q',native['binary'],native['offset'](table)+slot)[0]==native['symbol'](method)
factory=instructions('?GetAI@npc_travar_second@@')
constructor,=sites(factory,'??0ObjectGuid@@QEAA@XZ');assert factory[constructor-1].op_str=='rcx, [rsi + 0xe0]'
for op in ('qword ptr [rsi + 0xf0], rbp','qword ptr [rsi + 0xf8], rbp','qword ptr [rsi + 0x100], rbp','byte ptr [rsi + 0x108], bpl'):assert any(x.op_str==op for x in factory)
body=instructions('?Context@SilverHandProcessionRecovery@@')
for op,branch in (('dword ptr [rbx + 0xa18], 0','jne'),('eax, 0x1a08f','jne'),('dword ptr [rbx + 0xe0], 0x603','jne'),('qword ptr [rbx + 8], 0','je'),('eax, 0x444','je')):
 i=locate(body,op,'cmp');assert body[i+1].mnemonic==branch
for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@'):assert sites(body,target)
finish='?Finish@State@SilverHandProcessionRecovery@@'
body=instructions(finish)
clear,=sites(body,'?Clear@ObjectGuid@@');step,=sites(body,'?GetCurrentStep@Scenario@@');done=locate(body,'byte ptr [rbx + 0x15], 1','mov')
assert body[step+1].op_str=='eax, 5' and body[step+2].mnemonic=='jne' and step<done<clear
for op,branch in (('byte ptr [rbx + 0x14], 0','je'),('byte ptr [rbx + 0x15], 0','jne')):
 i=locate(body,op,'cmp');assert i<done
 if op=='byte ptr [rbx + 0x14], 0':
  assert body[i+1].mnemonic=='mov' and body[i+1].op_str=='rdi, rax' and body[i+2].mnemonic==branch
 else:assert body[i+1].mnemonic==branch
assert sites(body,'?IsEmpty@ObjectGuid@@') and not sites(body,'?UpdateAchievementCriteria@Player@@')
body=instructions(movement)
observe=locate(body,'byte ptr [rdi + 0x104], 1','mov');complete,=sites(body,consumer)
for op in ('ebx, 2','esi, 0xe'):
 i=locate(body,op,'cmp');assert i<observe and body[i+1].mnemonic=='jne' and not reaches(body,body[i+1].operands[0].imm,body[observe].address)
empty,=sites(body,'?IsEmpty@ObjectGuid@@');assert body[empty-2].op_str=='rcx, [rdi + 0xe0]' and body[empty-1].mnemonic=='mov' and body[empty-1].op_str=='rbx, rax'
assert empty<observe<complete and sites(body,'?Context@SilverHandProcessionRecovery@@')
assert not sites(body,'?UpdateAchievementCriteria@Player@@') and not sites(body,'?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z')
body=instructions(consumer)
lookup=sites(body,'?GetPlayer@ObjectAccessor@@SAPEAVPlayer@@AEBVWorldObject@@VObjectGuid@@@Z');valid=sites(body,'?CanArrive@SilverHandProcession@@');talk,=sites(body,'?Talk@CreatureAI@@QEAAXE');credit,=sites(body,'?UpdateAchievementCriteria@Player@@');finishes=sites(body,finish)
assert len(lookup)==len(valid)==2 and len(finishes)==3
assert finishes[0]<lookup[0]<valid[0]<talk<finishes[1]<lookup[1]<valid[1]<credit<finishes[2]
assert not sites(body,'?Clear@ObjectGuid@@') and not sites(body,'?FindNearestPlayer@WorldObject@@')
assert locate(body,'r14, [rcx + 0xe0]','lea')<lookup[0]
for index in lookup:
 assert any(x.op_str=='xmm0, xmmword ptr [r14]' for x in body[index-5:index])
 assert any(x.op_str=='rdx, [rsp + 0x80]' for x in body[index-5:index])
busy=locate(body,'byte ptr [rbx + 0x106], 1','mov');spoken=locate(body,'byte ptr [rbx + 0x107], 1','mov')
assert valid[0]<busy<spoken<talk and any(x.op_str=='dl, 5' for x in body[talk-5:talk])
i=locate(body,'byte ptr [rbx + 0x107], 0','cmp');assert body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm>body[talk].address
for op,branch in (('byte ptr [rsi + 0x14], 0','je'),('byte ptr [rsi + 0x15], 0','jne'),('byte ptr [rsi + 0x16], 0','jne'),('dword ptr [rsi + 0x10], 0x1e','jae'),('qword ptr [rsi], rax','jb')):
 i=locate(body,op,'cmp');assert i<lookup[0] and body[i+1].mnemonic==branch
for begin,end,player in ((lookup[0],busy,'rdi'),(lookup[1],credit,'r15')):
 policy=next(i for i in valid if begin<i<end);assert body[policy+2].mnemonic=='je'
 guard=locate(body,f'byte ptr [{player} + 0x4ff0], 1','test');assert policy<guard<end and body[guard+1].mnemonic=='jne'
for op in ('byte ptr [rcx + 0x858], 0','qword ptr [rcx + 0x9c0], rbx'):
 guards=[i for i,x in enumerate(body) if x.mnemonic=='cmp' and x.op_str==op]
 assert len(guards)==2 and guards[0]<lookup[0] and talk<guards[1]<lookup[1]
for op in ('edx, 0x5c','r8d, 0xc6d1','rcx, r15'):assert any(x.op_str==op for x in body[credit-8:credit])
assert any(x.op_str=='rax, 0x3e8' for x in body[valid[1]:credit]) and any(x.op_str=='qword ptr [rbx + 0xf8], rax' for x in body[valid[1]:credit])
assert body[credit+1].op_str=='byte ptr [rbx + 0x106], dil'
body=instructions(life);recover,=sites(body,'?Recover@SilverHandCompanion@@');complete=len(body)-1
assert body[complete].mnemonic=='jmp' and native['resolve'](body[complete].operands[0].imm)==native['symbol'](consumer)
clock=locate(body,'qword ptr [rdi + 0xf0], rbx','add');assert recover<clock<complete and any(x.mnemonic=='mov' and x.op_str=='ebx, edx' for x in body[:recover])
body=instructions('?sGossipSelect'+suffix)
empty,=sites(body,'?IsEmpty@ObjectGuid@@');remove,=sites(body,'?RemoveFlag@Object@@');move,=sites(body,'?MovePath@MotionMaster@@');guid,=sites(body,'?GetGuidValue@Object@@')
assert body[empty-1].op_str=='rcx, [rdi + 0xe0]' and body[empty+2].mnemonic=='je' and not reaches(body,body[empty+2].operands[0].imm,body[remove].address)
guard=locate(body,'byte ptr [rdi + 0x105], 0','cmp');assert guard<remove and body[guard+1].mnemonic=='jne' and not reaches(body,body[guard+1].operands[0].imm,body[remove].address)
assert remove<guid<move and body[guid-1].op_str=='rcx, r14' and body[guid+1].op_str=='xmm0, xmmword ptr [rax]' and body[guid+2].op_str=='xmmword ptr [rdi + 0xe0], xmm0'
waypoint=instructions('?OnArrived@?$WaypointMovementGenerator@VCreature@@');dispatch=locate(waypoint,'r9, qword ptr [r8 + 0x1a8]','mov')
assert waypoint[dispatch-1].op_str=='r8, qword ptr [rcx]' and waypoint[dispatch+1].op_str=='r8d, dword ptr [r15]' and waypoint[dispatch+2].op_str=='edx, 2' and waypoint[dispatch+3].op_str=='r9'
update=instructions('?DoUpdate@?$WaypointMovementGenerator@VCreature@@');arrival,=sites(update,'?OnArrived@?$WaypointMovementGenerator@VCreature@@')
assert update[arrival-4].op_str=='byte ptr [rax + 0x68], 0x20' and update[arrival-3].mnemonic=='je' and not reaches(update,update[arrival-3].operands[0].imm,update[arrival].address)
print('PASS selected native second procession: real waypoint14 retains original fullGUID0xe0; lifecycle retry clock0xf0, ready/busy/30-attempt/GM guards, one Talk5 before phase event, fresh owner relookup after speech, one50897 and real5 before completed/clear. Original checked gossip/followers/path and native finalized waypoint dispatch retained. No nearby substitute or client route/rendering proof.')
