"""Native Unit timers run in ALIVE/CORPSE, not DEAD/transient death states."""
from sharasdal_erunak_entry_pe import sites

def verify_clock(native,method,member):
    body=native['instructions'](method)
    # A short leaf can lack unwind bounds; the PE decoder then also returns
    # the linker's trailing INT3 alignment. All executable guards stay exact.
    while body and body[-1].mnemonic=='int3':body=body[:-1]
    pairs=[(op.mnemonic,op.op_str) for op in body]
    assert len(body)==9
    assert pairs[:3]==[('mov','rax, qword ptr [rcx + 0x20]'),('mov','r8d, dword ptr [rax + 0xa18]'),('test','r8d, r8d')]
    assert body[3].mnemonic=='je' and body[3].operands[0].imm==body[6].address
    assert pairs[4]==('cmp','r8d, 2') and body[5].mnemonic=='jne' and body[5].operands[0].imm==body[8].address
    assert pairs[6]==('add',f'rcx, {member:#x}') and sites(native,body,'?Update@FunctionProcessor@@')==[7]
    assert body[7].mnemonic=='jmp' and pairs[8]==('ret','')
    return body
