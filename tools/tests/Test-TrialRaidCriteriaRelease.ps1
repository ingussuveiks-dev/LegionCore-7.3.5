$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-LostArmyRelease.ps1"
if((Get-Item "$repo/src/server/game/Achievements/AchievementMgr.cpp").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw 'Final-link game library predates achievement fix'}
$scoped=Resolve (Symbol '?AddQuestDelayedEvent@Player@@')
if(@(Calls '?SetCriteriaProgress@?$AchievementMgr@VPlayer@@@@') -notcontains $scoped){throw 'Canonical achievement notification lacks quest lifetime'}
$dispatch=Resolve (Symbol '?DoUpdateAchievementCriteria@InstanceScript@@')
foreach($name in @('?DamageTakenAfterScaling@boss_tov_odyn@@','?JustDied@boss_helya_tov@@')){
 if(@(Calls $name) -notcontains $dispatch){throw "Canonical boss callback lacks instance criteria dispatch: $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/trial-raid-criteria-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;quest_lifetime_in_player_progress=$true;boss_instance_dispatch=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical PE/thunks verify quest lifetime in player achievement progress and native Odyn/Helya criteria dispatch.'
