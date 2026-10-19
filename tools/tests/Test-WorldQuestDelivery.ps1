$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/world-quest-delivery"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$signature){
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native boundary $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 if($depth){throw 'Unterminated native block'};$source.Substring($a,$b-$a)
}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$item=Get-Content "$repo/src/server/game/Entities/Item/Item.cpp" -Raw
$mail=Get-Content "$repo/src/server/game/Mails/Mail.cpp" -Raw
foreach($definition in @(@('Entities/Item/Item.h','InventoryResult'),@('Entities/Item/Item.h','ItemUpdateState'),@('Entities/Item/ItemTemplate.h','ItemBondingType'),@('DataStores/DBCEnums.h','ItemLimitCategoryMode'),@('Globals/QuestData.h','WorldQuestTypeReward'),@('Mails/Mail.h','MailMessageType'),@('Mails/Mail.h','MailCheckMask'),@('Mails/Mail.h','MailStationery'),@('Mails/Mail.h','MailState'))){
 $header=Get-Content "$repo/src/server/game/$($definition[0])" -Raw
 [IO.File]::WriteAllText("$out/$($definition[1]).inc",(Block $header "enum $($definition[1])")+';')
}
$mailHeader=Get-Content "$repo/src/server/game/Mails/Mail.h" -Raw
$limit=[regex]::Match($mailHeader,'(?m)^#define MAX_MAIL_ITEMS\s+\d+')
if(!$limit.Success){throw 'Missing native mail attachment limit'}
[IO.File]::WriteAllText("$out/MailLimit.inc",$limit.Value)
foreach($pair in @(@($player,'void RewardWorldQuestItem(','Delivery'),@($item,'Item* Item::CreateItem(','CreateItem'),@($item,'void Item::SetState(','ItemState'),@($item,'void Item::AddToUpdateQueueOf(','QueueAdd'),@($item,'void Item::RemoveFromUpdateQueueOf(','QueueRemove'),@($item,'bool Item::IsInUpdateQueue() const','Queued'),@($mail,'MailDraft& MailDraft::AddItem(','AddItem'),@($mail,'void MailDraft::SendMailTo(','SendMail'),@($mail,'void MailDraft::deleteIncludedItems(','DeleteMailItems'))){
 [IO.File]::WriteAllText("$out/$($pair[2]).inc",(Block $pair[0] $pair[1]))
}
[IO.File]::WriteAllText("$out/Similar.inc",(Block $player 'InventoryResult Player::CanTakeMoreSimilarItems('))
function Selection([string]$source,[string]$folder){
 $start=$source.IndexOf('            uint32 ItemID = GetTeamId() == TEAM_HORDE ? wq->ItemList[CLASS_NONE]')
 $end=$source.IndexOf('            // TC_LOG_DEBUG("worldquest", "RewardQuest IsWorld moneyRew',$start)
 if($start -lt 0 -or $end -lt $start){throw 'Native item selection boundary changed'}
 [IO.File]::WriteAllText("$folder/Selection.inc",$source.Substring($start,$end-$start))
}
Selection $player $out
$a=$player.IndexOf('    SaveInventoryAndGoldToDB(rewardTrans);')
$b=$player.IndexOf('    // cast spells after mark quest complete',$a)
if($a -lt 0 -or $b -lt $a){throw 'Missing atomic completion boundary'}
[IO.File]::WriteAllText("$out/Commit.inc",$player.Substring($a,$b-$a))
$baseline=(& git -C $repo show ccb40e91:src/server/game/Entities/Player/Player.cpp) -join "`n"
if($LASTEXITCODE){throw 'Pinned pre-delivery source unavailable'}
New-Item -ItemType Directory -Force "$out/baseline" | Out-Null
Copy-Item "$out/*.inc" "$out/baseline/"
Selection $baseline "$out/baseline"
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($variant in @('current','baseline')){
 $include=if($variant -eq 'baseline'){"$out/baseline"}else{$out}
 $define=if($variant -eq 'baseline'){'/DOLD_DELIVERY'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$include`" `"$PSScriptRoot/WorldQuestDeliveryTest.cpp`" /Fe`"$out/$variant.exe`" /Fo`"$out/$variant.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$variant.exe`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/run-$variant.cmd",$cmd)
 & "$out/run-$variant.cmd" | Tee-Object "$out/$variant.txt"
 if($LASTEXITCODE){throw "Native world quest delivery $variant check failed"}
}
$hashes=@{}
foreach($path in @('src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Item/Item.cpp','src/server/game/Mails/Mail.cpp','tools/tests/WorldQuestDeliveryTest.cpp')){$hashes[$path]=(Get-FileHash "$repo/$path").Hash}
$hashes | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: production item selection/delivery, native CreateItem stack clamp and native mail add/send/ownership/cleanup with transaction sinks. Actual item serialization, inventory admission, DB commit and client receipt remain separate integration checks.'
