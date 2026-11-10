"""Selected linked timers, health-width operations, empty selection and queued origins."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_fifteenth_ten_release.py')))
n=p['n'];ins=p['ins'];sites=p['sites'];values=p['values'];ref=p['ref'];expanded=p['expanded'];callbacks=p['callbacks'];map_guards=p['map_guards']
def op(r,m,s):return any(x.mnemonic==m and x.op_str==s for x in r)
assert op(ins('?Reset@npc_moonkin_hatchling_pet@@'),'mov','qword ptr [rcx + 0x38], 0x3e8'),'both adjacent timers initialized1000/0'
for cls in ['boss_leviathan_mkAI','boss_vx_001AI','boss_aerial_unitAI']:
 r=ins('?DoAction@'+cls+'@');at=next(i for i,x in enumerate(r)if x.mnemonic=='shr'and x.op_str=='rax, 1');assert sites(r[at:at+8],'?SetHealth@Unit@@')and op(r[at:at+8],'mov','rdx, rax'),'native64 half health '+cls
for pre in ['?OnCreatureCreate@instance_crypt_of_forgotten_kings_InstanceMapScript@','?OnUnitEnter@sat_ragefire_emberstrikesAI@']:
 r=ins(pre);assert op(r,'cvttss2si','rdx, xmm0')and sites(r,'?SetHealth@Unit@@'),'native64 float conversion '+pre
r=ins('?HandlePeriodicTick@spell_sinestra_mana_barrier_AuraScript@');assert op(r,'sub','rdi, rax')and op(r,'mov','rdx, rdi')and sites(r,'?ModifyHealth@Unit@@')and op(r,'cmp','rcx, rdi')and op(r,'cmovl','edi, eax')and sites(r,'?ModifyPower@Unit@@'),'full heal with bounded32 actual mana'
r=ins('?HandlePeriodicTick@spell_sinestra_pyrrhic_focus_AuraScript@');assert op(r,'cvttss2si','rdi, xmm0')and op(r,'cmp','rax, rdi')and op(r,'neg','rdi')and op(r,'mov','rdx, rdi')and sites(r,'?ModifyHealth@Unit@@')and sites(r,'?Kill@Unit@@'),'full-width native drain and separate lethal boundary'
r=ins('?SpellHit@boss_twinemperorsAI@@');assert op(r,'test','rsi, rsi')and op(r,'divsd','xmm0, xmm1')and op(r,'comisd','xmm0, xmm1')and op(r,'cvttsd2si','rbx, xmm0')and sites(r,'?SetHealth@Unit@@'),'bounded double compensation before uint64 heal'
r=ins('?UpdateAI@boss_anubarak_trialAI@');at=next(i for i,x in enumerate(r)if x.mnemonic=='mov'and x.op_str=='rdx, qword ptr [rdi + 0x178]');assert r[at+1].mnemonic=='test'and r[at+2].mnemonic=='je';assert {66340,4000}<=values(r[at:at+65])and sites(r[at:at+65],'?ScheduleEvent@EventMap@@'),'empty native list bypasses selection but still retries'
army,=callbacks('instance_army_training_InstanceMapScript','onScenarionNextStep');r=expanded(army);map_guards(r);assert sites(r,'?TeleportTo@Player@@'),'guarded original extraction'
eonar,=callbacks('spell_eonar_dive_down_jump','HandleScript');r=expanded(eonar);map_guards(r);assert sites(r,'?CastSpellDuration@Unit@@')and 246330 in values(r)and 6000 in values(r),'guarded alive original jump bonus'
print('PASS actual Release sixteenth ten:Moonkin1000/0;three native64 Mimiron halves;two uint64 bootstrap health conversions;Sinestra64 heal/bounded mana and64 drain/lethal comparison;bounded Twin compensation;Anub empty-list guard plus4s retry;Army/Eonar original live map-instance guards and payloads. Prior157 selected paths retained;native/source/client scopes distinct.')
