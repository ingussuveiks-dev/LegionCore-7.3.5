"""Follow the shared instance factory through a real outlined constructor."""
from sharasdal_erunak_entry_pe import sites

def instance_guid_instructions(n,ins):
    method='?GetGuidData@instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
    if not any(name.startswith('?GetData@npc_danica_the_reclaimerAI@') for name in n['symbols']):return ins(method)
    from capstone import Cs,CS_ARCH_X86,CS_MODE_64,CS_OP_IMM
    address=n['symbol'](method)
    end=min(value for value in n['symbols'].values() if value>address)
    assert end-address<=128,('Selected two-GUID instance leaf must remain bounded',end-address)
    decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
    body=list(decoder.disasm(n['binary'][n['offset'](address):n['offset'](address)+end-address],address))
    assert not any(op.mnemonic=='call' for op in body)
    assert any(op.mnemonic=='ret' for op in body)
    for op in body:
        if op.mnemonic.startswith('j'):
            assert op.operands[0].type==CS_OP_IMM and address<=op.operands[0].imm<end
    return body

def selected_instance_factory(n,ins):
    factory='?GetInstanceScript@instance_broken_shore@@'
    constructor='??0instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
    body=ins(factory)
    calls=sites(n,body,constructor)
    if not calls:
        return factory
    at,=calls
    assert [(row.mnemonic,row.op_str) for row in body[at-2:at]]==[('mov','rdx, rbx'),('mov','rcx, rax')]
    assert [(row.mnemonic,row.op_str) for row in body[at+1:at+5]]==[('nop',''),('add','rsp, 0x20'),('pop','rbx'),('ret','')]
    selected=ins(constructor)
    assert sites(n,selected,'??0InstanceScript@@')
    pairs=[(row.mnemonic,row.op_str) for row in selected]
    if any(name.startswith('?GetData@npc_danica_the_reclaimerAI@') for name in n['symbols']):
        assert {('mov','rdi, rdx'),('mov','rsi, rcx'),('mov','qword ptr [rsi + 0x380], rdi'),
                ('mov','qword ptr [rsi + 0x3d8], rdi')}<=set(pairs)
        assert ('mov','ecx, 0x520') in [(row.mnemonic,row.op_str) for row in body[:at]]
    else:
        assert ('mov','qword ptr [rdi + 0x380], rbx') in pairs
    return constructor

def selected_malgalor_death(n,ins):
    """Trace the same fallback when MSVC inlines the small instance helper."""
    from capstone import CS_OP_IMM
    scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
    callback='?CreatureDies@'+scope;helper='?MalgalorDeath@'+scope
    body=ins(callback)
    if sites(n,body,helper):return helper
    assert any(name.startswith('?GetData@npc_danica_the_reclaimerAI@') for name in n['symbols'])
    capture,=sites(n,body,'?Capture@MalgalorFinale@@');death,=sites(n,body,'?Death@MalgalorFinale@@')
    assert capture<death and sites(n,body,'?Victory@MalgalorFinale@@') and sites(n,body[capture:death],'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
    positions={op.address:i for i,op in enumerate(body)}
    def reaches(address):
        pending=[address];seen=set()
        while pending:
            address=pending.pop()
            if address in seen or address not in positions:continue
            seen.add(address);op=body[positions[address]]
            if op.mnemonic in ('ret','int3','ud2'):continue
            if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm)
            if op.mnemonic!='jmp':pending.append(op.address+op.size)
        return seen
    for condition in ('eax, 0x169c0','qword ptr [rbx + 0x3a88], 0x582cf','dword ptr [rbx + 0xe0], 0x5dc','eax, 0x3a5','eax, 4'):
        at=next(i for i,op in enumerate(body[:capture]) if op.mnemonic=='cmp' and op.op_str==condition)
        assert body[at+1].mnemonic=='jne'
        rejected=reaches(body[at+1].operands[0].imm)
        assert body[capture].address not in rejected and body[death].address not in rejected
    return callback
