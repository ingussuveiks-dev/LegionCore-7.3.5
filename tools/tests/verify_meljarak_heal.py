"""Native profile or pinned legacy25% heal;unsigned64 remaining room before add."""
def verify_meljarak_heal(ins,sites,values):
 r=ins('?SpellHit@npc_generic_soldierAI@')
 def op(m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
 assert 122147 in values(r) and {0x38,0x40}<=set(values(r))
 assert sites(r,'?LivingSquad@npc_soldierAI@@') and sites(r,'?SetHealth@Unit@@')
 if sites(r,'?GetEffect@SpellInfo@@'):
  assert sites(r,'?GetDifficultyID@ScriptedAI@@'),'Actual cast difficulty must select the native profile'
  for m,s in [('movabs','r10, 0x47ae147ae147ae15'),('imul','rcx, r8, 0x64'),('sub','rbp, rcx'),('imul','rbp, r9'),('imul','r9, r8'),('add','rbp, r9')]:assert op(m,s),(m,s)
  assert sum(x.mnemonic=='mul'and x.op_str=='rbp'for x in r)==2,'Both quotient/remainder divisions must remain unsigned64'
  current='rbx'
 else:
  assert op('shr','rbp, 2'),'Pinned legacy normal25% amount changed'
  current='rdi'
 for m,s in [('cmp','rax, '+current),('cmovb','rdx, rax'),('sub','rax, rdx'),('cmp','rax, rbp'),('cmovb','rdx, rax'),('add','rdx, '+current)]:assert op(m,s),(m,s)
 assert not any(x.mnemonic=='movsxd' for x in r),'No signed32 narrowing of health/room'
