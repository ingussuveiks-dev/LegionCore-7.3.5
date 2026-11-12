from linked_player_abi import creature_loot_operand
"""Native scaled shared damage, owner admission and exactly-once paired death."""
from pathlib import Path
import re,runpy,struct
from capstone import CS_OP_IMM
p=runpy.run_path(str(Path(__file__).with_name('verify_health_delta_release.py')))
loader=p
while 'n' not in loader:loader=loader['p']
n,ins,sites,values=loader['n'],p['ins'],p['sites'],p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
after=ins('?DamageTakenAfterScaling@boss_twinemperorsAI@@');death=ins('?JustDied@boss_twinemperorsAI@@')
assert not any(k.startswith('?DamageTaken@boss_twinemperorsAI@@') for k in n['symbols']),'old pre-scaling mirror override returned'
target=n['symbol']('?DamageTakenAfterScaling@boss_twinemperorsAI@@')
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
for cls in ['boss_veknilash','boss_veklor']:
 name='??_7'+cls+'AI@'+cls+'@@6B@'
 tables=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',maptext);assert len(tables)==1
 table=int(tables[0],16) # Vtables are data symbols,not the loader's code-only map.
 assert n['resolve'](struct.unpack_from('<Q',n['binary'],n['offset'](table)+0xa8)[0])==target,'real emperor vtable must dispatch the shared after-scaling hook'
calls={n['resolve'](x.operands[0].imm) for x in after if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM}
for operation in ['eval_multiply','eval_divide']:
 assert any(operation in name and '$0IA@' in name and n['resolve'](address) in calls for name,address in n['symbols'].items()),'actual128bit '+operation
assert {0x38,0x40}<=set(values(after)) and len(sites(after,'?Kill@Unit@@'))==2
assert len(sites(after,'?RegisterSharedDamage@boss_twinemperorsAI@@'))==4 and sites(after,'?CopySharedEligibility@boss_twinemperorsAI@@')
assert op(after,'cmp','dword ptr [rdx + 0xa18], 0'),'explicit native Kill fallback when brother AI did not propagate'
assert len(sites(death,'?Kill@Unit@@'))==1 and not sites(death,'?setDeathState@Creature@@') and not sites(death,'?SetHealth@Unit@@'),'native death only,no manual corpse/loot flag shortcut'
assert op(death,'cmp','byte ptr [rcx + 0x51], 0') and op(death,'mov','byte ptr [rcx + 0x51], 1'),'one-use death checkpoint'
first=ins('?Reset@boss_veknilashAI@boss_veknilash@@');second=ins('?Reset@boss_veklorAI@boss_veklor@@')
assert op(first,'mov','word ptr [rbx + 0x50], 0') and op(second,'mov','word ptr [rbx + 0x50], di') and op(second,'xor','edi, edi'),'both original resets clear yell+death checkpoint'
tag=ins('?RegisterSharedDamage@boss_twinemperorsAI@@')
assert sites(tag,'?GetAffectingPlayer@Unit@@') and sites(tag,'?hasLootRecipient@Creature@@') and sites(tag,'?SetLootRecipient@Creature@@')
assert op(tag,'cmp','rcx, rdi') and op(tag,'cmovb','edi, eax') and op(tag,'mov',creature_loot_operand('dword ptr [rbx + 0x39b8], edi')),'native saturating positive contribution'
brother=ins('?GetLivingBrother@boss_twinemperorsAI@@')
assert op(brother,'cmp','rax, qword ptr [rbx + 0x20]') and op(brother,'cmp','dword ptr [rax + 0xa18], 0') and 15276 in values(brother),'real alive opposite entry,not self/wrong actor'
damage=ins('?DealDamage@Unit@@');guards=[i for i,x in enumerate(damage) if x.mnemonic=='cmp' and x.op_str=='dword ptr [rbx + 0xa18], 0'];assert len(guards)==3
ends=set()
for at in guards:
 assert damage[at+1].mnemonic=='jne';end=damage[at+1].operands[0].imm;ends.add(end)
 assert any(x.mnemonic=='call' for x in damage[at-3:at]) and op(damage[at-3:at],'cmp','dword ptr [rsp + 0x60], 0'),'guard immediately follows native instance/current/suspended dispatch'
assert len(ends)==1;end=ends.pop();row=next(x for x in damage if x.address==end)
assert row.mnemonic=='mov' and row.op_str=='eax, dword ptr [rsp + 0x60]','retain actual normalized damage result after callback-owned death'
print('PASS linked Twin Emperors:both real vtables after-scaling,128bit proportional operations/native64 fields,GetAffectingPlayer and native tap/saturation,two native Kill endpoints plus replay/reset/fallback,alive/opposite/self guards,and three core post-callback life checks returning actual damage. Complete prior health/Freya/counter/Tsulong/Rook/367 chain retained;native source/data/gameplay scopes remain distinct.')
