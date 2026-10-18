$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/vrekt-credits"
New-Item -ItemType Directory -Force $out | Out-Null
function Block([string]$source,[string]$signature){
 $a=$source.IndexOf($signature);if($a -lt 0){throw "Missing native boundary $signature"}
 $b=$source.IndexOf('{',$a)+1;$depth=1
 while($depth -and $b -lt $source.Length){if($source[$b] -eq '{'){++$depth};if($source[$b] -eq '}'){--$depth};++$b}
 if($depth){throw 'Unterminated native block'};$source.Substring($a,$b-$a)
}
$player=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
$reward=Get-Content "$repo/src/server/game/Entities/Player/KillRewarder.cpp" -Raw
$def=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
foreach($name in @('QuestType','QuestStatus','QuestObjectiveType','__QuestFlags','__QuestSpecialFlags','QuestObjectiveFlags')){
 [IO.File]::WriteAllText("$out/$name.inc",(Block $def "enum $name")+';')
}
$objective=[regex]::Match($def,'(?s)struct QuestObjective\s*\{.*?\n\};')
if(!$objective.Success){throw 'Missing native QuestObjective definition'}
[IO.File]::WriteAllText("$out/QuestObjective.inc",$objective.Value)
foreach($pair in @(
 @('void Player::KilledMonster(','KilledMonster'),
 @('void Player::KilledMonsterCredit(','KilledMonsterCredit'),
 @('bool Player::HasQuestObjectiveComplete(','HasObjective'),
 @('bool Player::CanCompleteQuest(','CanComplete'),
 @('void Player::CompleteQuest(','Complete'))){
 [IO.File]::WriteAllText("$out/$($pair[1]).inc",(Block $player $pair[0]))
}
[IO.File]::WriteAllText("$out/Owner.inc",(Block $unit 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
[IO.File]::WriteAllText("$out/KillCredit.inc",(Block $reward 'inline void KillRewarder::_RewardKillCredit('))
# Execute the real initial Kill guard and recipient selection; packet, loot,
# combat/death dispatch after this boundary are outside this fixture.
$kill=Block $unit 'void Unit::Kill('
$end=$kill.IndexOf('    // Reward player, his pets, and group/raid members')
if($end -lt 0){throw 'Missing native kill recipient boundary'}
[IO.File]::WriteAllText("$out/KillRecipient.inc",$kill.Substring($kill.IndexOf('{')+1,$end-$kill.IndexOf('{')-1))
$c=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf" | Where-Object {$_ -match '^WorldDatabaseInfo\s*='} | Select-Object -First 1).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{
 $env:MYSQL_PWD=$c[3]
 $sql=@'
SELECT 'quest',ID,QuestType,Flags,RewardSpell FROM quest_template WHERE ID=44923;
SELECT 'objective',ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags,Flags2 FROM quest_objectives WHERE QuestID=44923 ORDER BY StorageIndex;
SELECT 'creature',Entry,KillCredit1,KillCredit2 FROM creature_template_wdb WHERE Entry IN(115846,116180,116600,116601) ORDER BY Entry;
SELECT 'marker',COUNT(*) FROM creature_template_wdb WHERE entry=115845;
'@
 $rows=@($sql | & C:/wamp64/bin/mysql/mysql8.4.9/bin/mysql.exe --host=$($c[0]) --port=$($c[1]) --user=$($c[2]) --database=$($c[4]) --batch --raw --skip-column-names)
 if($LASTEXITCODE){throw 'Vrekt read-only data query failed'}
}finally{$env:MYSQL_PWD=$previous}
$rows | Set-Content "$out/data.tsv"
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/VrektCreditsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/data.tsv`"`nexit /b %errorlevel%`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd)
& "$out/run.cmd" | Tee-Object "$out/results.txt"
if($LASTEXITCODE){throw 'Vrekt native credit regression failed'}
$hashes=@{}
foreach($path in @('src/server/game/Entities/Player/Player.cpp','src/server/game/Entities/Player/KillRewarder.cpp','src/server/game/Entities/Unit/Unit.cpp','src/server/game/Quests/QuestDef.h','tools/tests/VrektCreditsTest.cpp')){
 $hashes[$path]=(Get-FileHash "$repo/$path").Hash
}
$hashes['read-only-data.tsv']=(Get-FileHash "$out/data.tsv").Hash
$hashes | ConvertTo-Json | Set-Content "$out/inputs.json"
'PASS: production recipient selection, delayed credit, alternate kill markers, mandatory objectives and task auto-reward dispatch. Fixture reward/storage/network leaves do not verify actual rewards, persistence or client playback.'
