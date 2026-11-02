"""The exact490-byte Monk successor outlines its constructor.

Keep allocation/call registration separate from real constructor field stores.
Only the constructor's proven nonvolatile zero register receives an alias in
legacy field-pattern comparisons; actual instructions/addresses stay intact.
"""
import re
from capstone import CS_OP_IMM
from sharasdal_erunak_entry_pe import sites,values

def reachable(body,start,blocked=()):
    by_address={o.address:i for i,o in enumerate(body)};pending=[start];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address in blocked or address not in by_address:continue
        seen.add(address);o=body[by_address[address]]
        if o.mnemonic in ('ret','int3','ud2'):continue
        if o.mnemonic.startswith('j') and o.operands[0].type==CS_OP_IMM:pending.append(o.operands[0].imm)
        if o.mnemonic!='jmp':pending.append(o.address+o.size)
    return seen

def construction(n):
    entry=n['instructions']('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@')
    if 0x490 not in values(entry):return entry,entry,False
    pairs=[(o.mnemonic,o.op_str) for o in entry]
    assert len(entry)==13 and pairs[2:4]==[('mov','rbx, rdx'),('mov','ecx, 0x490')]
    assert sites(n,entry,'??2@')==[4] and sites(n,entry,'??0instance_monk_intro@@')==[8]
    assert pairs[6:8]==[('mov','rdx, rbx'),('mov','rcx, rax')]
    assert pairs[9:]==[('nop',''),('add','rsp, 0x20'),('pop','rbx'),('ret','')]
    body=n['instructions']('??0instance_monk_intro@@');bp=[(o.mnemonic,o.op_str) for o in body]
    assert not any(o.mnemonic.startswith('j') for o in body)
    assert ('mov','rbx, rdx') in bp and ('mov','rsi, rcx') in bp
    assert sites(n,body,'??0InstanceScript@@')==[8]
    zero=bp.index(('xor','ebp, ebp'));restore=bp.index(('pop','rbp'))
    assert all(not re.match(r'(?:rbp|ebp|bp|bpl)(?:,|$)',o.op_str) for o in body[zero+1:restore])
    assert {('lea','rdi, [rsi + 0x2e0]'),('mov','qword ptr [rsi + 0x430], rdi'),
            ('mov','qword ptr [rsi + 0x438], rbp'),('lea','rcx, [rsi + 0x440]'),
            ('lea','rcx, [rsi + 0x450]'),('mov','qword ptr [rsi + 0x460], rbp'),
            ('mov','qword ptr [rsi + 0x468], rbp'),('mov','qword ptr [rsi + 0x470], rbp'),
            ('mov','qword ptr [rsi + 0x478], rbp'),('mov','qword ptr [rsi + 0x480], rbp'),
            ('mov','word ptr [rsi + 0x488], bp'),('mov','rax, rsi')}<=set(bp)
    dispatch=n['instructions']('?Update@instance_monk_intro@@')
    before,=sites(n,dispatch,'?Update@State@MonkNineArrival@@')
    after,=sites(n,dispatch,'?Update@State@MonkChenRescue@@')
    assert before<after and ('add','rcx, 0x430') in [(o.mnemonic,o.op_str) for o in dispatch[:before]]
    return entry,body,True

def zero_alias_pairs(body):
    # construction() proves this exact straight-line constructor's EBP zero
    # definition, no overwrite and nonvolatile ABI before this comparison.
    names={'rbp':'rbx','ebp':'ebx','bp':'bx','bpl':'bl'}
    return [(o.mnemonic,re.sub(r'\b(?:rbp|ebp|bp|bpl)\b',lambda m:names[m[0]],o.op_str)) for o in body]
