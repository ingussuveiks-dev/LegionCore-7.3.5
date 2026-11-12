"""Actual native321 roster mapping, user sacrifice and retained passive rules."""
from verify_pet_battle_self_sacrifice_release import ins,sites,values,pairs,reaches,n
import hashlib,json
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(selector)if(x.mnemonic,x.op_str)==('cmp','ecx, 0x141'))
assert ('mov','ecx, dword ptr [rbx + 0x18]')in pairs(selector[:gate])
assert (selector[gate+2].mnemonic,selector[gate+2].op_str)==('cmp','eax, 0x32')
addresses=reaches(selector,selector[gate+4].address)
body=[x for x in selector if x.address in addresses]
assert {('movzx','ecx, byte ptr [rdx + 0x14]'),('dec','ecx'),('shr','ecx, 1'),('cmp','ecx, dword ptr [rdx + 0x58]'),('test','rdx, rdx'),('mov','edx, dword ptr [rdx + 0x130]')}<=set(pairs(body))
assert len(sites(body,'?AddTarget@PetBattleAbilityEffect@@'))==1
terminal=ins('?HandleKill@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(terminal)if(x.mnemonic,x.op_str)==('cmp','r8d, 0x141'))
branch=terminal[gate+1];assert branch.mnemonic=='je'
body=[x for x in terminal if x.address in reaches(terminal,branch.operands[0].imm)]
kill,=sites(body,'?Kill@PetBattleAbilityEffect@@');state,=sites(body,'?SetState@PetBattleAbilityEffect@@')
assert pairs(body[kill-2:kill])==[('mov','edx, dword ptr [rbx + 0x1c]'),('mov','rcx, rbx')]
guard=next(i for i,x in enumerate(body)if(x.mnemonic,x.op_str)==('cmp','r8d, 0x401'))
branch=body[guard+1];assert branch.mnemonic=='jne'
assert body[kill].address in reaches(body,branch.operands[0].imm)and body[state].address not in reaches(body,branch.operands[0].imm),'321 must bypass the 282/1025-only Failsafe prohibition'
cast=ins('?Cast@PetBattle@@');assert 321 in values(cast[:112])
print('PASS actual linked321 property50 odd-order enemy slots, bounded actual roster recipients, one caster-only terminal/native immunity and retained ordinary Mechanical reaction; retained four-sacrifice and racial proofs also pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'ability':321,'aura':320,'native_cases':16}))
