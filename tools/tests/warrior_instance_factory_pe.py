"""Follow the shared instance factory through a real outlined constructor."""
from sharasdal_erunak_entry_pe import sites

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
    assert ('mov','qword ptr [rdi + 0x380], rbx') in [(row.mnemonic,row.op_str) for row in selected]
    return constructor
