from linked_player_abi import inherited_operand
"""Selected native Danica bridge and instance-owned ritual implementation."""
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,fields

def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def ritual(name):return '?'+name+'@State@WarriorRitual@@'
def needs(n,body,*names):
    for name in names:assert sites(n,body,name),('Native ritual dependency',name)
def floats(n,body):
    return {struct.unpack_from('<f',n['binary'],n['offset'](op.address+op.size+arg.mem.disp))[0]
            for op in body if op.mnemonic=='movss' for arg in op.operands if arg.type==CS_OP_MEM and arg.mem.base==X86_REG_RIP}

def verify_bridge(n,ins,refs,table,resolved,reachable):
    suffix='npc_danica_the_reclaimerAI@npc_danica_the_reclaimer@@'
    vt,raw=table('??_7'+suffix+'6B@',82);slots=resolved(raw)
    script,sraw=table('??_7npc_danica_the_reclaimer@@6BScriptObject@@@',16)
    factory=ins('?GetAI@npc_danica_the_reclaimer@@')
    assert vt in refs(factory) and script in refs(ins('?AddSC_wars_intro_scenario@@'))
    assert n['symbol']('?GetAI@npc_danica_the_reclaimer@@') in resolved(sraw)
    for method in ('GetData','SpellHitTarget'):assert n['symbol']('?'+method+'@'+suffix) in slots
    _,base_raw=table('??_7ScriptedAI@@6B@',82)
    base_slots=resolved(base_raw)
    update_slot,=[i for i,at in enumerate(base_slots) if at==n['symbol']('?UpdateAI@ScriptedAI@@')]
    empty_update=n['symbol']('?UpdateAI@'+suffix)
    assert slots[update_slot]==empty_update
    assert n['binary'][n['offset'](empty_update):n['offset'](empty_update)+3]==b'\xc2\x00\x00'
    assert 0x38 in values(factory) and not sites(n,factory,'??0FunctionProcessor@@')
    status=ins('?GetData@'+suffix)
    assert pairs(status)[:4]==[('xor','eax, eax'),('cmp','edx, 0x16c92'),('sete','al'),('ret','')]
    body=ins('?SpellHitTarget@'+suffix)
    dispatch,=[i for i,op in enumerate(body) if op.mnemonic=='call' and op.op_str=='rbx']
    for operand,jump in (('rdx, rdx','je'),('r8, r8','je'),('dword ptr [r8], 0x321e7','jne'),
                         ('dword ptr [rcx + 0xa18], 0','jne'),('rax, rbx','jne')):
        at=next(i for i,op in enumerate(body[:dispatch]) if op.mnemonic in ('test','cmp') and op.op_str==operand)
        assert body[at+1].mnemonic==jump and body[dispatch].address not in reachable(body,body[at+1].operands[0].imm)
    for dependency,jump in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne')):
        at,=sites(n,body,dependency);assert body[at+2].mnemonic==jump
        assert body[dispatch].address not in reachable(body,body[at+2].operands[0].imm)
    needs(n,body,'?GetCreature@ObjectAccessor@@','?GetInstanceScript@WorldObject@@','??8ObjectGuid@@')
    assert {93331,93332}<=values(body)
    assert {('mov','r9, qword ptr [rcx + 0x90]'),('mov','rbx, qword ptr [rax + 0x98]'),
            ('movups','xmm0, xmmword ptr [rax]'),('movaps','xmmword ptr [rsp + 0x20], xmm0')}<=set(pairs(body))
    equal,=sites(n,body,'??8ObjectGuid@@');assert body[equal+2].mnemonic=='je'
    assert body[dispatch].address not in reachable(body,body[equal+2].operands[0].imm)
    assert not sites(n,body,'?CastSpell@Unit@@') and not sites(n,body,'?UpdateAchievementCriteria@Player@@')

def verify_presentation_handoff(n,ins,refs,table,resolved,reachable):
    n['ragnvald_second_battle_successor']=any(name.startswith('?Update@State@RagnvaldSecondBattle@@') for name in n['symbols'])
    prepare=ins(ritual('Prepare'));send,=sites(n,prepare,'?SendChat@CreatureTextMgr@@')
    assert {97166,361091,102964,102965,92445,92446,92450,92447,97176,102990,102995,97167,105419}<=values(prepare)
    needs(n,prepare,ritual('Ready'),ritual('Danica'),'?Kill@Unit@@','?GetCreature@Map@@','?MovePoint@MotionMaster@@')
    assert ('xor','r8d, r8d') in pairs(prepare[send-5:send]) and not sites(n,prepare,'?Talk@CreatureAI@@')
    assert not 0x9c0 in fields(prepare),'Native text must not borrow another actor AI'
    assert {160.0,90.0,100.0}<=floats(n,prepare)
    needs(n,prepare,'?InSamePhase@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
    # Float literals are represented by their exact32-bit native values.
    for value in (-1980.04,-1251.96,4.83):assert any(abs(found-value)<0.0001 for found in floats(n,prepare))
    body=ins(ritual('Update'))
    action,=[i for i,op in enumerate(body) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x48]']
    if n.get('ragnvald_second_battle_successor'):
        # MSVC now inlines the unchanged Depart into the native stage3 branch.
        leave=next(i for i,op in enumerate(body) if op.mnemonic=='cmp' and op.op_str=='byte ptr [rdi + 0x105], 0')
        finish=next(i for i,op in enumerate(body[leave:],leave) if op.mnemonic=='lea' and op.op_str=='r12, [rdi + 0xc8]')
        depart=body[leave:finish]
        assert {('lea','r15, [rdi + 0x88]'),('mov','rdi, rcx')}<=set(pairs(body[:leave]))
        assert {('movups','xmm0, xmmword ptr [r15 - 0x20]'),('movups','xmm1, xmmword ptr [r15]'),
                ('movups',inherited_operand('xmm0, xmmword ptr [r14 + 0x3b64]')),('mov','edx, 0x7c'),
                ('lea','ecx, [r13 + 0x321ef]'),('cmp','eax, ecx'),('cmp','r12b, 2')}<=set(pairs(depart))
    else:
        depart=ins(ritual('Depart'))
        leave,=sites(n,body,ritual('Depart'))
        assert {('lea','rbp, [rcx + 0x88]'),('movups','xmm0, xmmword ptr [rbp - 0x20]'),
                ('movups','xmm1, xmmword ptr [rbp]'),('movups',inherited_operand('xmm0, xmmword ptr [rdi + 0x3b64]'))}<=set(pairs(depart))
    assert leave<action and 4000 in values(depart) and 40.0 in floats(n,depart)
    assert len(sites(n,depart,'?MovePoint@MotionMaster@@'))==len(sites(n,depart,'?DespawnOrUnsummon@Creature@@'))==2
    needs(n,depart,'??8ObjectGuid@@',ritual('Danica'),'?GetUInt32Value@Object@@')
    assert {97167,361094,98257,361151,93320,2000,46912,46929}<=values(body[:action])
    assert 250.0 in floats(n,body[:action])
    if n.get('ragnvald_second_battle_successor'):
        snapshot,=[i for i,op in enumerate(body[:action]) if op.mnemonic=='call' and op.op_str=='rbx']
        assert {('mov','byte ptr [rdi + 0x106], 1'),('mov','byte ptr [rdi + 0xfc], 0'),
                ('mov','rbx, qword ptr [rcx + 0x98]'),('mov','edx, 0x16c96'),
                ('movups','xmm0, xmmword ptr [rax]'),('lea','r8, [rsp + 0x100]')}<=set(pairs(body[leave:snapshot]))
        assert {('mov','edx, 1'),('mov','rcx, r15')}<=set(pairs(body[snapshot+1:action]))
        assert sites(n,body[snapshot+1:action],'?clear@?$vector@UMember@MalgalorFinale@@')
    else:
        assert {('mov','byte ptr [rbx + 0x106], 1'),('mov','byte ptr [rbx + 0xfc], 0'),
                ('mov','edx, 1'),('mov','rcx, rdi')}<=set(pairs(body[action-8:action]))
    for operand,jump in ((('r14, r14','je'),('rbx, rbx','je'),('eax, 1','ja')) if n.get('ragnvald_second_battle_successor') else (('r15, r15','je'),('r14, r14','je'),('eax, 1','ja'))):
        at=max(i for i,op in enumerate(body[leave:action],leave) if op.mnemonic in ('test','cmp') and op.op_str==operand)
        assert body[at+1].mnemonic==jump and body[action].address not in reachable(body,body[at+1].operands[0].imm)
    if n.get('ragnvald_second_battle_successor'):
        missing=next(i for i,op in enumerate(body[leave:action],leave) if op.mnemonic=='test' and op.op_str=='r15, r15')
        assert body[missing+1].mnemonic=='je'
        target=next(i for i,op in enumerate(body) if op.address==body[missing+1].operands[0].imm)
        assert pairs(body[target:target+1])==[('mov','r13d, esi')]
        assert ('xor','esi, esi') in pairs(body[leave:target])
        assert ('lea','eax, [r13 - 1]') in pairs(body[target:action])
    for dependency in ('?InSamePhase@WorldObject@@','?IsWithinDistInMap@WorldObject@@'):
        at,=sites(n,body[leave:action],dependency);at+=leave
        assert body[at+2].mnemonic=='je' and body[action].address not in reachable(body,body[at+2].operands[0].imm)
    at=next(i for i,op in enumerate(body[:leave]) if op.mnemonic=='cmp' and op.op_str=='rax, rcx')
    assert body[at+1].mnemonic=='jb'
    early=reachable(body,body[at+1].operands[0].imm)
    assert body[leave].address not in early and body[action].address not in early
    for forbidden in ('?FindNearestPlayer@WorldObject@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
        assert not sites(n,depart,forbidden) and not sites(n,prepare,forbidden)

def verify_state(n,ins,refs,table,resolved,reachable):
    scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
    vt,raw=table('??_7'+scope+'6B@',80);slots=resolved(raw)
    for slot,method in ((0x90,'GetGuidData'),(0x98,'SetGuidData')):assert slots[slot//8]==n['symbol']('?'+method+'@'+scope)
    setter=ins('?SetGuidData@'+scope)
    if n.get('ragnvald_second_battle_successor'):
        hit,=sites(n,setter,ritual('Hit'))
        assert {('lea','rbx, [rcx + 0x3d8]'),('cmp','edx, 0x16c94'),('movups','xmm0, xmmword ptr [r8]'),
                ('mov','rcx, rbx'),('lea','rdx, [rsp + 0x20]'),('movaps','xmmword ptr [rsp + 0x20], xmm0')}<=set(pairs(setter[:hit]))
        assert setter[hit+3].mnemonic=='ret'
    else:
        assert {('cmp','edx, 0x16c94'),('movups','xmm0, xmmword ptr [r8]'),('add','rcx, 0x3d8')}<=set(pairs(setter))
    needs(n,setter,ritual('Hit'))
    death_hook=ins('?CreatureDies@'+scope)
    capture,=sites(n,death_hook,ritual('Death'));wave,=sites(n,death_hook,'?Death@State@WarriorFirstWave@@')
    assert capture<wave,'Capture original weak participants before first-wave native advancement clears its own receipt'
    needs(n,ins('?OnCreatureCreate@'+scope),ritual('Observe'))
    needs(n,ins('?Update@'+scope),ritual('Update'))
    ctor=ins('??0'+scope)
    assert ('mov','qword ptr [rsi + 0x3d8], rdi') in pairs(ctor) and vt in refs(ctor)
    # Instance GUID getter is a bounded leaf; instructions() uses the next
    # public symbol, retaining both real128-bit branches, not a16-byte guess.
    from warrior_instance_factory_pe import instance_guid_instructions
    guid=instance_guid_instructions(n,ins)
    assert {93324,93331}<=values(guid) and ('movups','xmm0, xmmword ptr [rcx + 0x3f0]') in pairs(guid)
    assert ('movups','xmmword ptr [rdx], xmm0') in pairs(guid)

    native=ins(ritual('Native'));owned=ins(ritual('Owned'));progress=ins(ritual('Progress'))
    for body in (native,owned):needs(n,body,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetUInt32Value@Object@@')
    assert {('cmp',inherited_operand('qword ptr [rbx + 0x3a88], rsi')),('cmp','al, byte ptr [rsp + 0x50]')}<=set(pairs(native))
    assert {('test','byte ptr [rax + 0x30e8], 1'),('movups',inherited_operand('xmm0, xmmword ptr [rdi + 0x3b64]')),
            ('mov','edx, 0x7c'),('cmp','eax, dword ptr [rsp + 0x70]')}<=set(pairs(owned))
    needs(n,owned,'??8ObjectGuid@@')
    assert {1500,933}<=values(progress);needs(n,progress,'?GetScenario@ScenarioMgr@@')
    ready=ins(ritual('Ready'));assert {2,45771,46175}<=values(ready)
    needs(n,ready,ritual('Progress'),ritual('Leaf'),ritual('Current'))
    retained=ins(ritual('Retained'));current=ins(ritual('Current'))
    needs(n,retained,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?GetQuestStatus@Player@@','?GetReqKillOrCastCurrentCount@Player@@')
    assert 103661 in values(retained) and any(op.mnemonic=='lock cmpxchg' for op in retained)
    needs(n,current,'?Original@MalgalorFinale@@',ritual('Retained'),ritual('Danica'),'?InSamePhase@WorldObject@@')
    body=ins(ritual('Death'))
    assert {102970,361138,0x380,103661,1,3}<=values(body)
    needs(n,body,ritual('Progress'),ritual('Native'),'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',
          '?AddQuestDelayedEvent@Player@@','?GetQuestStatus@Player@@','?InSamePhase@WorldObject@@')
    check,=sites(n,body,ritual('Native'))
    assert pairs(body[check+1:check+3])==[('cmp','rax, r13'),('jne',body[check+2].op_str)]
    assert ('mov','byte ptr [rsp + 0x20], 0') in pairs(body[check-8:check])
    assert ('cmp','eax, 1') in pairs(body[:check]) and ('lea','rbp, [rsi + 0x58]') in pairs(body[:check])
    observe=ins(ritual('Observe'))
    assert {102970,361138,97167,361094,97166,361091,98257,361151,103608,103688}<=values(observe)
    needs(n,observe,ritual('Progress'),ritual('Danica'),'??8ObjectGuid@@')
    assert {('lea','ecx, [r13 + 0x321ef]'),('movups',inherited_operand('xmm0, xmmword ptr [r14 + 0x3b64]')),
            ('movups','xmmword ptr [rbp + 0x40], xmm0'),('cmp','byte ptr [r12], 0')}<=set(pairs(observe))

    accept=ins(ritual('AcceptSpirit'))
    at,=sites(n,accept,ritual('Owned'))
    assert ('cmp','byte ptr [r14 + rbx + 0x102], 0') in pairs(accept[:at])
    assert {('movups','xmm0, xmmword ptr [rdi + 0xa8]'),('movups','xmmword ptr [rdi + 0x88], xmm0'),
            ('mov','byte ptr [r14 + rbx + 0x100], 0'),('mov','dword ptr [rbx + 0xf8], 0')}<=set(pairs(accept[at:]))
    assert accept[at+2].mnemonic=='je'
    copied=next(op.address for op in accept[at:] if op.mnemonic=='movups' and op.op_str=='xmmword ptr [rdi + 0x88], xmm0')
    assert copied not in reachable(accept,accept[at+2].operands[0].imm)
    hit=ins(ritual('Hit'));cast,=sites(n,hit,'?CastSpell@Unit@@')
    assert {('cmp','byte ptr [rcx + 0xff], 0'),('lea','rdi, [rsi + 0xa8]'),('lea','r14, [rsi + 0x100]'),
            ('lea','ecx, [r15 + 0x19234]'),('lea','ecx, [r15 + 0x5828a]')}<=set(pairs(hit[:cast]))
    gate,=sites(n,hit,ritual('Ready'));assert hit[gate+2].mnemonic=='je'
    assert hit[cast].address not in reachable(hit,hit[gate+2].operands[0].imm)
    assert {('lea','r8d, [r15 + 0x321ef]'),('xor','r9d, r9d'),('mov','rdx, rbx'),('mov','rcx, rbx')}<=set(pairs(hit[cast-12:cast]))
    assert pairs(hit[cast+1:cast+5])==[('test','eax, eax'),('lea','rbx, [rsi + 0xfc]'),('sete','cl'),('mov','byte ptr [r14 + 2], cl')]
    accepted,=sites(n,hit,ritual('AcceptSpirit'));assert accepted>cast
    assert ('cmp','byte ptr [rbx], r13b') in pairs(hit[cast:accepted])

    speak=ins(ritual('Speak'));factory,=sites(n,speak,'?CreateConversation@Conversation@@')
    needs(n,speak,'?InSamePhase@WorldObject@@')
    assert 1405 in values(speak) and speak[factory+2].mnemonic=='jne'
    needs(n,speak[factory+1:],ritual('Danica'),ritual('Ready'),'?GetConversation@ObjectAccessor@@','?Remove@Conversation@@')
    views=sites(n,speak,'?GetCreature@Conversation@@');assert len(views)==2
    assert ('cmp','rax, 6') in pairs(speak[factory:views[0]]) and ('sub','rax, 0xc') in pairs(speak[factory:views[0]])
    assert {('lea','rcx, [rsi + 0x18]'),('lea','rcx, [rsi + 0x28]')}<=set(pairs(speak[views[-1]:]))
    assert len(sites(n,speak,'?GetLowPart@ObjectGuid@@'))==len(sites(n,speak,'?GetHighPart@ObjectGuid@@'))==2

    update=ins(ritual('Update'));events=sites(n,update,'?UpdateAchievementCriteria@Player@@');assert len(events)==2
    one,two=events;assert {49203,49262,92,2000,3000}<=values(update)
    needs(n,update,ritual('Progress'),ritual('Retained'),ritual('Current'),ritual('Speak'),ritual('AcceptSpirit'),ritual('Cancel'))
    state='rdi' if n.get('ragnvald_second_battle_successor') else 'rbx'
    assert ('cmp','rax, 0xbb8') in pairs(update[:one]) and ('add',f'qword ptr [{state} + 0xe0], r15') in pairs(update[:one])
    assert {('lea','rcx, ['+state+' + 0x88]'),('lea','rcx, ['+state+' + 0x98]'),('mov','dword ptr ['+state+' + 0xf8], 0x7d0')}<=set(pairs(update[:one]))
    assert ('mov','r8d, 0xc033') in pairs(update[one-5:one]) and ('mov','r8d, 0xc06e') in pairs(update[two-5:two])
    assert sites(n,update[one+1:two],ritual('Current')) and sites(n,update[one+1:two],ritual('Progress'))
    assert ('cmp','byte ptr ['+state+' + 0xfc], 0') in pairs(update[one+1:two])
    # Native two-leaf gates skip exactly their already accepted event.
    assert update[one-8].mnemonic=='jne' and update[one-8].operands[0].imm==update[one+1].address
    assert update[two-8].mnemonic=='jne' and update[two-8].operands[0].imm==update[two+1].address
    normal,=sites(n,update,'?CastSpell@Unit@@')
    assert {('xor','r9d, r9d'),('mov','r8d, 0x321e7'),('mov','rdx, r15'),('mov','rcx, r15')}<=set(pairs(update[normal-8:normal]))
    assert ('mov','edx, 0x16c92') in pairs(update[normal-20:normal]) and ('cmp','eax, 1') in pairs(update[normal-20:normal])
    for body in (update,hit,ins(ritual('Death')),speak,update if n.get('ragnvald_second_battle_successor') else ins(ritual('Depart')),ins(ritual('Prepare'))):
        for forbidden in ('?FindNearestPlayer@WorldObject@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
            assert not sites(n,body,forbidden)
