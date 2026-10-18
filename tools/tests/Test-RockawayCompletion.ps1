$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/rockaway-completion"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$signature){
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native boundary $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 if($depth){throw 'Unterminated native block'};$source.Substring($a,$b-$a)
}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$def=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
$header=Get-Content "$repo/src/server/game/Entities/Player/Player.h" -Raw
foreach($name in @('QuestType','QuestInfo','QuestStatus','QuestObjectiveType','__QuestFlags','QuestFlagsEx','__QuestSpecialFlags','QuestObjectiveFlags')){
 [IO.File]::WriteAllText("$out/$name.inc",(Block $def "enum $name")+';')
}
foreach($name in @('QUEST_ITEM_DROP_COUNT','QUEST_REWARD_DISPLAY_SPELL_COUNT')){
 $match=[regex]::Match($def,"(?m)^#define $name\s+\d+")
 if(!$match.Success){throw "Missing native $name"};[IO.File]::WriteAllText("$out/$name.inc",$match.Value)
}
$objective=[regex]::Match($def,'(?s)struct QuestObjective\s*\{.*?\n\};')
if(!$objective.Success){throw 'Missing native objective definition'}
[IO.File]::WriteAllText("$out/QuestObjective.inc",$objective.Value)
[IO.File]::WriteAllText("$out/WorldQuestInfo.inc",(Block $header 'struct WorldQuestInfo')+';')
$items=Get-Content "$repo/src/server/game/Entities/Item/Item.h" -Raw
[IO.File]::WriteAllText("$out/InventoryResult.inc",(Block $items 'enum InventoryResult')+';')
foreach($pair in @(
 @('void Player::KilledMonsterCredit(','Credit'),
 @('bool Player::HasQuestObjectiveComplete(','HasObjective'),
 @('bool Player::CanCompleteQuest(','CanComplete'),
 @('void Player::CompleteQuest(','Complete'),
 @('bool Player::CanRewardWorldQuest(','Admission'),
 @('bool Player::WorldQuestCompleted(','Completed'),
 @('void Player::RewardQuest(','Reward'),
 @('void Player::_SaveWorldQuestStatus(','SaveWorld'),
 @('void Player::_LoadWorldQuestStatus(','LoadWorld'),
 @('bool Player::AutoStoreLoot(','AutoStoreLoot'))){
 [IO.File]::WriteAllText("$out/$($pair[1]).inc",(Block $player $pair[0]))
}
$ai=Get-Content "$repo/src/server/scripts/Legion/rockaway_coast.h" -Raw
foreach($pair in @(@('inline bool Active()','Active'),@('inline bool Eligible(','Eligible'),@('    void Finish()','Finish'))){
 [IO.File]::WriteAllText("$out/$($pair[1]).inc",(Block $ai $pair[0]))
}
$old=(& git -C $repo show 65c7ac0a:src/server/scripts/Legion/rockaway_coast.h) -join "`n"
if($LASTEXITCODE){throw 'Missing pinned pre-eligibility-fix source'}
[IO.File]::WriteAllText("$out/EligibleBaseline.inc",(Block $old 'inline bool Eligible('))
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 $sql="SELECT 'quest',ID,QuestType,QuestInfoID,QuestSortID,Flags,FlagsEx,RewardSpell FROM quest_template WHERE ID=44746;
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2 FROM quest_objectives WHERE QuestID=44746;
SELECT 'loot',Entry,Item,MinCount,MaxCount,QuestRequired FROM spell_loot_template WHERE Entry=229113;
SELECT 'marker',COUNT(*) FROM creature_template_wdb WHERE Entry=115168;
SELECT 'static',QuestPackageID,RewardItem1+RewardItem2+RewardItem3+RewardItem4,
RewardAmount1+RewardAmount2+RewardAmount3+RewardAmount4,
RewardChoiceItemID1+RewardChoiceItemID2+RewardChoiceItemID3+RewardChoiceItemID4+RewardChoiceItemID5+RewardChoiceItemID6,
RewardChoiceItemQuantity1+RewardChoiceItemQuantity2+RewardChoiceItemQuantity3+RewardChoiceItemQuantity4+RewardChoiceItemQuantity5+RewardChoiceItemQuantity6,
RewardCurrencyID1+RewardCurrencyID2+RewardCurrencyID3+RewardCurrencyID4,
RewardCurrencyQty1+RewardCurrencyQty2+RewardCurrencyQty3+RewardCurrencyQty4,
ItemDrop1+ItemDrop2+ItemDrop3+ItemDrop4,ItemDropQuantity1+ItemDropQuantity2+ItemDropQuantity3+ItemDropQuantity4,
RewardDisplaySpell1+RewardDisplaySpell2+RewardDisplaySpell3,
RewardHonor+RewardArtifactXP+RewardArtifactCategoryID+RewardTitle+RewardSkillLineID+RewardNumSkillUps,
RewardMoney,RewardBonusMoney,RewardXPDifficulty,RewardXPMultiplier FROM quest_template WHERE ID=44746;
SELECT 'addon',RewardMailTemplateID,RewardMailDelay,SpecialFlags FROM quest_template_addon WHERE ID=44746;"
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Read-only Rockaway snapshot failed'}
}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/data.tsv"
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($variant in @('current','baseline')){
 $define=if($variant -eq 'baseline'){'/DOLD_ELIGIBILITY'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 $define /I`"$out`" `"$PSScriptRoot/RockawayCompletionTest.cpp`" /Fe`"$out/$variant.exe`" /Fo`"$out/$variant.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$variant.exe`" `"$out/data.tsv`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/run-$variant.cmd",$cmd)
 & "$out/run-$variant.cmd" | Tee-Object "$out/$variant.txt"
 if($LASTEXITCODE){throw "Native Rockaway completion $variant regression failed"}
}
$hashes=@{}
foreach($path in @('src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Player/Player.h','src/server/game/Quests/QuestDef.h','src/server/scripts/Legion/rockaway_coast.h','tools/tests/RockawayCompletionTest.cpp')){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
$hashes['data.tsv']=(Get-FileHash "$out/data.tsv").Hash
$hashes | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: complete production Finish -> AutoStoreLoot/credit -> CompleteQuest -> full RewardQuest -> native completion save/load. Inventory/loot generation/reward-item helper, DB transaction, network and spell playback are controlled leaves; this is not a real Player session.'
