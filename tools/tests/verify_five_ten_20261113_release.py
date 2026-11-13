"""Selected implementation proof;50 is the authorization,not a pass count."""
import verify_stormheim_departure_release
from five_ten_20261113_scope import verify,PLAN
from five_ten_20261111_pe import n,ins,sites,values,callbacks,expanded
from verify_ai_lifetime_guard import verify_plan
verify()
p=dict(n=n,ins=ins,sites=sites,values=values,callbacks=callbacks,expanded=expanded)
groups=[('NightholdGateLifetime','go_nighthold_gate_event','OnStateChanged',3)]
seen=verify_plan(p,[dict(key=key,callbacks=[dict(cls=cls,method=method,compiled_count=count)])for key,cls,method,count in groups])
assert len(seen)==3
for cls in ('spell_felhounds_corruption_filter','spell_felhounds_molten_touch_filter','spell_felhounds_shadowscar_filter'):
 rows=ins('?FilterTargets@'+cls+'@@')
 guard=next(i for i,x in enumerate(rows)if x.mnemonic=='cmp' and x.op_str in ('byte ptr [rsi + 0x16], 4','byte ptr [rcx + 0x16], 4') and rows[i+1].mnemonic=='jne')
 assert guard<sites(rows,'?HasAura@Unit@@')[0],(cls,'Player gate must precede aura/member access')
rows=ins('?OnStateChanged@go_nighthold_gate_event@@')
assert any(x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x20], 1'for x in rows),'Gate claims its sequence before scheduling'
passenger=ins('?CheckElevation@spell_events_warsong_take_passenger_check@@')
assert any(x.mnemonic=='test'and x.op_str=='rdi, rdi'for x in passenger) and any(x.mnemonic=='test'and x.op_str=='rax, rax'for x in passenger),'Passenger checks both actual caster and explicit target'
swarm=ins('?HandleEffectPeriodic@spell_anubarak_leeching_swarm_AuraScript@')
caster=sites(swarm,'?GetCaster@AuraScript@@')[0];target=sites(swarm,'?GetTarget@AuraScript@@')[0]
assert caster<target and swarm[caster+2].op_str=='rax, rax' and swarm[caster+3].mnemonic=='je','Missing Swarm caster returns before recipient/payload access'
snow=ins('??0npc_snowglobe_stalker@@')
create=sites(snow,'?CreateAreaTrigger@AreaTrigger@@')[0]
assert any(x.mnemonic=='xor'and x.op_str=='esi, esi'for x in snow[create:create+14]),'Failed snowglobe area is deleted and its pointer cleared'
scale=sites(snow,'?SetSphereScale@AreaTrigger@@');assert len(scale)==1
assert sum(x.mnemonic=='test'and x.op_str=='rsi, rsi'for x in snow[create:scale[0]])==3,'Both scaled map branches admit only a real area'
command=ins('?HandleAreaTriggerCommand@misc_commandscript@@')
failed=next(i for i,x in enumerate(command)if x.mnemonic=='mov'and x.op_str=='byte ptr [rdi + 0x10], 1'and command[i+1].mnemonic=='xor'and command[i+1].op_str=='al, al')
assert any(x.mnemonic=='jmp' and x.operands[0].imm==command[failed].address for x in command[sites(command,'?CreateAreaTrigger@AreaTrigger@@')[0]:failed]),'Creation failure reaches visible error/false return'
snapshots=[name for name in callbacks('npc_crash_test','OnGossipSelect')if name.endswith('EEAAXXZ')];assert len(snapshots)==1
snapshot=expanded(snapshots[0]);assert any(x.mnemonic=='mov'and x.op_str=='edi, dword ptr [rcx + 0x20]'for x in snapshot)and any(x.mnemonic=='mov'and x.op_str=='dword ptr [rsp + 0x38], edi'for x in snapshot),'Queued spawn consumes captured scalar despawn value'
print('PASS first10 linked guards,GO lifetime/claim,Snowglobe release,command failure and spawn snapshot;retained Stormheim/logout/earlier PE scopes passed;authorized50 remains separate')
