"""Verify owned first-room choreography and actual point3->50872->step4 cleanup."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
previous=runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_first_path_release.py')))
n,ins,symbol,sites,locate=(previous[k] for k in ('n','ins','symbol','sites','locate'))
suffix='@npc_travar_firstAI@npc_travar_first@@'
assert symbol('?Context@SilverHandFirstRoom@@')==symbol('?Context@SilverHandFirstPath@@'), 'Native identical context guards did not fold as inspected'
body=ins('?MovementInform'+suffix)
cast,=sites(body,'?DoCast@UnitAI@@QEAAXI@Z')
start=locate(body,'byte ptr [rdi + 0x1ec], 1','mov')
assert locate(body,'ebx, byte ptr [rdi + 0x1b6]','movzx')<start<cast
assert locate(body,'byte ptr [rdi + 0x1ec], 0','cmp')<start and body[start-1].mnemonic=='jne'
assert any(x.op_str=='edx, 0x2d446' for x in body[cast-3:cast])
observe=locate(body,'byte ptr [rdi + 0x1ee], 1','mov');complete,=sites(body,'?CompleteFirstRoom'+suffix)
assert observe<complete and locate(body,'byte ptr [rdi + 0x1ed], 0','cmp')<observe
point=locate(body,'esi, 8','cmp');assert body[point+1].mnemonic=='jne'
assert not sites(body,'?FindNearestPlayer@WorldObject@@') and not any(x.op_str=='r8d, 0xc6b8' for x in body)
calculate=sites(body,'?CalculateTime@FunctionProcessor@@');enqueue=sites(body,'?AddFunction@FunctionProcessor@@')
assert len(calculate)==len(enqueue)==2
closures=[]
for index,(time,add,delay) in enumerate(zip(calculate,enqueue,(4000,7000)),1):
 assert any(x.op_str==f'edx, {delay:#x}' for x in body[time-3:time])
 tables=[x.address+x.size+x.operands[1].mem.disp for x in body[time:add] if x.mnemonic=='lea' and x.operands[1].type==CS_OP_MEM and x.operands[1].mem.base==X86_REG_RIP]
 assert len(tables)==1
 callback=struct.unpack_from('<Q',n['binary'],n['offset'](tables[0])+0x10)[0]
 name,=[s for s,a in n['symbols'].items() if a==callback and s.startswith('?_Do_call@') and f'<lambda_{index}>' in s and 'MovementInform@npc_travar_firstAI' in s]
 closures.append(ins(name))
cleanup,depart=closures
for target in ('?RemoveAura@Unit@@QEAAXIVObjectGuid@@','?GetCreatureListWithEntryInGrid@WorldObject@@','?HasAura@Unit@@QEBA_NIVObjectGuid@@0I@Z','?SetVisible@WorldObject@@'):assert sites(cleanup,target)
for op in ('edx, 0x2d446','r8d, 0x19e6e','edx, 0x33a2e','edx, edx','dl, 0x13'):assert any(x.op_str==op for x in cleanup)
issued=locate(depart,'byte ptr [rbx + 0x1ed], 1','mov');talk,=sites(depart,'?Talk@CreatureAI@@QEAAXE');move,=sites(depart,'?MovePoint@MotionMaster@@QEAAXIMMM_NM@Z')
for op in ('byte ptr [rbx + 0x1ec], 0','byte ptr [rbx + 0x1ed], 0','eax, 3'):assert locate(depart,op,'cmp')<issued
assert issued<talk<move and any(x.op_str=='edx, 3' for x in depart[issued:move]) and any(x.op_str=='dl, 0x14' for x in depart[issued:talk])
body=ins('?Complete@State@SilverHandFirstRoom@@')
credit,=sites(body,'?UpdateAchievementCriteria@Player@@')
for op,branch in (('byte ptr [rcx + 0x16], 0','je'),('byte ptr [rcx + 0x17], 0','jne'),('byte ptr [rcx + 0x18], 0','jne'),('dword ptr [r14 + 0x10], 0x1e','je')):
 i=locate(body,op,'cmp');assert i<credit and body[i+1].mnemonic==branch
quest,=sites(body,'?GetQuestStatus@Player@@');assert body[quest+1].op_str=='eax, 3' and body[quest+2].mnemonic=='jne'
reject=body[quest+2].operands[0].imm
for name,branch in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('?CanContact@Player@@','je'),('?InSamePhase@WorldObject@@','je'),('?IsWithinDistInMap@WorldObject@@','je'),('?GetQuestRewardStatus@Player@@','jne')):
 i=sites(body,name)[0];assert i<credit and body[i+2].mnemonic==branch and body[i+2].operands[0].imm==reject
for op in ('byte ptr [rdi + 0x4ff0], 1','byte ptr [rdi + 0x6848], r15b','byte ptr [rdi + 0x6849], r15b','al, 2','qword ptr [rdi + 8], rax','dword ptr [rdi + 0x3b8], eax'):
 i=locate(body,op);assert i<credit and body[i+1].mnemonic=='jne' and body[i+1].operands[0].imm==reject
assert not sites(body,'?FindNearestPlayer@WorldObject@@') and not any(x.op_str=='dword ptr [rdi + 0xa18], 0' for x in body)
busy=locate(body,'byte ptr [r14 + 0x18], 1','mov');done=locate(body,'byte ptr [r14 + 0x17], 1','mov')
assert busy<credit<done and body[credit+1].op_str=='byte ptr [r14 + 0x18], r15b'
for op in ('edx, 0x5c','r8d, 0xc6b8'):assert any(x.op_str==op for x in body[credit-6:credit])
assert any(x.mnemonic=='cmp' and x.op_str=='eax, 4' and body[i+1].mnemonic=='je' and body[i+1].operands[0].imm==body[done].address for i,x in enumerate(body[credit:done],credit))
assert any(x.op_str=='rax, 0x3e8' for x in body[:credit])
body=ins('?CompleteFirstRoom'+suffix)
complete,=sites(body,'?Complete@State@SilverHandFirstRoom@@');find,=sites(body,'?FindNearestCreature@WorldObject@@');despawn,=sites(body,'?DespawnOrUnsummon@Creature@@');gates,=sites(body,'?GetGameObjectListWithEntryInGrid@WorldObject@@')
assert complete<find<despawn<gates and body[complete+2].mnemonic=='je' and body[complete+2].operands[0].imm>body[gates].address
assert locate(body,'byte ptr [rdx + 0x858], 0','cmp')<complete and locate(body,'qword ptr [rdx + 0x9c0], rcx','cmp')<complete
assert any(x.op_str=='edx, 0x18c55' for x in body[find-4:find]) and any(x.op_str=='r8d, 0x3d5d5' for x in body[gates-4:gates])
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xa0 for x in body[gates:])
body=ins('?UpdateAI'+suffix)
queue,=sites(body,'?Update@FunctionProcessor@@');path,=sites(body,'?CompleteFirstPath'+suffix);room,=sites(body,'?CompleteFirstRoom'+suffix)
assert queue<path<room
assert any(x.mnemonic=='add' and x.op_str=='qword ptr [rbx + 0x1d8], rax' for x in ins('?UpdateLifecycle'+suffix))
print('PASS selected first-room POINT2 real-fight/stage3/once guard, original selected4s cleanup/7s issued-point3 closure, native receipt before one50872/real4/claim-before101461+251349 cleanup, actual42120 map participant/finite retry/lifecycle clock/post-queue service. Client movement/visuals remain separate.')
