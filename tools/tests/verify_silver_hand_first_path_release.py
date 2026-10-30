"""Verify the selected first-Travard path receipt and native50766 handoff."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p=runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_first_fight_release.py')))
n,ins,symbol,sites,locate=p['n'],p['ins'],p['symbol'],p['sites'],p['locate']
suffix='@npc_travar_firstAI@npc_travar_first@@'
for name,slot in (('?UpdateAI',0x18),('?MovementInform',0x1a8),('?sGossipSelect',0xc8)):
 assert p['slots'][slot//8]==symbol(name+suffix)
body=ins('?Context@SilverHandFirstPath@@')
for op,branch in (('dword ptr [rbx + 0xa18], 0','jne'),('eax, 0x19fbd','jne'),('dword ptr [rbx + 0xe0], 0x603','jne'),('qword ptr [rbx + 8], 0','je'),('eax, 0x444','je')):
 i=locate(body,op,'cmp');assert body[i+1].mnemonic==branch
for name in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@'):assert sites(body,name)
body=ins('?sGossipSelect'+suffix)
claim=locate(body,'byte ptr [rdi + 0x1cc], al','mov');remove,=sites(body,'?RemoveFlag@Object@@');move,=sites(body,'?MovePath@MotionMaster@@')
assert claim<remove<move and [(x.mnemonic,x.op_str) for x in body[claim-2:claim]]==[('cmp','eax, 1'),('jne',body[claim-1].op_str)]
assert body[claim-1].operands[0].imm>body[move].address
i=locate(body,'byte ptr [rdi + 0x1cc], 0','cmp');assert i<claim and body[i+1].mnemonic=='jne'
assert sites(body,'?Context@SilverHandFirstPath@@')[0]<claim
body=ins('?MovementInform'+suffix)
observe=locate(body,'byte ptr [rdi + 0x1cd], 1','mov');handoff,=sites(body,'?CompleteFirstPath'+suffix)
for op in ('esi, 2','byte ptr [rdi + 0x5a], 0','ebx, 0xa','byte ptr [rdi + 0x1cc], 0'):
 assert locate(body,op,'cmp')<observe
assert observe<handoff and not any(x.op_str=='r8d, 0xc64e' for x in body)
body=ins('?Complete@State@SilverHandFirstPath@@')
credit,=sites(body,'?UpdateAchievementCriteria@Player@@')
for op,branch in (('byte ptr [rcx + 0x15], 0','je'),('byte ptr [rcx + 0x16], 0','jne'),('byte ptr [rcx + 0x17], 0','jne'),('dword ptr [r14 + 0x10], 0x1e','je')):
 i=locate(body,op,'cmp');assert i<credit and body[i+1].mnemonic==branch
assert sites(body,'?PathAvailable@State@SilverHandFirstPath@@')[0]<credit
for op in ('qword ptr [r14], rax','al, 2','qword ptr [rdi + 8], rax','dword ptr [rdi + 0x3b8], eax','eax, 3'):
 i=locate(body,op,'cmp');assert i<credit
quest,=sites(body,'?GetQuestStatus@Player@@');assert body[quest+2].mnemonic=='jne'
reject=body[quest+2].operands[0].imm
for name,branch in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('?CanContact@Player@@','je'),('?InSamePhase@WorldObject@@','je'),('?IsWithinDistInMap@WorldObject@@','je'),('?GetQuestRewardStatus@Player@@','jne')):
 i=sites(body,name)[0];assert i<credit and body[i+2].mnemonic==branch and body[i+2].operands[0].imm==reject
for op in ('byte ptr [rdi + 0x4ff0], 1','byte ptr [rdi + 0x6848], r15b','byte ptr [rdi + 0x6849], r15b'):
 i=locate(body,op);assert body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm==reject
assert not sites(body,'?FindNearestPlayer@WorldObject@@') and not any(x.op_str=='dword ptr [rdi + 0xa18], 0' for x in body)
busy=locate(body,'byte ptr [r14 + 0x17], 1','mov');done=locate(body,'byte ptr [r14 + 0x16], 1','mov')
assert busy<credit<done and body[credit+1].op_str=='byte ptr [r14 + 0x17], r15b'
for op in ('edx, 0x5c','r8d, 0xc64e'):assert any(x.op_str==op for x in body[credit-6:credit])
assert any(x.mnemonic=='cmp' and x.op_str=='eax, 2' and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm==body[done].address for i,x in enumerate(body[credit:done],credit))
assert any(x.op_str=='rax, 0x3e8' for x in body[:credit])
body=ins('?CompleteFirstPath'+suffix)
complete,=sites(body,'?Complete@State@SilverHandFirstPath@@');claim=locate(body,'word ptr [rdi + 0x5a], 0x100','mov');talk,=sites(body,'?Talk@CreatureAI@@QEAAXE')
assert complete<claim<talk and body[complete+2].mnemonic=='je' and body[complete+2].operands[0].imm>body[talk].address
assert locate(body,'byte ptr [rdx + 0x858], 0','cmp')<complete and locate(body,'qword ptr [rdx + 0x9c0], rcx','cmp')<complete
calculate=sites(body,'?CalculateTime@FunctionProcessor@@');add=sites(body,'?AddFunction@FunctionProcessor@@');assert len(calculate)==len(add)==2
for i,delay in zip(calculate,(5000,9000)):assert any(x.op_str==f'edx, {delay:#x}' for x in body[i-3:i])
for index,enqueue in enumerate(add,1):
 begin=calculate[index-1]
 tables=[x.address+x.size+x.operands[1].mem.disp for x in body[begin:enqueue] if x.mnemonic=='lea' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP]
 assert len(tables)==1
 callback=struct.unpack_from('<Q',n['binary'],n['offset'](tables[0])+0x10)[0]
 name,=[s for s,a in n['symbols'].items() if a==callback and s.startswith('?_Do_call@') and f'<lambda_{index}>' in s and 'CompleteFirstPath@npc_travar_firstAI' in s]
 closure=ins(name)
 assert sites(closure,'?Context@SilverHandFirstPath@@') and sites(closure,'?GetCurrentStep@Scenario@@')
 assert any(x.op_str=='eax, 2' for x in closure)
 if index==1:assert any(x.op_str=='dl, 7' for x in closure)
 else:assert any(x.op_str=='byte ptr [rax + 0x1d0], 1' for x in closure) and sites(closure,'?DepartFirstPath'+suffix)
body=ins('?DepartFirstPath'+suffix)
move,=sites(body,'?MovePath@MotionMaster@@');claim=locate(body,'byte ptr [rdi + 0x1d1], 1','mov')
assert sites(body,'?PathAvailable@State@SilverHandFirstPath@@')[0]<claim<move
for op in ('byte ptr [rcx + 0x1d0], 0','byte ptr [rdi + 0x1ce], 0','byte ptr [rdi + 0x1d1], 0','eax, 2'):assert locate(body,op,'cmp')<claim
assert any(x.op_str=='edx, 0xa7c08d' for x in body[move-5:move])
body=ins('?UpdateAI'+suffix)
update,=sites(body,'?Update@FunctionProcessor@@');service,=sites(body,'?CompleteFirstPath'+suffix)
assert update<service and body[update-1].op_str=='rcx, 0x60'
assert any(x.mnemonic=='add' and x.op_str=='qword ptr [rbx + 0x1b8], rax' for x in ins('?UpdateLifecycle'+suffix))
print('PASS actual selected path04 request/waypoint10 receipt, active42120 participant, bounded native50766 dispatch and real step2 before door/narration; original AI-owned5s/9s queues, guarded retryable path05 departure, lifecycle clock and post-queue service. Controlled tests/native data remain distinct from client movement/audio.')
