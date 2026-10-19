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
$save=Resolve (Symbol '?SaveInventoryAndGoldToDB@Player@@')
$rewardCalls=@(Calls '?RewardQuest@Player@@')
# The three-call save wrapper can be inlined into RewardQuest by Release.
$saving=if($rewardCalls -contains $save){@(Calls '?SaveInventoryAndGoldToDB@Player@@')}else{$rewardCalls}
foreach($name in @('?_SaveInventory@Player@@','?_SaveCurrency@Player@@')){
 if($saving -notcontains (Resolve (Symbol $name))){throw "Canonical checkpoint helper lacks $name"}
}
$out=Join-Path $repo 'build-extractors/runtime-test/lost-army-turnin-native'
New-Item -ItemType Directory -Force $out | Out-Null
[ordered]@{worldserver=(Get-FileHash $exe).Hash;actual_game_library=(Get-FileHash $gameInput).Hash;choice_retry=$true;inventory_currency_checkpoint=$true;gold='SaveGoldToDB may be inlined; native probe checks actual SQL money against memory before the next full save.'} | ConvertTo-Json | Set-Content "$out/release-inputs.json"
'PASS: canonical Release choice retry and inventory/currency reward checkpoint verified with final-link freshness and PE/thunk call sites; native probe separately checks persisted gold.'
