"""Typed original25% heal with remaining native64 room checked before add."""
def verify_meljarak_heal(ins,sites,values):
 r=ins('?SpellHit@npc_generic_soldierAI@')
 def op(m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
 assert 122147 in values(r) and {0x38,0x40}<=set(values(r))
 assert op('shr','rbp, 2') and sites(r,'?LivingSquad@npc_soldierAI@@') and sites(r,'?SetHealth@Unit@@')
 for m,s in [('cmp','rax, rdi'),('cmovb','rdx, rax'),('sub','rax, rdx'),('cmp','rax, rbp'),('cmovb','rdx, rax'),('add','rdx, rdi')]:assert op(m,s),(m,s)
 assert not any(x.mnemonic=='movsxd' for x in r),'No signed32 narrowing of health/room'
