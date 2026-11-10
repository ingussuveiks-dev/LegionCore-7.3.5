"""Native shared squad deaths/ownership and generation checks in deployed PE."""
from pathlib import Path
import runpy,re,struct
p=runpy.run_path(str(Path(__file__).with_name('verify_twin_emperors_release.py')))
loader=p
while 'n' not in loader:loader=loader['p']
n,ins,sites,values=loader['n'],p['ins'],p['sites'],p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
table='??_7npc_generic_soldierAI@npc_generic_soldier@@6B@'
text=(n['args'].release/'worldserver.map').read_text(errors='replace');tables=re.findall(r'\s+\w+:\w+\s+'+re.escape(table)+r'\s+([0-9a-fA-F]{16})',text);assert len(tables)==1
address=int(tables[0],16)
for slot,method in [(0xa8,'DamageTakenAfterScaling'),(0x130,'JustDied')]:
 target=struct.unpack_from('<Q',n['binary'],n['offset'](address)+slot)[0]
 assert n['resolve'](target)==n['symbol']('?'+method+'@npc_generic_soldierAI@'),'real soldier virtual slot '+method
assert not any(k.startswith('?DamageTaken@npc_generic_soldierAI@') for k in n['symbols']),'pre-scaled squad callback returned'
r=ins('?DamageTakenAfterScaling@npc_generic_soldierAI@');assert sites(r,'?SendDamageSoldiers@npc_soldierAI@@')
r=ins('?SendDamageSoldiers@npc_soldierAI@@');assert sites(r,'?LivingSquad@npc_soldierAI@@') and sites(r,'?GetAffectingPlayer@Unit@@') and len(sites(r,'?Kill@Unit@@'))==1 and sites(r,'?SendDiedSoldiers@npc_soldierAI@@')
assert op(r,'sub','rax, rbp') and len([x for x in r if x.mnemonic=='cmp' and x.op_str=='rbp, rax'])>=3 and sites(r,'?SetHealth@Unit@@'),'full-width bounded same-amount mirror and source/other lethal gates'
assert op(r,'cmp','qword ptr [rax + 0x9c0], rdi') and op(r,'cmp','byte ptr [rax + 0x858], 0'),'retain direct native spell death,explicit fallback for suspended AI'
d=ins('?SendDiedSoldiers@npc_soldierAI@@');assert op(d,'cmp','byte ptr [rcx + 0x38], 0') and op(d,'mov','byte ptr [rcx + 0x38], 1') and len(sites(d,'?Kill@Unit@@'))==1 and sites(d,'?LivingSquad@npc_soldierAI@@')
assert op(d,'test','cl, al') and op(d,'or','al, cl') and op(d,'mov','byte ptr [rbx + 0x1da], al') and {62408,65499,65500,122354,125873}<=set(values(d)),'inlined boss per-type generation checkpoint and native recklessness dispatch'
r=ins('?LivingSquad@npc_soldierAI@@');assert {10,13,16,62408,65499,65500}<=set(values(r)) and op(r,'cmp','rax, qword ptr [r14 + 0x20]') and op(r,'cmp','dword ptr [rax + 0xa18], 0') and op(r,'cmp','edi, eax'),'only live/same-entry/nonself actual instance slots'
r=ins('?Reset@npc_generic_soldierAI@');assert op(r,'mov','byte ptr [rcx + 0x38], 0'),'retry clears actor checkpoint'
r=ins('?DoAction@npc_generic_soldierAI@');at,=sites(r,'?DoResetThreat@ScriptedAI@@');guards=[i for i,x in enumerate(r[:at]) if x.mnemonic=='cmp' and '0xa18]' in x.op_str];assert len(guards)==1
assert r[guards[0]+1].mnemonic=='jne' and r[guards[0]+1].operands[0].imm>r[at].address,'dead soldier must return before native strike threat reset'
r=ins('?JustSummoned@boss_lord_meljarakAI@');assert any(x.mnemonic=='and' and '0x1da]' in x.op_str for x in r),'actual native replacement rearms only its squad generation'
print('PASS linked Meljarak real after-scaling/death vtable,full-width bounded shared damage,native ownership/Kill/fallback,actor and boss-generation one-use gates,live/same-type instance lookup and reset. Complete Twin/health/Freya/counter/Tsulong/Rook/prior367 chain retained;native/client scopes separate.')
