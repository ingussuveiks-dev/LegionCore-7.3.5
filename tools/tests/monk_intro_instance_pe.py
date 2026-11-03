"""The exact490/498/4a0-byte Monk successors outline their constructors.

Keep allocation/call registration separate from real constructor field stores.
Only proven constructor context/instance/zero registers receive aliases in
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
    opening=0x4a0 in values(entry)
    initial=opening or 0x498 in values(entry)
    n['monk_initial_successor']=initial
    n['monk_hait_opening_successor']=opening
    n['monk_hait_defeat_successor']=any(name.startswith('?UpdateDefeat@State@MonkHaitOpening@@') for name in n['symbols'])
    n['monk_hait_state_size']=0x98 if n['monk_hait_defeat_successor'] else 0x60
    n['monk_instance_size']=0x4a0 if opening else 0x498 if initial else 0x490
    if not initial and 0x490 not in values(entry):return entry,entry,False
    pairs=[(o.mnemonic,o.op_str) for o in entry]
    assert len(entry)==13 and pairs[2:4]==[('mov','rbx, rdx'),('mov',f"ecx, {n['monk_instance_size']:#x}")]
    assert sites(n,entry,'??2@')==[4] and sites(n,entry,'??0instance_monk_intro@@')==[8]
    assert pairs[6:8]==[('mov','rdx, rbx'),('mov','rcx, rax')]
    assert pairs[9:]==[('nop',''),('add','rsp, 0x20'),('pop','rbx'),('ret','')]
    body=n['instructions']('??0instance_monk_intro@@');bp=[(o.mnemonic,o.op_str) for o in body]
    assert not any(o.mnemonic.startswith('j') for o in body)
    assert ('mov','rbp, rdx' if opening else 'rbx, rdx') in bp and ('mov','r14, rcx' if initial else 'rsi, rcx') in bp
    assert sites(n,body,'??0InstanceScript@@')==([10] if opening else [9] if initial else [8])
    zero=bp.index(('xor','r15d, r15d' if opening else 'ebp, ebp'));restore=bp.index(('pop','r15' if opening else 'rbp'))
    assert all(not re.match(r'(?:r15|r15d|r15w|r15b)(?:,|$)' if opening else r'(?:rbp|ebp|bp|bpl)(?:,|$)',o.op_str) for o in body[zero+1:restore])
    if initial:
        assert {('mov','ecx, 0xa0'),('mov','rdi, rax'),('mov','qword ptr [rax], rsi'),
                ('mov','qword ptr [r14 + 0x490], rdi'),('mov','rax, r14')}<=set(bp)
        assert len(sites(n,body,'??2@'))==(2 if opening else 1)
        if opening:
            assert {('mov','ecx, '+hex(n['monk_hait_state_size'])),('mov','qword ptr [rax], rbp'),('mov','qword ptr [r14 + 0x498], rbx')}<=set(bp)
        bp=constructor_pairs(body,False)
    assert {('lea','rdi, [rsi + 0x2e0]'),('mov','qword ptr [rsi + 0x430], rdi'),
            ('mov','qword ptr [rsi + 0x438], rbp'),('lea','rcx, [rsi + 0x440]'),
            ('lea','rcx, [rsi + 0x450]'),('mov','qword ptr [rsi + 0x460], rbp'),
            ('mov','qword ptr [rsi + 0x468], rbp'),('mov','qword ptr [rsi + 0x470], rbp'),
            ('mov','qword ptr [rsi + 0x478], rbp'),('mov','qword ptr [rsi + 0x480], rbp'),
            ('mov','word ptr [rsi + 0x488], bp'),('mov','rax, rsi')}<=set(bp)
    dispatch=n['instructions']('?Update@instance_monk_intro@@')
    before,=sites(n,dispatch,'?Update@State@MonkNineArrival@@')
    after,=sites(n,dispatch,'?Update@State@MonkChenRescue@@')
    assert before<after and (('lea','rcx, [rdi + 0x430]') if initial else ('add','rcx, 0x430')) in [(o.mnemonic,o.op_str) for o in dispatch[:before]]
    return entry,body,True

def zero_alias_pairs(body):
    # construction() proves this exact straight-line constructor's EBP zero
    # definition, no overwrite and nonvolatile ABI before this comparison.
    return constructor_pairs(body,True)

def constructor_pairs(body,zero):
    names={'rbp':'rbx','ebp':'ebx','bp':'bx','bpl':'bl'} if zero else {}
    if ('mov','r14, rcx') in [(o.mnemonic,o.op_str) for o in body]:
        # Exact straight-line498 constructor checked above: R14 is the
        # instance, RSI its unchanged2e0 context, EBP remains zero across calls.
        names.update(r14='rsi',rsi='rdi')
    if ('mov','rbp, rdx') in [(o.mnemonic,o.op_str) for o in body]:
        # Exact4a0 constructor: RBP is the retained Map, R15 remains zero;
        # the original context/instance prefix is unchanged. Keep both
        # source-map and zero-definition aliases tied to that proof above.
        names.update(rbp='rbx',ebp='ebx',bp='bx',bpl='bl',r15='rbx' if zero else 'rbp',r15d='ebx' if zero else 'ebp',r15w='bx' if zero else 'bp',r15b='bl' if zero else 'bpl')
    pattern=r'\b('+ '|'.join(names)+r')\b'
    return [(o.mnemonic,re.sub(pattern,lambda m:names[m[0]],o.op_str)) for o in body]
