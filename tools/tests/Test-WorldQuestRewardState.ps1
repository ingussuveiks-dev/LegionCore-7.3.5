$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/world-quest-reward-state"
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
foreach($name in @('QuestType','QuestInfo','QuestStatus','QuestObjectiveType','__QuestFlags','__QuestSpecialFlags','QuestObjectiveFlags')){
 [IO.File]::WriteAllText("$out/$name.inc",(Block $def "enum $name")+';')
}
[IO.File]::WriteAllText("$out/WorldQuestInfo.inc",(Block $header 'struct WorldQuestInfo')+';')
foreach($pair in @(
 @('bool Player::CanRewardWorldQuest(','Admission'),
 @('bool Player::WorldQuestCompleted(','Completed'),
 @('void Player::_LoadWorldQuestStatus(','Load'),
 @('void Player::_SaveWorldQuestStatus(','Save'),
 @('void Player::ResetWorldQuest(','Reset'))){
 [IO.File]::WriteAllText("$out/$($pair[1]).inc",(Block $player $pair[0]))
}
function WriteVariant([string]$source,[string]$folder){
 foreach($pair in @(@('void Player::CompleteQuest(','Complete'),@('bool Player::CanCompleteQuest(','CanComplete'),@('bool Player::CanRewardQuest(Quest const* quest, bool msg)','CanReward'))){
  [IO.File]::WriteAllText("$folder/$($pair[1]).inc",(Block $source $pair[0]))
 }
 $reward=Block $source 'void Player::RewardQuest('
 $a=$reward.IndexOf('{')+1;$b=$reward.IndexOf('    //this THING')
 if($b -lt $a){throw 'Missing destructive reward boundary'}
 [IO.File]::WriteAllText("$folder/RewardPrefix.inc",$reward.Substring($a,$b-$a))
}
WriteVariant $player $out
$baseline=(& git -C $repo show caddeec5:src/server/game/Entities/Player/Player.cpp) -join "`n"
if($LASTEXITCODE){throw 'Pinned reward baseline unavailable'}
New-Item -ItemType Directory -Force "$out/baseline" | Out-Null
Copy-Item "$out/*.inc" "$out/baseline/"
WriteVariant $baseline "$out/baseline"
$sql=Get-Content "$repo/src/server/database/Database/Implementation/CharacterDatabase.cpp" -Raw
if($sql -notmatch 'CHAR_INS_CHARACTER_WORLDQUESTSTATUS, "REPLACE INTO character_queststatus_world \(guid, quest, account, resetTime\) VALUES \(\?, \?, \?, \?\)"' -or
 $sql -notmatch 'CHAR_SEL_CHARACTER_WORLDQUESTSTATUS, "SELECT quest, guid, resetTime FROM character_queststatus_world WHERE account = \? AND \(guid = 0 OR guid = \?\)"'){
 throw 'Native save/load SQL order changed; update the fixture boundary'
}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
foreach($variant in @('current','baseline')){
 $include=if($variant -eq 'baseline'){"$out/baseline"}else{$out}
 $define=if($variant -eq 'baseline'){'/DOLD_REWARD'}else{''}
 $cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $define /I`"$include`" `"$PSScriptRoot/WorldQuestRewardStateTest.cpp`" /Fe`"$out/$variant.exe`" /Fo`"$out/$variant.obj`"`nif errorlevel 1 exit /b 1`n`"$out/$variant.exe`"`nexit /b %errorlevel%`n"
 [IO.File]::WriteAllText("$out/run-$variant.cmd",$cmd)
 $lines=@(& "$out/run-$variant.cmd")
 $lines | Tee-Object "$out/$variant.txt"
 if($LASTEXITCODE){throw "World quest reward $variant regression failed"}
 if($variant -eq 'current'){$savedRow=@($lines | Where-Object {$_ -match '^ROW [0-9]+ [0-9]+ [0-9]+ [0-9]+$'})}
}
if($savedRow.Count -ne 1){throw 'Missing native prepared-statement field snapshot'}
$values=($savedRow[0] -split ' ')[1..4]
$insert=[regex]::Match($sql,'CHAR_INS_CHARACTER_WORLDQUESTSTATUS, "([^"]+)"').Groups[1].Value
$select=[regex]::Match($sql,'CHAR_SEL_CHARACTER_WORLDQUESTSTATUS, "([^"]+)"').Groups[1].Value
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^CharacterDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$connection[3]
 # Empty connection-local shadow table: never read or change personal rows.
 $roundTrip=@"
CREATE TEMPORARY TABLE worldquest_schema LIKE character_queststatus_world;
CREATE TEMPORARY TABLE character_queststatus_world LIKE worldquest_schema;
PREPARE nativeSave FROM '$insert';
SET @guid=$($values[0]),@quest=$($values[1]),@account=$($values[2]),@resetTime=$($values[3]);
EXECUTE nativeSave USING @guid,@quest,@account,@resetTime;
EXECUTE nativeSave USING @guid,@quest,@account,@resetTime;
SELECT COUNT(*) FROM character_queststatus_world;
PREPARE nativeLoad FROM '$select';
EXECUTE nativeLoad USING @account,@guid;
SET @account=78;
EXECUTE nativeLoad USING @account,@guid;
"@
 $rows=@($roundTrip | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Temporary native character status SQL round trip failed'}
}finally{$env:MYSQL_PWD=$previous}
if(($rows -join '|') -ne "1|$($values[1])`t$($values[0])`t$($values[3])"){throw 'Native character status SQL changed rows, widths or account isolation'}
$rows | Set-Content "$out/sql-round-trip.tsv"
$hashes=@{}
foreach($path in @('src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Player/Player.h','src/server/database/Database/Implementation/CharacterDatabase.cpp','tools/tests/WorldQuestRewardStateTest.cpp')){
 $hashes[$path]=(Get-FileHash "$repo/$path").Hash
}
$hashes | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: production completion/direct-reward admission and save/load/reset functions; native prepared SQL round-trip in an empty temporary shadow table preserves unsigned GUID, repeat save and account isolation. Existing objective/turn-in predicates unchanged; network/reward delivery are fixture leaves.'
