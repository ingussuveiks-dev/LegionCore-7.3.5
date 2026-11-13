"""Map-thread ceremony departure route in the actual linked Release image."""
import verify_pet_battle_logout_release
from five_ten_20261111_pe import ins,sites,values

leave=ins('?OnPlayerLeave@map_stormheim_blessing@@')
assert [(x.mnemonic,x.op_str) for x in leave[:1]]==[('mov','rcx, r8')]
assert sites(leave,'?StopForPlayer@StormheimBlessing@@')==[1], 'Actual departing Player reaches the shared cleanup'
stop=ins('?StopForPlayer@StormheimBlessing@@')
assert sites(stop,'?GetCreature@ObjectAccessor@@') and len(sites(stop,'?Stop@npc_stormheim_blessing_actor@@'))==1
claim=sites(stop,'?Stop@npc_stormheim_blessing_actor@@')[0]
assert any(x.mnemonic=='cmp' and x.op_str=='byte ptr [rax + 0x2a4], 0' for x in stop[:claim]), 'Only a scoped ceremony may be stopped'
assert any(x.mnemonic=='lea' and x.op_str=='rcx, [rbx + 0x258]' for x in stop[:claim]) and sites(stop,'??8ObjectGuid@@'), 'Compare the actual full owner GUID before Stop'
cleanup=ins('?Cleanup@npc_stormheim_blessing_actor@@')
assert sites(cleanup,'?GetPlayer@ObjectAccessor@@') and not sites(cleanup,'?FindPlayer@ObjectAccessor@@'), 'Actor cleanup stays on its own map'
assert len(sites(cleanup,'?RemoveAurasDueToSpell@Unit@@'))==2 and {190904,190905}<=values(cleanup)
assert sum(x.mnemonic=='lea' and x.op_str=='r8, [rsp + 0x30]' for x in cleanup)==2, 'Both removals pass the real actor GUID as the caster filter'
logout=ins('?OnLogout@player_stormheim_blessing@@')
assert 1220 in values(logout) and sites(logout,'?StopForPlayer@StormheimBlessing@@'), 'Late logout fallback is restricted to its source map'
registration=ins('?AddSC_stormheim_blessing@@')
assert 1220 in values(registration) and sites(registration,'??0WorldMapScript@@'), 'Production registration includes the real1220 map script'
print('PASS linked Stormheim map departure,scoped/full-owner admission,map-local exact-caster root cleanup and real map registration;retained logout checks passed')
