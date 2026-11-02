"""Selected native Ragnvald opening queue/claim contract in the linked image."""
from capstone import CS_OP_IMM
from sharasdal_erunak_entry_pe import sites,values

def pairs(body):return [(op.mnemonic,op.op_str) for op in body]

def verify_actor(n,ins,table,resolved,reachable,parent,callback):
    suffix='npc_ragnvald_dragonbornAI@npc_ragnvald_dragonborn@@'
    vt,raw=table('??_7'+suffix+'6B@',82)
    slots=resolved(raw)
    marker='?GetData@'+suffix
    assert slots[0x50//8]==n['symbol'](marker)
    code=[row for row in ins(marker) if row.mnemonic!='int3']
    assert pairs(code)==[('xor','eax, eax'),('cmp','edx, 0x16c88'),('jne',code[2].op_str),
                         ('cmp','byte ptr [rcx + 0x38], al'),('setne','al'),('inc','eax'),('ret','')]
    assert code[2].operands[0].imm==code[-1].address
    los=n['symbol']('?MoveInLineOfSight@'+suffix)
    assert los in slots and pairs([row for row in ins('?MoveInLineOfSight@'+suffix) if row.mnemonic!='int3'])==[('ret','0')], 'Native empty LOS override must suppress default auto-aggro'
    queue,=sites(n,parent,'?AddDelayedEvent@FunctionProcessor@@')
    assert {93321,93324,2000,(-93320)&0xffffffff}<=values(parent)
    assert {('mov','qword ptr [rbp - 1], rdi'),('mov','dword ptr [rbp + 7], esi'),
            ('mov','ecx, dword ptr [rax + 0x3b8]'),('mov','dword ptr [rbp + 0xb], ecx'),
            ('mov','edx, 0x7d0'),('lea','rcx, [rdi + 0x60]')}<=set(pairs(parent[:queue]))
    branch=next(i for i,row in enumerate(parent) if row.mnemonic=='cmp' and row.op_str=='edx, 0xfffe9378')
    assert parent[branch+1].mnemonic=='jne'
    assert parent[queue].address not in reachable(parent,parent[branch+1].operands[0].imm)
    for operand,jump in (('byte ptr [rdi + 0x38], 0','jne'),('esi, esi','je'),('rax, rcx','jne')):
        at=next(i for i,row in enumerate(parent[:queue]) if row.mnemonic in ('cmp','test') and row.op_str==operand)
        assert parent[at+1].mnemonic==jump
        assert parent[queue].address not in reachable(parent,parent[at+1].operands[0].imm)
    for name,jump in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('??9ObjectGuid@@QEBA_NAEBV0@@Z','jne')):
        at,=sites(n,parent[:queue],name)
        assert pairs(parent[at+1:at+2])==[('test','al, al')] and parent[at+2].mnemonic==jump
        assert parent[queue].address not in reachable(parent,parent[at+2].operands[0].imm)
    alive=next(i for i,row in enumerate(parent[:queue]) if row.mnemonic=='cmp' and row.op_str=='dword ptr [rcx + 0xa18], 0')
    assert parent[alive+1].mnemonic=='jne' and parent[queue].address not in reachable(parent,parent[alive+1].operands[0].imm)
    assert len(sites(n,parent[:queue],'?GetCreature@ObjectAccessor@@'))==1
    bridge=ins(callback)
    assert pairs(bridge)[:1]==[('add','rcx, 8')] and bridge[1].mnemonic=='jmp'
    outlined,=[name for name in n['symbols'] if name.startswith('??R<lambda_') and 'DoAction@'+suffix in name and n['symbol'](name)==n['resolve'](bridge[1].operands[0].imm)]
    body=ins(outlined)
    claim,=[i for i,row in enumerate(body) if row.mnemonic=='call' and row.op_str=='qword ptr [rax + 0xb8]']
    request,started=[i for i,row in enumerate(body) if row.mnemonic=='call' and row.op_str=='qword ptr [rax + 0xb0]']
    assert request<claim<started
    assert pairs(body[request-3:request])==[('mov','rax, qword ptr [rdi]'),('mov','edx, 0x16c89'),('mov','rcx, rdi')]
    assert pairs(body[request+1:request+2])==[('cmp','eax, dword ptr [rsi + 8]')]
    assert body[request+2].mnemonic=='jne'
    rejected=body[request+2].operands[0].imm
    assert body[claim].address not in reachable(body,rejected)
    assert {('mov','r8d, dword ptr [rsi + 8]'),('mov','edx, 0x16c8b'),('mov','rcx, rdi')}<=set(pairs(body[claim-5:claim]))
    assert ('mov','edx, 0x16c8a') in pairs(body[claim:started])
    assert pairs(body[started+1:started+2])==[('cmp','eax, ebx')] and body[started+2].mnemonic=='jne'
    assert body[started+2].operands[0].imm==rejected
    payload=body[started+3:]
    helper='?AttackBattleTarget@npc_ragnvald_dragonbornAI@'
    assert len(sites(n,payload,helper))==len(sites(n,payload,'?GetGuidValue@Object@@'))==3
    assert len(sites(n,payload,'?SetHomePosition@Creature@@QEAAXMMMM'))==2
    assert {102990,102995,97166,97167,97176,102970,102964,102965,105419}<=values(payload)
    assert ('cmp','qword ptr [rbx + 0x10], rcx') in pairs(payload) and ('cmove','rdi, rdx') in pairs(payload)
    own,=[i for i,row in enumerate(payload) if row.mnemonic=='call' and row.op_str=='qword ptr [r8 + 0x10]']
    assert pairs(payload[own-3:own])==[('mov','rcx, qword ptr [rsi]'),('mov','r8, qword ptr [rcx]'),('mov','rdx, rax')]
    for at in sites(n,payload,helper):
        assert ('mov','r9b, 0xa') in pairs(payload[at-6:at])
        assert ('movups','xmm0, xmmword ptr [rax]') in pairs(payload[at-12:at])
    for name,jump in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('??9ObjectGuid@@QEBA_NAEBV0@@Z','jne')):
        at,=sites(n,body[:request],name)
        assert pairs(body[at+1:at+2])==[('test','al, al')] and body[at+2].mnemonic==jump
        assert body[at+2].operands[0].imm==rejected
    for operand in ('dword ptr [rcx + 0xa18], 0','rax, rbx','dword ptr [rdx + 0x3b8], ecx'):
        at=next(i for i,row in enumerate(body[:request]) if row.mnemonic=='cmp' and row.op_str==operand)
        assert body[at+1].mnemonic=='jne' and body[at+1].operands[0].imm==rejected
    assert {('mov','ecx, dword ptr [rsi + 0xc]'),('mov','byte ptr [rax + 0x38], 0')}<=set(pairs(body[:request]))
    assert len(sites(n,body[:request],'?GetCreature@ObjectAccessor@@'))==1
    targets=sites(n,body[:claim],'?GetClosestCreatureWithEntry@@')
    assert len(targets)==2
    assert ('mov','edx, 0x17b98') in pairs(body[targets[0]-5:targets[0]]) and ('mov','edx, 0x1923a') in pairs(body[targets[1]-5:targets[1]])
    assert body[targets[0]+3].mnemonic==body[targets[1]+2].mnemonic=='jne'
    assert body[targets[0]+3].operands[0].imm==body[targets[1]+2].operands[0].imm
    assert body[claim].address in reachable(body,body[targets[1]+2].operands[0].imm)
    assert b'Warrior opening: no original nearby battle target' in n['binary']
    for name in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
        assert not sites(n,body,name) and not sites(n,parent,name)
    return {'body':body,'payload':payload,'outlined':outlined,'marker_slot':0x50,'instance_slots':(0x90,0xb0,0xb8)}

def verify_second_battle(n,body,reachable):
    at,=sites(n,body,'?AttackBattleTarget@npc_ragnvald_dragonbornAI@')
    guid=[i for i in sites(n,body,'?GetGuidValue@Object@@') if i<at][-1]
    assert pairs(body[guid-2:guid])==[('xor','edx, edx'),('mov','rcx, r14')]
    assert {('movups','xmm0, xmmword ptr [rax]'),('movaps','xmmword ptr [rbp - 0x29], xmm0'),
            ('mov','r9b, 0xa'),('xor','r8d, r8d'),('lea','rdx, [rbp - 0x29]'),
            ('mov','rcx, qword ptr [rbx + 0x10]')}<=set(pairs(body[guid+1:at]))
    branch=next(i for i,row in enumerate(body) if row.mnemonic=='cmp' and row.op_str=='edx, 0xfffe9378')
    assert body[at].address in reachable(body,body[branch+1].operands[0].imm)
    assert ('call','qword ptr [rax + 0x10]') in pairs(body[at:])
