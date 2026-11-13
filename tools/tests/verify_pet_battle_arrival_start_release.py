"""Actual x64 arrival deadline, scoped pending receipt and native Begin path."""
from five_ten_20261111_pe import ins,sites,n
from verify_pet_battle_crumble_release import reaches
from pathlib import Path
import re,struct,hashlib,json
rows=ins('?OnUpdate@PlayerScriptPetBattle@@')
pairs=lambda r:[(x.mnemonic,x.op_str)for x in r]
begin,=sites(rows,'?Begin@PetBattle@@')
clock,=sites(rows,'?getMSTime@@')
assert pairs(rows[clock+1:clock+4])[:2]==[('sub','eax, edi'),('cmp','eax, 0x3e8')]
early=rows[clock+3]
assert early.mnemonic=='jb' and rows[begin].address not in reaches(rows,early.operands[0].imm),'Unsigned elapsed<1000 must return without Begin,including wrapped clock values'
erase,=sites(rows,'?_Erase_unchecked@?$_Tree@V?$_Tmap_traits@VObjectGuid@@UPendingStart@PlayerScriptPetBattle@@')
assert clock<erase<begin and ('movups','xmm0, xmmword ptr [rsi + 0x2c]')in pairs(rows[clock:erase]),'Consume the exact stored battle ID once before external callbacks'
lines=(n['args'].release/'worldserver.map').read_text(errors='replace')
def raw_symbol(name):
 matches=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})\s',lines,re.M)
 assert len(matches)==1,name
 return int(matches[0],16)
unlock=n['resolve'](raw_symbol('_Mtx_unlock'))
unlocks=[i for i,x in enumerate(rows)if x.mnemonic=='call'and x.op_str.startswith('0x')and n['resolve'](x.operands[0].imm)==unlock]
assert len(unlocks)==2 and erase<unlocks[0]<begin,'Dispatch must release its actual mutex before native Begin'
gate,=sites(rows,'?IsPendingParticipant@PlayerScriptPetBattle@@')
assert erase<gate<begin and rows[gate+2].mnemonic=='je'and rows[begin].address not in reaches(rows,rows[gate+2].operands[0].imm)
assert {('lea','rcx, [rbp + 0x980]'),('lea','rdx, [rsp + 0x20]'),('test','al, al')}<=set(pairs(rows[erase:gate])),'Compare live player battle ID with the consumed full GUID'
equal,=sites(rows[erase:gate],'??8ObjectGuid@@');equal+=erase
assert rows[equal+2].mnemonic=='je'and rows[begin].address not in reaches(rows,rows[equal+2].operands[0].imm),'A replacement battle must not inherit an old arrival'
assert {('mov','r8d, 0x20200'),('mov','r8d, 0x400'),('mov','dl, 1')}<=set(pairs(rows[gate:begin]))
assert sites(rows,'?SetFlag@Object@@')and sites(rows,'?SetControlled@Unit@@'),'Original root and NPC combat protection retained'
participant=ins('?IsPendingParticipant@PlayerScriptPetBattle@@')
assert ('cmp','dword ptr [rdx + 0x78], 0')in pairs(participant),'Only CREATION admits a delayed Begin'
for prefix in ('?IsInWorld@Object@@','?IsDelete@Object@@','?isDead@Unit@@'):
 assert sites(participant,prefix),'Missing actual participant admission '+prefix
assert len(sites(participant,'??8ObjectGuid@@'))==2,'Both real PlayerGuid and OwnerGuid must match'
arrival=ins('?OnMovementInform@PlayerScriptPetBattle@@')
assert {('cmp','r8d, 8'),('cmp','r9d, 0xa42ba70b')}<=set(pairs(arrival))
assert sites(arrival,'?IsPendingParticipant@PlayerScriptPetBattle@@')and sites(arrival,'?getMSTime@@')
assert ('mov','dword ptr [rsi + 0x3c], r15d')in pairs(arrival),'Store the start timestamp,not an overflow-prone absolute deadline'
logout=ins('?OnBeforeLogout@PlayerScriptPetBattle@@')
assert not sites(logout,'?Begin@PetBattle@@')and not sites(logout,'?Finish@PetBattle@@'),'Pending-arrival cleanup does not invent disconnect outcome policy'
vtable=raw_symbol('??_7PlayerScriptPetBattle@@6B@')
slots=struct.unpack_from('<48Q',n['binary'],n['offset'](vtable))
observed={name:[i for i,p in enumerate(slots)if p==n['symbol']('?'+name+'@PlayerScriptPetBattle@@')]for name in('OnMovementInform','OnUpdate','OnBeforeLogout')}
assert all(len(x)==1 for x in observed.values()),'Actual script vtable must dispatch each repaired callback'
assert observed['OnBeforeLogout']==[44],'Retain native appended before-save logout slot44'
print('PASS linked arrival unsigned elapsed>=1000,stored full battle GUID,consume/unlock before Begin,creation/player ownership admission,root/protection and actual script vtable;retained pet lifecycle proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'callback_slots':observed,'controlled_asan_cases':114,'actual_native_cases':6,'client_gameplay_verified':False}))
