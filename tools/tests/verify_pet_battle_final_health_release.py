"""Actual owned final-death clamp precedes native journal/final-health sends."""
from verify_pet_battle_pound_release import ins,sites,pairs,n
from pet_final_health_scope import verify
import hashlib,json
verify()
rows=ins('?Finish@PetBattle@@')
gate=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('mov','ecx, dword ptr [rdi + 0x150]'))
assert pairs(rows[gate:gate+8])==[('mov','ecx, dword ptr [rdi + 0x150]'),('test','ecx, ecx'),('jne',rows[gate+2].op_str),('cmp','dword ptr [rdi + 0x60], ecx'),('jge',rows[gate+4].op_str),('mov','dword ptr [rdi + 0x60], r9d'),('cmp','byte ptr [rsp + 0x4c], 0'),('jne',rows[gate+7].op_str)],'Actual final-dead or negative health reaches zero before penalty gate'
assert rows[gate+2].operands[0].imm==rows[gate+5].address and rows[gate+4].operands[0].imm==rows[gate+6].address,'Death must take the zero store;only alive nonnegative health skips it'
zero=max(i for i,x in enumerate(rows[:gate])if(x.mnemonic,x.op_str)==('xor','r9d, r9d'))
assert all(not x.op_str.startswith(('r9d,','r9,','r9b,'))or x.mnemonic in('cmp','test')for x in rows[zero+1:gate+5]),'The actual clamp value remains the established zero'
journal=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('mov','rcx, qword ptr [rdi + 0x480]'))
assert gate+6<journal
# The second life-state test is the XP admission guard. ECX remains the
# original actual dead state along the non-forfeit path to that guard.
xp=next(i for i,x in enumerate(rows[gate+6:journal],gate+6)if(x.mnemonic,x.op_str)==('test','ecx, ecx'))
assert rows[xp+1].mnemonic=='jne'and rows[xp+1].operands[0].imm==rows[journal].address,'Final death skips level-up healing and goes directly to journal copy'
copies=[i for i in range(journal,len(rows)-2)if pairs(rows[i:i+3])==[('mov','rcx, qword ptr [rdi + 0x480]'),('mov','eax, dword ptr [rdi + 0x60]'),('mov','dword ptr [rcx + 0x60], eax')]]
assert len(copies)==1 and sites(rows,'?SendBattlePetUpdates@WorldSession@@')and sites(rows,'?SendPetBattleFinalRound@WorldSession@@'),'Real inlined journal update copies the normalized health before real updates/final sender'
assert {('mov','rax, qword ptr [rdi + 0x480]'),('mov','byte ptr [rax + 0x120], 1')}<=set(pairs(rows[copies[0]:])),'The actual journal object is marked for persistence'
sender=ins('?SendPetBattleFinalRound@WorldSession@@')
assert any(x.mnemonic=='mov'and x.op_str.endswith(', dword ptr [rcx + 0x60]')for x in sender),'Unchanged native final packet reads the actual pet Health'
print('PASS linked final owned dead-state/negative-health zeroing before ordinary penalty,journal persistence and actual final sender;living XP/racials and all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':102,'scope':'actual resolved Player-owned Finish; no intermediate Kill or NPC/capture rewrite'}))
