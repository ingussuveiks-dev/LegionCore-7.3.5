param([string]$OutputDirectory="$PSScriptRoot/../../build-extractors/runtime-test/highmountain-bloodtotem")
$ErrorActionPreference='Stop'
$output=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$PSScriptRoot/../../src/server/game/Entities/Player/Player.cpp" -Raw
$progressStart=$source.IndexOf('case QUEST_OBJECTIVE_TASK_IN_ZONE:', $source.IndexOf('bool Player::HasQuestObjectiveComplete('))
$progressEnd=$source.IndexOf('case QUEST_OBJECTIVE_PET_TRAINER_DEFEAT:', $progressStart)
[IO.File]::WriteAllText("$output/BloodtotemProgress.inc",$source.Substring($progressStart,$progressEnd-$progressStart))
$code=''
foreach($pair in @(@('bool Player::SatisfyQuestPreviousQuest(', 'bool Player::SatisfyQuestClass('),@('InventoryResult Player::CanTakeMoreSimilarItems(', 'uint8 Player::GetItemLimitCategoryQuantity('),@('bool Player::CanCompleteQuest(', 'bool Player::CanCompleteRepeatableQuest('))){
 $start=$source.IndexOf($pair[0]);$end=$source.IndexOf($pair[1],$start)
 if($start -lt 0 -or $end -lt 0){throw 'Missing production handler'}
 $code+=$source.Substring($start,$end-$start)
}
[IO.File]::WriteAllText("$output/BloodtotemCore.inc",$code)
$code=''
foreach($pair in @(@('bool Player::HasQuestForItem(', 'bool Player::HasQuestForCreature('),@('void Player::ItemAddedQuestCheck(', 'void Player::KilledMonster('))){
 $start=$source.IndexOf($pair[0]);$end=$source.IndexOf($pair[1],$start)
 if($start -lt 0 -or $end -lt 0){throw 'Missing production item lifecycle handler'}
 $code+=$source.Substring($start,$end-$start)
}
[IO.File]::WriteAllText("$output/BloodtotemItems.inc",$code)
$start=$source.IndexOf('void Player::KilledMonster(');$end=$source.IndexOf('void Player::KilledPlayerCredit(',$start)
if($start -lt 0 -or $end -lt 0){throw 'Missing production kill credit handlers'}
[IO.File]::WriteAllText("$output/BloodtotemKills.inc",$source.Substring($start,$end-$start))
$start=$source.IndexOf('case QUEST_OBJECTIVE_ITEM:', $source.IndexOf('void Player::RewardQuest('))
$end=$source.IndexOf('case QUEST_OBJECTIVE_CURRENCY:',$start)
[IO.File]::WriteAllText("$output/BloodtotemReward.inc",$source.Substring($start,$end-$start))
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_bloodtotem.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/BloodtotemLogin.inc",$source.Substring(0,$source.IndexOf('void AddSC_highmountain_bloodtotem()')))
$source=Get-Content "$PSScriptRoot/../../src/server/scripts/Legion/highmountain_lair.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/LairLogin.inc",$source.Substring(0,$source.IndexOf('void AddSC_highmountain_lair()')))
$source=Get-Content "$PSScriptRoot/../../src/server/game/DataStores/DB2Stores.cpp" -Raw
$start=$source.IndexOf('itemSparseHotfixes.ApplyHotfix(128380,');$end=$source.IndexOf('}, true);',$start)+'}, true);'.Length
[IO.File]::WriteAllText("$output/BloodtotemHotfix.inc",$source.Substring($start,$end-$start))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd=@"
@echo off
call "$vs/VC/Auxiliary/Build/vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /I"$output" "$PSScriptRoot/HighmountainBloodtotemTest.cpp" /Fe"$output/HighmountainBloodtotemTest.exe" /Fo"$output/HighmountainBloodtotemTest.obj"
if errorlevel 1 exit /b 1
"$output/HighmountainBloodtotemTest.exe"
exit /b %errorlevel%
"@
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Bloodtotem production regression failed'}
