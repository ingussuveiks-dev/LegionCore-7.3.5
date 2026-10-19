$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidInstanceRelease.ps1"
if((Get-Item "$repo/src/server/scripts/Legion/TrialOfValor/trial_of_valor.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw 'Actual final-link scripts library predates exit validation'}
$calls=@(Calls '?sGossipSelect@npc_odyn_after_kill_helya@@')
foreach($name in @('?GetMenuId@GossipMenu@@','?GetItem@GossipMenu@@','?ClearMenus@PlayerMenu@@','?TeleportTo@Player@@QEAA_NIMMMMII@Z')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical exit handler lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-raid-exit-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;exit_menu_lookup_consume_and_native_teleport=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical PE/thunks retain exit menu/option lookup, consumed-menu cleanup and native teleport.'
