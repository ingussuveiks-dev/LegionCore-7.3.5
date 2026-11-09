"""Actual retained Release paths; unused memory importer is covered by source/library checks."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_fifth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];ref=p['ref'];values=p['values']
def op(rows,m,s):return any(x.mnemonic==m and x.op_str==s for x in rows)
def registered(cls,method):
 rows=ins('?'+method+'@'+cls);assert ref(ins('?Register@'+cls),n['symbol']('?'+method+'@'+cls));return rows
rows=registered('spell_q_artifact_trait_unlock_SpellScript@spell_q_artifact_trait_unlock@@','HandleOnCast')
assert sites(rows,'?GetUInt64Value@Object@@') and op(rows,'mov','edx, 0x52') and op(rows,'cvttss2si','r8, xmm0')
rows=ins('?SearchChainTargets@Spell@@')
assert sites(rows,'?GetMaxHealth@Unit@@') and sites(rows,'?GetHealth@Unit@@') and op(rows,'sub','r15, rax') and op(rows,'cmp','r15, r12')
assert sites(rows,'?IsWithinLOSInMap@WorldObject@@') and sites(rows,'?IsWithinDist@WorldObject@@')
rows=ins('?RegenerateHealth@Player@@');assert sites(rows,'?ModifyHealth@Unit@@') and op(rows,'cvttss2si','rdx, xmm10')
rows=ins('?ResurrectPlayer@Player@@');assert sites(rows,'?SetHealth@Unit@@') and op(rows,'cvttss2si','rdx, xmm0')
rows=registered('spell_sha_ancestral_protection@@','Absorb');assert op(rows,'sub','rcx, rax') and sum(op([x],'cmp','rcx, rax') for x in rows)==2
cls='spell_betrug_mighty_smash_SpellScript@spell_betrug_mighty_smash@@'
rows=registered(cls,'HandleOnCast');at,=sites(rows,'?CastSpell@Unit@@')
assert op(rows[at:at+5],'sete','al') and op(rows,'mov','byte ptr [rdi + 0x1fa], al')
rows=registered(cls,'HandleOnHit');at=sites(rows,'?MoveJump@MotionMaster@@')[0]
assert op(rows[:at],'cmp','byte ptr [rdi + 0x1fa], 0')
rows=ins('?EnterEvadeMode@npc_orinoko_tuskbreakerAI@');assert sites(rows,'?DespawnOrUnsummon@Creature@@') and sites(rows,'?EnterEvadeMode@CreatureAI@@')
rows=ins('?EnterEvadeMode@CreatureAI@@');at,=sites(rows,'?KillAllEvents@EventProcessor@@');assert op(rows[max(0,at-5):at],'xor','edx, edx')
rows=ins('?JustDied@npc_orinoko_tuskbreakerAI@')
for dep in ['?DespawnOrUnsummon@Creature@@','?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?GroupEventHappens@Player@@']:assert sites(rows,dep)
rows=ins('?DumpTable@PlayerDumpWriter@@');assert len(sites(rows,'?GetUInt64@Field@@'))>=5
rows=ins('?ChangeGuid@@') # actual signature carries uint64 seed (_K), rather than I
assert '_K' in next(k for k in n['symbols'] if k.startswith('?ChangeGuid@@'))
rows=ins('?ChangePetReference@@');assert sites(rows,'?GeneratePetNumber@ObjectMgr@@') and sites(rows,'?ChangeNth@@')
rows=ins('?LoadDump@PlayerDumpReader@@');assert len(sites(rows,'?ChangePetReference@@'))==2
rows=ins('?FindNth@@');assert op(rows,'cmp','r15d, 1') and op(rows,'cmp','rax, -1') and op(rows,'cmp','byte ptr [rcx + rax], 0x5c') and op(rows,'lea','rbx, [rcx + 2]')
print('PASS actual Release sixth ten:64bit AP/Chain/regen/resurrection;Ancestral overflow-safe bound;Betrug accepted-pad gate;Orinoko deletion/owner credit and shared evade deferred event deletion;dump64bit collector/seed,pet parent mapping and escaped parser. Prior57 paths retained;unused memory importer source/library covered,not linked;native/model/client scopes distinct.')
