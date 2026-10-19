$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-TrialRaidCriteriaRelease.ps1"
if((Get-Item "$repo/src/server/scripts/Legion/TrialOfValor/instance_trial_of_valor.cpp").LastWriteTimeUtc -gt (Get-Item $lib).LastWriteTimeUtc){throw 'Final-link scripts library predates victory actor fix'}
$owner='@instance_trial_of_valor_InstanceMapScript@instance_trial_of_valor@@'
$ensure=Resolve (Symbol ('?EnsureVictoryActor'+$owner))
$odyn=Resolve (Symbol ('?RestoreOdynVictoryActors'+$owner))
$helya=Resolve (Symbol ('?RestoreHelyaVictoryActors'+$owner))
foreach($method in @('OnCreatureCreate','SetBossState')){
 $calls=@(Calls ('?'+$method+$owner))
 if($calls -notcontains $odyn -or $calls -notcontains $helya){throw "Canonical $method does not share victory actor restoration"}
}
foreach($method in @('RestoreOdynVictoryActors','RestoreHelyaVictoryActors')){
 if(@(Calls ('?'+$method+$owner)) -notcontains $ensure){throw "Canonical $method bypasses actor admission"}
}
$calls=@(Calls ('?EnsureVictoryActor'+$owner))
foreach($name in @('?GetCreature@Map@@','?SummonCreature@Map@@')){
 if($calls -notcontains (Resolve (Symbol $name))){throw "Canonical victory actor admission lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-raid-instance-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;actual_scripts_library=(Get-FileHash $lib).Hash;both_callbacks_share_restoration=$true;native_actor_lookup_and_summon=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical PE/thunks retain shared victory restoration in both callbacks and native per-instance actor lookup before summon.'
