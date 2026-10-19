$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidInstanceRelease.ps1"
$owner='@instance_trial_of_valor_InstanceMapScript@instance_trial_of_valor@@'
$calls=@(Calls ('?OnGameObjectCreate'+$owner))
foreach($name in @('?isSpawned@GameObject@@','?SetRespawnTime@GameObject@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical late chest admission lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-raid-chest-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;native_chest_admission_checks_spawn_and_restores_timer=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical PE/thunks contain native hidden-chest admission and respawn timer restoration.'
