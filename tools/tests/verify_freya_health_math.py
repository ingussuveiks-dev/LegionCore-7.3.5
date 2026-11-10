"""Actual post-repair Freya native64 health calculation in the linked PE."""
def verify_freya_health_math(ins,sites,values):
    r=ins('?EnterCombat@boss_freyaAI@boss_freya@@');store,=sites(r,'?SetMaxHealth@Unit@@')
    window=r[max(0,store-36):store]
    def op(m,s):return any(x.mnemonic==m and x.op_str==s for x in window)
    for m,s in [('movabs','rbx, 0xcccccccccccccccd'),('mov','r9, rax'),('mov','r10, rax'),
     ('not','r9'),('mul','r10'),('div','r11'),('shr','rcx, 2'),('cmp','rcx, rax'),
     ('imul','r8, rcx'),('lea','rcx, [rcx + rcx*4]'),('sub','rdx, rcx'),('sub','r9, r8'),
     ('imul','rdx, r11'),('mul','rdx'),('shr','rdx, 2'),('cmp','rdx, r9'),('cmovb','r9, rdx'),
     ('add','r9, r8'),('lea','rdx, [r10 + r9]')]:assert op(m,s),(m,s)
    branch,=[x for x in window if x.mnemonic=='ja']
    assert branch.operands[0].imm==r[store-2].address,'overflow branch must retain room bonus and skip multiplication/remainder writes'
    getter=sites(r[:store],'?GetUInt64Value@Object@@')[-1]
    assert 0x40 in values(r[getter-4:getter]),'actual native maxhealth field'
    zero,=[i for i,x in enumerate(r[:getter]) if x.mnemonic=='cmp' and x.op_str=='byte ptr [rdi + 0x174], bpl']
    assert r[zero+1].mnemonic=='jne' and any(x.mnemonic=='jmp' and x.operands[0].imm>r[store].address for x in r[zero+2:getter]),'zero elders must skip divisor/bonus branch'
