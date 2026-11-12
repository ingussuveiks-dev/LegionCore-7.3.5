"""Native1907 owner-bound end check, finite lifetime and actual death receipts."""
from verify_pet_battle_hourglass_release import ins,sites,pairs,reaches,n
from pet_cheat_death_scope import verify
from capstone import CS_OP_IMM
import hashlib,json,re
verify()
cast=ins('?Cast@PetBattle@@');select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@')
assert {('cmp','r8d, 0x773'),('cmp','esi, 7'),('mov','byte ptr [rsp + 0x40], dil'),('cmp','byte ptr [r9 + 0x129], 0'),('cmp','byte ptr [r9 + 0x128], 0')}<=set(pairs(cast[:select])),'Actual issued1907 owner context rejects both captured flags before dispatch'
assert {('mov','eax, dword ptr [rbp + 0x7f]'),('cmp','byte ptr [rsp + 0x40], 0'),('mov','dword ptr [rsp + 0x48], eax'),('lea','rax, [rsp + 0x48]')}<=set(pairs(cast[select:execute])),'Known1907 self rows use the actual holder, retaining the original source'
process=ins('?Process@PetBattleAura@@');expire,=sites(process,'?Expire@PetBattleAura@@')
assert {('cmp','r8d, 0x773'),('cmp','dword ptr [rcx + 0x1c], 0'),('cmp','dword ptr [rcx + 0x18], 0'),('cmp','dword ptr [rcx + 0x10], 1')}<=set(pairs(process[:expire]))
gate=next(i for i,x in enumerate(process[:expire])if(x.mnemonic,x.op_str)==('cmp','dword ptr [rcx + 0x10], 1'))
branch=process[gate+1];assert branch.mnemonic=='je'
assert process[expire].address not in reaches(process,branch.operands[0].imm),'Native duration0 waits for its one round-end opportunity'
rounds=ins('?ProceedRound@PetBattle@@');calls=sites(rounds,'?Cast@PetBattle@@')
end=next(i for i in calls if ('mov','dword ptr [rsp + 0x20], 7')in pairs(rounds[i-9:i]))
expire=next(i for i in sites(rounds,'?Expire@PetBattleAura@@')if i>end)
assert {('cmp','byte ptr [rbx + 0x20], 0'),('cmp','dword ptr [rbx], 0x773'),('cmp','dword ptr [rbx + 0x1c], 0'),('cmp','dword ptr [rbx + 0x18], 0')}<=set(pairs(rounds[end:expire])),'Issued native0 controller expires even when its living check stops before removal'
dispatch=ins('?Execute@PetBattleAbilityEffect@@')
assert {('cmp','eax, 0x773'),('cmp','dword ptr [rbx + 0x4c], ecx'),('test','dl, dl')}<=set(pairs(dispatch)),'Only the actual beneficial holder bypasses hostile-target gating'
revive=ins('?HandleResurect@PetBattleAbilityEffect@@');alive,=sites(revive,'?SetState@PetBattleAbilityEffect@@')
assert {('cmp','dword ptr [rcx + 0x18], 0x773'),('cmp','dword ptr [rcx + 0x4c], eax'),('cmp','byte ptr [rdx + 0x129], 0'),('cmp','byte ptr [rdx + 0x128], 0')}<=set(pairs(revive[:alive]))
symbols={}
with (n['args'].release/'worldserver.map').open(errors='replace')as f:
 for line in f:
  m=re.match(r'\s+\w+:\w+\s+(__std_remove_1|__std_find_trivial_1)\s+([0-9a-fA-F]{16})\s+f',line)
  if m:symbols[m[1]]=n['resolve'](int(m[2],16))
assert len(symbols)==2
find,= [i for i,x in enumerate(revive)if x.mnemonic=='call'and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==symbols['__std_find_trivial_1']]
remove,= [i for i,x in enumerate(revive)if x.mnemonic=='call'and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==symbols['__std_remove_1']]
assert alive<find<remove
assert {('test','al, al'),('test','sil, sil'),('mov','r8d, dword ptr [rdi + 0x3c]'),('mov','rsi, qword ptr [rbp + 0xa8]'),('mov','rcx, qword ptr [rbp + 0xa0]')}<=set(pairs(revive[alive:find])),'Only a successful actual1907 revival searches its own current death vector'
assert {('movzx','r8d, byte ptr [rdi + 0x3c]'),('mov','rdx, rsi'),('mov','rcx, rax')}<=set(pairs(revive[find:remove])),'Native byte-vector removal names only the actual revived holder'
assert {('cmp','qword ptr [rbp + 0xa0], rax'),('cmp','dword ptr [rax + 0x84], 3'),('cmp','byte ptr [rcx + 0x129], 0'),('cmp','byte ptr [rcx + 0x128], 0'),('add','rax, 0x10'),('mov','dword ptr [rax + 0x84], 0')}<=set(pairs(revive[remove:])),'Other death receipts or captured pets retain the actual terminal result'
death=ins('?Kill@PetBattle@@');expire,=sites(death,'?Expire@PetBattleAura@@')
gate=next(i for i,x in enumerate(death[:expire])if(x.mnemonic,x.op_str)==('cmp','eax, 0x773'))
assert death[gate+1].mnemonic=='je'and death[gate+1].operands[0].imm==death[expire+1].address
print('PASS actual linked1907 issued owner/source and captured guards, native0 expiry deferral/end cleanup, actual50percent resurrection and holder-only death removal/result restoration; all retained native pet linked scopes pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'root':1906,'controller':1907,'native_cases':28}))
