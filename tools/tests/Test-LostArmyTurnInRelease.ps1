$ErrorActionPreference='Stop'
. "$PSScriptRoot/Test-GroupRollLifecycleRelease.ps1"
foreach($path in @('Handlers/QuestHandler.cpp','Entities/Player/Player.cpp','Entities/Player/CollectionMgr.cpp')){
 if((Get-Item "$repo/src/server/game/$path").LastWriteTimeUtc -gt (Get-Item $gameInput).LastWriteTimeUtc){throw "Final-link game library predates $path"}
}
$choose=@(Calls '?HandleQuestGiverChooseReward@WorldSession@@')
$select=Resolve (Symbol '?CanSelectQuestPackageItem@Player@@')
$offer=Resolve (Symbol '?SendQuestGiverOfferReward@PlayerMenu@@')
# MSVC merges the two retry branches into one common offer-reward tail.
if(@($choose | Where-Object {$_ -eq $select}).Count -lt 2 -or $choose -notcontains $offer){throw 'Canonical reward handler lacks empty-choice admission/retry'}
$out=Join-Path $repo 'build-extractors/runtime-test/lost-army-turnin-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;choice_retry=$true} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release choice retry verified with final-link freshness and PE/thunk call sites.'
