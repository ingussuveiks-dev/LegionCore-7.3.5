"""Actual owned final-death clamp precedes native journal/final-health sends."""
from verify_pet_battle_pound_release import ins,sites,pairs,n
from pet_final_health_scope import verify
from pet_logout_pe import original_instance
import hashlib,json
verify()
rows=ins('?Finish@PetBattle@@')
gate=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('mov','ecx, dword ptr [r14 + 0x150]'))
assert pairs(rows[gate:gate+8])==[('mov','ecx, dword ptr [r14 + 0x150]'),('test','ecx, ecx'),('jne',rows[gate+2].op_str),('cmp','dword ptr [r14 + 0x60], ecx'),('jge',rows[gate+4].op_str),('mov','dword ptr [r14 + 0x60], r9d'),('cmp','byte ptr [rsp + 0x41], 0'),('jne',rows[gate+7].op_str)],'Actual final-dead or negative health reaches zero before penalty gate'
assert rows[gate+2].operands[0].imm==rows[gate+5].address and rows[gate+4].operands[0].imm==rows[gate+6].address,'Death must take the zero store;only alive nonnegative health skips it'
zero=max(i for i,x in enumerate(rows[:gate])if(x.mnemonic,x.op_str)==('xor','r9d, r9d'))
assert all(not x.op_str.startswith(('r9d,','r9,','r9b,'))or x.mnemonic in('cmp','test')for x in rows[zero+1:gate+5]),'The actual clamp value remains the established zero'
journal=next(i for i in sites(rows,'?UpdateOriginalInstance@BattlePetInstance@@')if i>gate)
assert gate+6<journal
# The second life-state test is the XP admission guard. ECX remains the
# original actual dead state along the non-forfeit path to that guard.
xp=next(i for i,x in enumerate(rows[gate+6:journal],gate+6)if(x.mnemonic,x.op_str)==('test','ecx, ecx'))
assert rows[xp+1].mnemonic=='jne'and rows[xp+1].operands[0].imm==rows[journal-2].address,'Final death skips level-up healing and goes directly to journal receiver setup'
assert pairs(rows[journal-2:journal])==[('mov','rdx, r12'),('mov','rcx, r14')],'The journal call uses the actual player and normalized pet'
copy=original_instance()
assert {('mov','eax, dword ptr [rdx + 0x60]'),('mov','rcx, qword ptr [rdx + 0x480]'),('mov','dword ptr [rcx + 0x60], eax'),('mov','rax, qword ptr [rdx + 0x480]'),('mov','byte ptr [rax + 0x120], 1')}<=set(pairs(copy)),'Independent bounded journal leaf copies actual normalized health and marks persistence'
assert sites(rows,'?SendBattlePetUpdates@WorldSession@@')and sites(rows,'?SendPetBattleFinalRound@WorldSession@@'),'Real journal call still precedes actual native notifications/final sender'
sender=ins('?SendPetBattleFinalRound@WorldSession@@')
assert any(x.mnemonic=='mov'and x.op_str.endswith(', dword ptr [rcx + 0x60]')for x in sender),'Unchanged native final packet reads the actual pet Health'
print('PASS linked final owned dead-state/negative-health zeroing before ordinary penalty,journal persistence and actual final sender;living XP/racials and all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':102,'scope':'actual resolved Player-owned Finish; no intermediate Kill or NPC/capture rewrite'}))
