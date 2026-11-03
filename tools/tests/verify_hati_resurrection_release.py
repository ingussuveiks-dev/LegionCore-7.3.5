"""Broken Bond expiration may resummon only for an actual selected Player pet."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values
from monk_intro_instance_pe import reachable
prior=runpy.run_path(str(Path(__file__).with_name('verify_hati_primary_release.py')))
n=prior['n'];reaction=prior['p']['p'];table=reaction['table'];refs=reaction['refs']
ins=n['instructions'];body=ins('?OnRemove@spell_hun_broken_bond_AuraScript@spell_hun_broken_bond@@')
pairs=[(o.mnemonic,o.op_str) for o in body]
factory=ins('?GetAuraScript@spell_hun_broken_bond@@')
register=ins('?Register@spell_hun_broken_bond_AuraScript@spell_hun_broken_bond@@')
vt,slots=table('??_7spell_hun_broken_bond_AuraScript@spell_hun_broken_bond@@6B@',16)
assert vt in refs(factory)
assert n['symbol']('?Register@spell_hun_broken_bond_AuraScript@spell_hun_broken_bond@@') in [n['resolve'](v) for v in slots if v>=n['base']]
assert n['symbol']('?OnRemove@spell_hun_broken_bond_AuraScript@spell_hun_broken_bond@@') in refs(register)
vt,slots=table('??_7spell_hun_broken_bond@@6B@',3)
assert vt in refs(ins('?AddSC_hunter_spell_scripts@@'))
assert n['resolve'](slots[2])==n['symbol']('?GetAuraScript@spell_hun_broken_bond@@')
app,=sites(n,body,'?GetTargetApplication@AuraScript@@')
caster,=sites(n,body,'?GetCaster@AuraScript@@')
pet,=sites(n,body,'?GetPet@Player@@')
aura,=sites(n,body,'?GetAura@Unit@@')
cast,=sites(n,body,'?CastSpell@Unit@@')
despawn,=sites(n,body,'?DespawnOrUnsummon@Creature@@')
assert app<caster<despawn<pet<aura<cast
assert pairs[app+1]==('cmp','byte ptr [rax + 0x10], 4') and body[app+2].mnemonic=='jne'
other_modes=reachable(body,body[app+2].operands[0].imm)
assert all(body[i].address not in other_modes for i in (caster,despawn,pet,aura,cast))
guard=pet-3
assert pairs[guard:pet]==[('cmp','byte ptr [rsi + 0x16], 4'),('jne',body[guard+1].op_str),('mov','rcx, rsi')]
assert pairs[pet+1:pet+3]==[('test','rax, rax'),('je',body[guard+1].op_str)]
failure=reachable(body,body[guard+1].operands[0].imm)
assert all(body[i].address not in failure for i in (pet,aura,cast))
for mandatory in (guard,pet+1):
 assert body[cast].address not in reachable(body,body[0].address,{body[mandatory].address})
assert {100324,0x1850,1000,197344}<=values(body)
assert ('mov','r8d, dword ptr [rax + 0x12c]') in pairs[aura:cast]
assert {('mov','rdx, rsi'),('mov','rcx, rsi'),('mov','r9b, 1')}<=set(pairs[aura:cast])
assert {('mov','edx, 0x3e8'),('mov','rcx, rdi')}<=set(pairs[despawn-5:despawn])
assert pairs[caster+1:caster+5]==[('mov','rsi, rax'),('test','rax, rax'),('je',body[caster+3].op_str),('cmp','dword ptr [rax + 0xa18], 0')]
assert ('cmp','dword ptr [rdi + 0xa18], ebp') in pairs[:despawn]
alive=next(i for i,pair in enumerate(pairs) if pair==('cmp','dword ptr [rdi + 0xa18], ebp'))
assert body[alive+1].mnemonic=='je' and body[cast].address not in reachable(body,body[alive+1].operands[0].imm)
for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?SetMinion@Unit@@','?SetPet@Player@@'):
 assert not sites(n,body,forbidden)
print('PASS registered Broken Bond after-remove: actual expiry4/living-caster/five-Hati alive guard and1000ms corpse cleanup retained; actual Player type4 and nonnull native GetPet dominate197344/custom-data/self-triggered resummon. Retained registered primary/passive/owner checks pass. Client death/revive remains separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'spell':211117,'native_duration_ms':30000,'required_primary':'Player::GetPet'}))
