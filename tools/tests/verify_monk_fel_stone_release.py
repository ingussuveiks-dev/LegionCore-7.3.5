"""Trace registered native193890 scene/children and acknowledged return in PE.

Full retained Chen/Jorvinax/shared inspectors run separately against this hash.
Native linked code and controlled component tests do not certify client scenes.
"""
from pathlib import Path
import hashlib,json,runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_czi_timers_release.py')))
n,ins,refs,table,resolved=(p[k] for k in ('n','ins','refs','table','resolved'))
def helper(name):return '?'+name+'@MonkFelStone@@'
def method(cls,name):return '?'+name+'@'+cls+'@@'
def pairs(body):return [(o.mnemonic,o.op_str) for o in body]
def needs(body,*targets):
    for target in targets:assert sites(n,body,target),('Fel Stone native dependency',target)
def reachable(body,start):
    positions={o.address:i for i,o in enumerate(body)};pending=[start];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address not in positions:continue
        seen.add(address);o=body[positions[address]]
        if o.mnemonic in ('ret','int3','ud2'):continue
        if o.mnemonic.startswith('j') and o.operands[0].type==CS_OP_IMM:pending.append(o.operands[0].imm)
        if o.mnemonic!='jmp':pending.append(o.address+o.size)
    return seen

registration=ins('?AddSC_monk_fel_stone@@')
needs(ins('?AddScenarioScripts@@'),'?AddSC_monk_fel_stone@@','?AddSC_monk_intro_scenario@@')
factory_count=0
for spell,aura,callbacks in (
    ('spell_monk_fel_stone','aura_monk_fel_stone_scene',('Check','Launch','Before','AuraHit','Credit','After')),
    ('spell_monk_fel_stone_event',None,('Check','Event')),
    ('spell_monk_fel_stone_pilgrimage','aura_monk_fel_stone_pilgrimage',('Check','Apply'))):
    generic=('?$GenericSpellAndAuraScriptLoader@V'+spell+'@@V'+aura+'@@@@') if aura else ('?$GenericSpellScriptLoader@V'+spell+'@@@@')
    loader,raw=table('??_7'+generic+'6B@',4)
    assert loader in refs(registration)
    factory=ins('?GetSpellScript@'+generic);script,sraw=table('??_7'+spell+'@@6B@',8)
    assert n['symbol']('?GetSpellScript@'+generic) in resolved(raw) and script in refs(factory)
    assert n['symbol'](method(spell,'Register')) in resolved(sraw)
    bound=refs(ins(method(spell,'Register')))
    for callback in callbacks:assert n['symbol'](method(spell,callback)) in bound,('Actual spell hook',spell,callback)
    if aura:
        factory=ins('?GetAuraScript@'+generic);script,araw=table('??_7'+aura+'@@6B@',8)
        assert n['symbol']('?GetAuraScript@'+generic) in resolved(raw) and script in refs(factory)
        assert n['symbol'](method(aura,'Register')) in resolved(araw)
        bound=refs(ins(method(aura,'Register')))
        for callback in (('Apply','Applied','Removed','Update') if aura.endswith('_scene') else ('Applied',)):
            assert n['symbol'](method(aura,callback)) in bound
    factory_count+=1
creature='?$GenericCreatureScript@Unpc_monk_fel_stone@@@@'
loader,raw=table('??_7'+creature+'6BScriptObject@@@',12)
factory=ins('?GetAI@'+creature);actor,araw=table('??_7npc_monk_fel_stone@@6B@',82)
assert loader in refs(registration) and actor in refs(factory)
assert n['symbol']('?GetAI@'+creature) in resolved(raw)
assert n['symbol']('?OnSpellClick@npc_monk_fel_stone@@') in resolved(araw)
needs(factory,'??0SmartAI@@')
click=ins('?OnSpellClick@npc_monk_fel_stone@@')
assert {1014,98353,362198,12691299}<=values(click)
native,=sites(n,click,'?OnSpellClick@SmartAI@@')
for spawn in ('rax, 0x586d6','rax, 0xc1a763'):
    guard=next(i for i,o in enumerate(click) if (o.mnemonic,o.op_str)==('cmp',spawn))
    assert click[guard+1].mnemonic=='je' and click[native].address not in reachable(click,click[guard+1].operands[0].imm)
selector=ins('?selectAI@FactorySelector@@')
assert sites(n,selector,'?GetCreatureAI@ScriptMgr@@')[0]<sites(n,selector,'?GetAIName@Creature@@')[0]

player,raw=table('??_7player_monk_fel_stone@@6B@',49)
assert player in refs(registration)
for callback,dispatcher in (('OnUpdate','?OnUpdate@ScriptMgr@@'),('OnBeforeLogout','?OnPlayerBeforeLogout@ScriptMgr@@'),('OnMapChanged','?OnPlayerEnterMap@ScriptMgr@@')):
    target=n['symbol'](method('player_monk_fel_stone',callback));assert target in resolved(raw)
    calls=[o for o in ins(dispatcher) if o.mnemonic=='call' and o.operands[0].type==CS_OP_MEM and o.operands[0].mem.disp>0]
    assert any(o.operands[0].mem.disp//8<49 and resolved(raw)[o.operands[0].mem.disp//8]==target for o in calls),('Actual player hook dispatch',callback)
needs(ins(method('player_monk_fel_stone','OnUpdate')),helper('Find'),helper('Tick'))
needs(ins(method('player_monk_fel_stone','OnMapChanged')),helper('ObserveReturn'))

parent='spell_monk_fel_stone';check=ins(method(parent,'Check'));validate=ins(method(parent,'Validate'))
needs(check,'?GetOriginalTargetGUID@SpellScript@@','?GetCreature@Map@@',helper('Contact'),helper('Casting'),helper('Find'),helper('Reserve'),helper('ParentGuid'),'?GetSubType@ObjectGuid@@','?getQuestStatus@Player@@')
assert len(sites(n,check,'??4?$weak_ptr@E@std@@'))==1
# The quest weak assignment is inlined; the source weak assignment uses the
# outlined shared-to-weak operator. Both retain their actual control blocks.
assert {('lock inc','dword ptr [rcx + 0xc]'),('mov','qword ptr [rax + 0x58], rdx'),('mov','qword ptr [rax + 0x60], r8'),('add','rdx, 0x258'),('add','rcx, 0x68')}<=set(pairs(check))
assert {193890,200617,12103}<=values(check)
assert {193890,194141,200617,20000,1111,1492,17,15,430,90,98353,64,61,46952,4}<=values(validate)
assert len(sites(n,validate,'?GetTarget@SpellImplicitTargetInfo@@'))==5
contact=ins(helper('Contact'));active=ins(helper('Active'));current=ins(helper('Current'));retained=ins(helper('Retained'))
needs(contact,helper('Active'),'?GetQuestTemplate@QuestDataStoreMgr@@','?IsAllowedInRaid@Quest@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?IsWithinLOSInMap@WorldObject@@')
# ShouldHideFor is inlined here: full player GUID hash/equality, native hidden
# recipient container478/488/4a0, and a rejecting branch for a found recipient.
needs(contact,'?GetHash@ObjectGuid@@','??8ObjectGuid@@')
assert {0x478,0x488,0x4a0}<=fields(contact)
assert ('cmp','rdi, qword ptr [rbx + 0x478]') in pairs(contact)
assert {1014,98353,362198,12691299,12103,0x1000000}<=values(contact) and 5.0 in floats(n,contact)
assert {12103,98514,97778,29071,29072,6,7,10,25}<=values(active)
needs(current,helper('Active'),helper('Find'),'?getQuestStatus@Player@@')
assert ('lock cmpxchg','dword ptr [rdx + 8], ecx') in pairs(current)
assert {12103,10,25}<=values(retained)
child=ins(helper('Child'));needs(child,helper('Current'),'??8ObjectGuid@@','?GetMapId@ObjectGuid@@','?GetEntry@ObjectGuid@@',helper('Find'))
assert {1014,193890}<=values(child) and ('cmp','byte ptr [rdx + 0x78], sil') in pairs(child) and ('add','rdx, 0x20') in pairs(child)
for cls in ('spell_monk_fel_stone_event','spell_monk_fel_stone_pilgrimage'):
    load=ins(method(cls,'Load'))
    needs(load,'?GetMapId@ObjectGuid@@','?GetEntry@ObjectGuid@@')
    assert {1014,193890}<=values(load)
    assert pairs(load).count(('lea','rcx, [rbx + 0x58]'))==2
    needs(ins(method(cls,'Check')),helper('Child'),helper('Find'))

applied=ins(method('aura_monk_fel_stone_scene','Applied'));removed=ins(method('aura_monk_fel_stone_scene','Removed'));update=ins(method('aura_monk_fel_stone_scene','Update'))
needs(applied,helper('Current'),'?HasSceneStatus@Player@@')
assert {1111,1,3}<=values(applied) and ('mov','byte ptr [rax + 0x78], 1') in pairs(applied)
assert ('mov','dword ptr [rcx + 0x50], eax') in pairs(applied)
needs(removed,'?GetUnitOwner@AuraScript@@','?HasSceneStatus@Player@@',helper('Release'),helper('Cleanup'))
assert {('cmp','byte ptr [rax + 0x10], 4'),('mov','dword ptr [rsp + 0x30], 3'),('mov','byte ptr [rax + 0x79], 1')}<=set(pairs(removed))
needs(update,'?GetUnitOwner@AuraScript@@',helper('Current'),helper('Stop'))
assert not sites(n,update,'?GetTargetApplication@AuraScript@@')
marker=ins(method('aura_monk_fel_stone_pilgrimage','Applied'));registered=ins(method('aura_monk_fel_stone_pilgrimage','Register'))
assert {('mov','dword ptr [rsp + 0x20], 9'),('lea','rcx, [rbx + 0x180]')}<=set(pairs(registered))
assert {('cmp','byte ptr [rcx + 0x370], r14b'),('mov','qword ptr [rcx + 0x360], r14'),('mov','qword ptr [rcx + 0x368], r14'),('mov','byte ptr [rcx + 0x370], r14b'),('mov','byte ptr [rax + 0x7a], 1')}<=set(pairs(marker))
marker_check=ins(method('spell_monk_fel_stone_pilgrimage','Check'))
needs(marker_check,'?GetAura@Unit@@')
assert pairs(marker_check).count(('cmp','qword ptr [rax + 0x360], rsi'))==2
assert ('mov','word ptr [rsi + 0x7a], 0x101') in pairs(marker_check)
for aura in ('aura_monk_fel_stone_scene','aura_monk_fel_stone_pilgrimage'):
    descriptor,_=table('??_R0?AV'+aura+'@@@8',1)
    assert descriptor in refs(marker_check)
cleanup=ins(helper('Cleanup'));needs(cleanup,helper('Owns'),'?GetAura@Unit@@')
assert {193890,200617}<=values(cleanup)

tick=ins(helper('Tick'));observe=ins(helper('ObserveReturn'));retry=ins(helper('RetryChild'))
needs(tick,helper('Current'),helper('World'),helper('Leaf'),'?GetQuestObjectiveData@Player@@',helper('RetryChild'),helper('ObserveReturn'),'?TeleportTo@Player@@')
assert {100,2000,194141,200617,98353,29072,1514,3}<=values(tick)
assert {('cmp','byte ptr [rbx + 0x79], 0'),('mov','byte ptr [rax + 0x7d], 1')}<=set(pairs(tick))
assert not any(o.mnemonic=='mov' and o.op_str.startswith('byte ptr [') and o.op_str.endswith('+ 0x7e], 1') for o in tick),'Teleport acceptance is not observed arrival'
assert {736.019,3437.01,122.146}<=floats(n,tick)
needs(retry,'?CastSpell@Unit@@','?GetSpellInfo@SpellMgr@@')
assert {('mov','dword ptr [rsp + 0x78], 0x10'),('mov','dword ptr [rsp + 0x30], 0xfff7ffff'),('movups','xmm0, xmmword ptr [rax + 0x20]'),('movups','xmmword ptr [rsp + 0x5c], xmm0')}<=set(pairs(retry))
needs(observe,helper('Retained'),'?IsInWorld@Object@@','?CanContact@Player@@','?IsWithinDist3d@WorldObject@@',helper('Release'),helper('Cleanup'))
assert {1514,30000,2000}<=values(observe) and {736.019,3437.01,122.146,5.0}<=floats(n,observe)
done=next(i for i,o in enumerate(observe) if (o.mnemonic,o.op_str)==('mov','byte ptr [rax + 0x7e], 1'))
distance,=sites(n,observe,'?IsWithinDist3d@WorldObject@@');assert distance<done
assert {0x6848,0x6849,0x6851}<=fields(observe[:done])
for at in (30,34,36,38,40,42,44,54):
    assert observe[at].mnemonic in ('je','jne') and observe[done].address not in reachable(observe,observe[at].operands[0].imm)
for name in ('Tick','ObserveReturn','RetryChild','Cleanup','Current','Child'):
    for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?SummonCreature@'):
        assert not sites(n,ins(helper(name)),forbidden),(name,forbidden)
for message in (b'native193890 produced no authenticated1111 scene',b'original1111 scene produced no completion or native expiry',b'no observed arrival after30s',b'no original destination pose'):
    assert message in n['binary']
print('PASS registered Fel Stone five factories/player vtables and native hooks: real193890 scene430/1111, original full-GUID delayed children/weak quest and source, native default90/61 only, owned200617 and unrelated refresh, actual removal4/sceneCOMPLETE3 before return, bounded native retries and real1514/position/pending acknowledgement before completion. Retained native paths/live SQL and actual client remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'spell_factories':factory_count,'native_scene':1111,'native_children':[194141,200617],'return_map':1514}))
