from linked_player_abi import inherited_operand,inherited_member_offset
from linked_player_abi import player_offset,player_operand
"""Selected registered real-death receipt and scoped Hait victory path in PE.

Run verify_monk_chen_routes_release.py separately for the retained full chain.
This inspector independently checks the new instance, native death boundary,
real criteria leaf and authenticated arrival; it cannot certify client visuals.
"""
from pathlib import Path
from monk_intro_instance_pe import construction,zero_alias_pairs,reachable
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_czi_timers_release.py')))
n,ins,refs,table,resolved=(p[k] for k in ('n','ins','refs','table','resolved'))
def state(name):return '?'+name+'@State@MonkJorvinax@@'
def context(name):return '?'+name+'@State@MonkChenRescue@@'
def instance(name):return '?'+name+'@instance_monk_intro@@'
def pairs(body):return zero_alias_pairs(body) if body is factory and nine_successor else [(o.mnemonic,o.op_str) for o in body]
def needs(body,*targets):
    for target in targets:assert sites(n,body,target),('Jorvinax native dependency',target)

allocation,factory,nine_successor=construction(n)
vt,raw=table('??_7instance_monk_intro@@6B@',140)
presentation_successor=0x430 in values(allocation) or nine_successor
instance_size=n.get('monk_instance_size',0x498) if n.get('monk_initial_successor') else 0x490 if nine_successor else 0x430 if presentation_successor else 0x418
assert vt in refs(factory) and instance_size in values(allocation)
if presentation_successor:
    presentation_zero = ('mov','qword ptr [rsi + 0x428], rbx')
    if presentation_zero not in pairs(factory):
        presentation_zero = ('mov','dword ptr [rsi + 0x428], ebx') if sites(n,ins('?Update@instance_monk_intro@@'),'?UpdateRescue@State@MonkChenPresentation@@') else ('mov','word ptr [rsi + 0x428], bx')
    assert {('mov','qword ptr [rsi + 0x418], rdi'),('mov','qword ptr [rsi + 0x420], rbx'),presentation_zero}<=set(pairs(factory))
registration,script=table('??_7?$GenericInstanceMapScript@Uinstance_monk_intro@@@@6BScriptObject@@@',2)
assert registration in refs(ins('?AddSC_instance_class_art_scenarios@@'))
assert n['symbol']('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@')==resolved(script)[1]
assert {('lea','rdi, [rsi + 0x2e0]'),('mov','word ptr [rdi + 0xd4], bx'),
        ('mov','qword ptr [rsi + 0x3b8], rdi'),('lea','rcx, [rsi + 0x3c0]'),
        ('lea','rcx, [rsi + 0x3d0]'),('mov','qword ptr [rsi + 0x3e0], rbx'),
        ('mov','qword ptr [rsi + 0x3f8], rbx'),('mov','qword ptr [rsi + 0x408], rbx'),
        ('mov','word ptr [rsi + 0x410], bx'),('mov','byte ptr [rsi + 0x412], bl')}<=set(pairs(factory))
for name,target in (('CreatureDies','Death'),('Update','Update')):
    body=ins(instance(name));assert n['symbol'](instance(name)) in resolved(raw)
    before,=sites(n,body,context(target));after,=sites(n,body,state(target))
    if n.get('monk_initial_successor'):
        assert before<after
        for call,offset in [(before,0x2e0),(after,0x3b8)]:
            arguments=[]
            for index,op in enumerate(body[:16]):
                if op.mnemonic!='mov' or len(op.operands)!=2 or op.op_str.split(', ')[-1]!='rcx' or op.op_str.split(', ')[0] not in ('rbx','rbp','rsi','rdi','r12','r13','r14','r15'):continue
                register=op.operands[0].reg;alias=op.reg_name(register)
                for at,argument in enumerate(body[max(0,call-20):call],max(0,call-20)):
                    if (argument.mnemonic,argument.op_str)!=('lea',f'rcx, [{alias} + {offset:#x}]'):continue
                    if any(register in other.regs_access()[1] for other in body[index+1:at]):continue
                    rcx=argument.operands[0].reg
                    if any(rcx in other.regs_access()[1] for other in body[at+1:call]):continue
                    arguments.append(at)
            assert len(arguments)==1,'Native state argument must retain the original instance receiver through the field-address instruction and call, including tail epilogues'
    else:
        context_argument=('lea','rcx, [rbx + 0x2e0]') if presentation_successor and name=='Update' else ('add','rcx, 0x2e0')
        assert before<after and context_argument in pairs(body[:before])
        assert ('lea','rcx, [rbx + 0x3b8]') in pairs(body[before:after])

death=ins(state('Death'));update=ins(state('Update'));ready=ins(state('Ready'))
leaf=ins(state('Leaf'));guide=ins(state('Guide'));bound=ins(state('BoundRoute'))
needs(death,context('Progress'),'?GetCurrentStep@Scenario@@','?GetCreature@Map@@','?IsInWorld@Object@@',
      '?IsPreDelete@Object@@','??8ObjectGuid@@','?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@',
      '?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@',
      '?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@','?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',state('Leaf'),state('Update'))
assert {1014,98217,362203,12103,6,10,25}<=values(death)
assert {('cmp','qword ptr [rbp + 8], rcx'),('cmp','dword ptr [rbp + 0xa18], edi'),
        ('cmp','rax, rbp'),('cmp','byte ptr [r15 + 0x54], dil'),('cmp','byte ptr [r15 + 0x55], dil'),
        ('mov','byte ptr [rax + 0xd5], 1'),('lock inc','dword ptr [rax + 0xc]')}<=set(pairs(death))
assert sites(n,death,'?AddQuestDelayedEvent@Player@@')[0]<sites(n,death,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')[0]<sites(n,death,state('Update'))[0]
assert not sites(n,death,'?UpdateAchievementCriteria@Player@@'),'Native AI delivery is observed before retry, never blindly replayed in Death'
needs(leaf,context('Progress'),'?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
assert 29071 in values(leaf) and ('cmp','qword ptr [rcx + 0x28], 0') in pairs(leaf)
needs(ready,context('Retained'),'?IsInWorld@Object@@','?CanContact@Player@@','?GetCreature@Map@@',
      '?IsPreDelete@Object@@','?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
assert 100.0 in floats(n,ready) and {inherited_member_offset(0x4ff0),player_offset(0x6848),player_offset(0x6849),player_offset(0x6851)}<=fields(ready)
assert not sites(n,ready,context('Current')),'Victory/voice must not use distant Chen as its player anchor'
assert {1014,97778,362271}<=values(guide)
needs(guide,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@')
needs(bound,context('Retained'),'??8ObjectGuid@@','?IsEmpty@ObjectGuid@@')
assert {('cmp','byte ptr [rcx + 0x93], 0'),('cmp','byte ptr [rcx + 0x95], 0'),('cmp','byte ptr [rcx + 0xce], 0')}<=set(pairs(bound))
assert {('add','rcx, 0xad'),('add','rdx, 0xad')}<=set(pairs(bound))

needs(update,state('Guide'),context('Progress'),context('Retained'),state('BoundRoute'),'?Pose@MonkJorvinax@@',
      '?GetMotionSlot@MotionMaster@@','?SetHomePosition@Creature@@',state('Ready'),state('Leaf'),
      '?GetCurrentStep@Scenario@@','?getMSTime@@','?GetUnit@ObjectAccessor@@','?UpdateAchievementCriteria@Player@@','?SendChat@CreatureTextMgr@@','?outMessage@Log@@')
assert {9859006,8,5,6,2000,46951,92}<=values(update)
event,=sites(n,update,'?UpdateAchievementCriteria@Player@@');chat,=sites(n,update,'?SendChat@CreatureTextMgr@@')
assert {('mov','edx, 0x5c'),('mov','r8d, 0xb767'),('mov','byte ptr [rsp + 0x30], 1')}<=set(pairs(update[event-12:event]))
assert {('cmp','eax, 5'),('cmp','ecx, 0x7d0')}<=set(pairs(update[:event]))
assert sites(n,update,state('Leaf'))[0]<event<sites(n,update,state('Leaf'))[1]<chat
assert {('cmp','byte ptr [r14 + 0x54], 0'),('cmp','qword ptr [r14 + 0x40], rbx')}<=set(pairs(update[event+1:event+8]))
assert {('mov','word ptr [r14 + 0x54], 0x100'),('mov','byte ptr [rax + 0xd5], 1')}<=set(pairs(update[event:chat]))
reserve=next(i for i,o in enumerate(update) if (o.mnemonic,o.op_str)==('mov','byte ptr [rdi + 0x59], 1') and i>sites(n,update,state('Ready'))[-1])
assert reserve<chat and ('mov','r8b, 0xf') in pairs(update[reserve:chat])
assert ('cmp',inherited_operand('dword ptr [rsi + 0x3b1c], 8')) in pairs(update)
assert ('cmp','dword ptr [rbx + 0x2c], 0x966fbe') in pairs(update)
pose=ins('?Pose@MonkJorvinax@@');needs(pose,'?GetPathScript@WaypointMgr@@','?IsWithinDist3d@WorldObject@@')
assert 9859006 in values(pose) and 0.5 in floats(n,pose)
assert {('sub','rcx, 0x18c'),('cmp','rcx, 0x2c')}<=set(pairs(pose))

request=ins(instance('SetGuidData'));assert 94350 in values(request)
needs(request,state('Guide'),state('BoundRoute'),'?Pose@MonkJorvinax@@',state('Update'),'??9ObjectGuid@@')
assert {('cmp','dword ptr [rbx + 0x2c], 0x966fbe'),('cmp','dword ptr [rbx + 0x18], 8'),
        ('cmp','qword ptr [rdi + rax*8 + 0x2f98], rbx'),('mov',f'word ptr [{"rbp" if nine_successor else "rsi"} + 0x57], 1')}<=set(pairs(request))
if nine_successor:
    rp=pairs(request);base=rp.index(('mov','rbp, rcx'));anchor=rp.index(('add','rbp, 0x3b8'));flag=rp.index(('mov','word ptr [rbp + 0x57], 1'))
    assert request[anchor].address not in reachable(request,request[0].address,{request[base].address})
    assert request[flag].address not in reachable(request,request[0].address,{request[anchor].address})
    assert not any(o.op_str.startswith('rbp, ') and o.mnemonic not in ('cmp','test') for o in request[base+1:anchor]+request[anchor+1:flag])
movement=ins('?MovementInform@npc_master_haitAI@npc_master_hait@@')
assert {1014,97778,362271,94350,46951}<=values(movement)
bridge=next(i for i,o in enumerate(movement) if (o.mnemonic,o.op_str)==('mov','edx, 0x1708e'))
old,=sites(n,movement,'?UpdateAchievementCriteria@Player@@')
assert movement[bridge+2].mnemonic=='call' and movement[bridge+3].mnemonic=='jmp'
assert movement[bridge+3].operands[0].imm>movement[old].address,'Scoped arrival cannot fall through to legacy victory'
stage=ins(context('EscortStage'));needs(stage,'?GetCurrentStep@Scenario@@','?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
assert {5,6,29071}<=values(stage) and ('cmp','byte ptr [rdi + 0xd5], 0') in pairs(stage)
for caller in ('UpdateArrival','UpdateRoutes'):
    needs(ins(context(caller)),context('EscortStage'))
valid=ins('??R<lambda_2>@?1??UpdateRoutes@State@MonkChenRescue@@');needs(valid,context('EscortStage'))
for body in (death,update,request):
    for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?SummonCreature@'):
        assert not sites(n,body,forbidden)
assert b'Monk Jorvinax: genuine98217 death46951 not acknowledged; retaining original quest attempts' in n['binary']
print('PASS actual registered Jorvinax real-death receipt: original98217/362203/full GUID/weak quest12103, native owner dispatch retained, missing46951-only real29071/6 acknowledgement, actual2s retry/epoch revalidation, bound9859006 node8/native endpoint and once-before-personal15 text, genuine6-only original escort continuation. Separate retained full-chain PE/client combat and visuals remain distinct.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scenario':943,'event':46951,'criterion':29071,'arrival_protocol':94350,'instance_size':instance_size}))
