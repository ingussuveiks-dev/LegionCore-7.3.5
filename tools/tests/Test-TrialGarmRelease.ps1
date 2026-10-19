$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidReentryRelease.ps1"
if((Get-Item "$repo/src/server/scripts/Legion/TrialOfValor/boss_tov_odyn.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw 'Final-link scripts library predates Sky Jump validation'}
$calls=@(Calls '?sGossipSelect@boss_tov_odyn@@')
foreach($name in @('?GetMenuId@GossipMenu@@','?GetItem@GossipMenu@@','?ClearMenus@PlayerMenu@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical Odyn travel handler lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-garm-native';New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;sky_jump_menu_lookup_and_consume=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Odyn Sky Jump PE/thunks retain actual menu/option lookup and consumed-menu cleanup.'
