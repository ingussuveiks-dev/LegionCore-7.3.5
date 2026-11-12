"""Selected actual native racial resurrection and final-death paths.

The linked proof complements real26972 native round tests; it is not client
animation or an assertion that every pet ability is implemented.
"""
from five_ten_20261111_pe import n,ins,sites,values
import hashlib,json
from capstone import CS_OP_IMM

def pairs(body):return [(x.mnemonic,x.op_str) for x in body]
def reaches(body,start):
 table={x.address:x for x in body};pending=[start];seen=set()
 while pending:
  at=pending.pop()
  if at in seen or at not in table:continue
  seen.add(at);x=table[at]
  if x.mnemonic in ('ret','int3','ud2'):continue
  if x.mnemonic.startswith('j') and x.operands[0].type==CS_OP_IMM:pending.append(x.operands[0].imm)
  if x.mnemonic!='jmp':pending.append(x.address+x.size)
 return seen

death=ins('?Kill@PetBattleAbilityEffect@@');trigger=ins('?Trigger@PetBattleAbilityEffect@@');cast=ins('?Cast@PetBattle@@')
final,=sites(death,'?Kill@PetBattle@@');revival,=sites(death,'?Trigger@PetBattleAbilityEffect@@')
assert revival<final and {48,50,723,724}<=values(death)
assert {('mov','esi, edx'),('mov','edx, esi'),('mov','r8d, 0x2d3'),('mov','r8d, 0x2d4'),('mov','byte ptr [rbx + 0x40], 1')}<=set(pairs(death))
failed=next(i for i,x in enumerate(death)if(x.mnemonic,x.op_str)==('test','dword ptr [rcx + 0x44], 0x29a'))
dead=next(i for i,x in enumerate(death)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rax + 0x150], 0'))
for at in (failed,dead):
 branch=next(x for x in death[at+1:at+5]if x.mnemonic=='jne')
 assert death[final].address not in reaches(death,branch.operands[0].imm)
assert len(sites(death,'?SetState@PetBattleAbilityEffect@@'))==4
native_cast,=sites(trigger,'?Cast@PetBattle@@');event,=sites(trigger,'??0PetBattleEvent@@')
assert event<native_cast and ('mov','edx, 0xa') in pairs(trigger[event-5:event])
assert {('mov','edi, edx'),('mov','r8d, edx')}<=set(pairs(trigger[:event]))
assert {('mov','dword ptr [rsp + 0x30], 0xfffffffe'),('mov','dword ptr [rsp + 0x28], 0xffffffff'),
        ('mov','dword ptr [rsp + 0x20], 0xff'),('mov','r9d, 1'),('mov','edx, edi')}<=set(pairs(trigger[native_cast-8:native_cast]))
# Actual slot/racial/turn/proc/death admission precedes the effect construction.
select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@')
assert {('mov','r10d, r8d'),('mov','r12d, edx'),('cmp','r12d, 6'),('cmp','r14d, -2'),('lea','eax, [r8 - 0x2d3]'),('cmp','eax, 1'),
        ('cmp','r9d, 1'),('cmp','r15d, 0xff'),('cmp','dword ptr [rsi + 0x150], 0')}<=set(pairs(cast[:select]))
assert select<execute and pairs(cast[select-6:select-1])==[('movzx','eax, byte ptr [rsp + 0x48]'),('mov','byte ptr [rbp - 0x27], al'),('mov','eax, dword ptr [rsp + 0x64]'),('mov','dword ptr [rbp - 0x14], eax'),('mov','byte ptr [rbp - 0x18], 0')]
revive=ins('?HandleResurect@PetBattleAbilityEffect@@')
hp,=sites(revive,'?SetHealth@PetBattleAbilityEffect@@');alive,=sites(revive,'?SetState@PetBattleAbilityEffect@@')
assert hp<alive and {('imul','r8, rax'),('cmp','dx, 0x63'),('cmovg','r8d, edx')}<=set(pairs(revive))
assert {('xor','r9d, r9d'),('mov','r8d, 1')}<=set(pairs(revive[hp:alive]))
equal=next(i for i,x in enumerate(revive)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rcx + 0x60], r8d'))
branch=revive[equal+1];assert branch.mnemonic=='je'
assert revive[hp].address not in reaches(revive,branch.operands[0].imm) and revive[alive].address in reaches(revive,branch.operands[0].imm),'An unchanged native health value must still clear actual death'
health=ins('?SetHealth@PetBattleAbilityEffect@@')
assert {('cmovl','ebx, eax'),('xor','r15d, r15d'),('test','ebx, ebx'),('cmovg','r15d, ebx')}<=set(pairs(health[:30]))
terminal=ins('?Kill@PetBattle@@');assert len(sites(terminal,'?Expire@PetBattleAura@@'))==1
death_states=sites(terminal,'?SetPetState@PetBattle@@');expire,=sites(terminal,'?Expire@PetBattleAura@@')
assert len(death_states)==2 and death_states[0]<expire<death_states[1]
for identity in [('cmp','dword ptr [rcx + 0x44], 0x713'),('cmp','dword ptr [rcx + 0x428], 0x632')]:
 guard=next(i for i,x in enumerate(terminal[:death_states[0]])if(x.mnemonic,x.op_str)==identity)
 branch=terminal[guard+1]
 assert branch.mnemonic=='jne'and branch.operands[0].imm==terminal[death_states[0]+1].address,'Only actual1811/183=1586 can precommit death before controller cleanup'
assert {('cmp','byte ptr [rdx + 0x20], 0'),('cmp','dword ptr [rdx], 0x6cb'),('cmp','dword ptr [rdx + 8], esi'),('cmp','dword ptr [rdx + 0xc], esi')}<=set(pairs(terminal[:death_states[0]])),'Early state commit requires a live actual self-Crumbling receipt;ordinary final state dispatch remains after cleanup'
stores=[(i,x)for i,x in enumerate(terminal)if x.mnemonic=='mov'and x.op_str.endswith(' + 0x84], 3')]
at,store=stores[0];assert len(stores)==1
register=store.operands[0].mem.base;alias=store.reg_name(register)
saved=next(i for i,x in enumerate(terminal[:16])if(x.mnemonic,x.op_str)==('mov',alias+', rcx'))
assert not any(register in x.regs_access()[1]for x in terminal[saved+1:at]),'Final death result must be written through the preserved actual battle receiver'
expiry=ins('?HandleKill@PetBattleAbilityEffect@@');gate=next(i for i,x in enumerate(expiry)if(x.mnemonic,x.op_str)==('cmp','r8d, 0xf2'))
branch=expiry[gate+1];assert branch.mnemonic=='jne'
owner_addresses=reaches(expiry,expiry[gate+2].address)
owner_path=[x for x in expiry if x.address in owner_addresses]
assert len(sites(owner_path,'?Kill@PetBattleAbilityEffect@@'))==1
assert ('mov','edx, dword ptr [rbx + 0x3c]') in pairs(owner_path)
print('PASS actual native recipient/turn1/723+724 trigger10, strict death context, failed/dead guards, one final death dispatch,64bit bounded resurrection, retained final-death aura cleanup and owner-only242 expiry; full26972 native rounds and client checks remain separate')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'racial_abilities':[723,724],'undead_expiry':242,'native_final_death_calls':1}))
