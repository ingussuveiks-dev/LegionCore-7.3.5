param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify current Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/skyhold-artifact-return"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
if($Baseline){$source=(& git -C $repo show '18033742:src/server/game/Entities/Player/Player.cpp') -join "`n";if($LASTEXITCODE){throw 'Pinned credit source unavailable'}}
[IO.File]::WriteAllText("$out/Policy.inc",$(if($Baseline){''}else{Block $source 'namespace SkyholdArtifactReturn'}))
$credit=Block $source 'void Player::KilledMonsterCredit('
[IO.File]::WriteAllText("$out/Credit.inc",(Block $credit '    for (uint8 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)'))
$blocks=@('int32 Player::GetQuestObjectiveData(Quest const*','void Player::SetQuestObjectiveData(','bool Player::HasQuestObjectiveComplete(','bool Player::CanCompleteQuest(','void Player::RewardPlayerAndGroupAtEvent(','bool Player::IsAtGroupRewardDistance(')
[IO.File]::WriteAllText("$out/Native.inc",[string]::Join("`n",[string[]]@($blocks|ForEach-Object {Block $source $_})))
$definition=Get-Content "$repo/src/server/game/Quests/QuestDef.h" -Raw
[IO.File]::WriteAllText("$out/ObjectiveEnums.inc",(Block $definition 'enum QuestObjectiveType')+';'+(Block $definition 'enum QuestObjectiveFlags')+';')
$connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
$previous=$env:MYSQL_PWD
try{$env:MYSQL_PWD=$connection[3];$rows=@(@'
SELECT ID,QuestID,Type,StorageIndex,ObjectID,Amount,Flags FROM quest_objectives WHERE QuestID IN(39191,40043,41105) ORDER BY QuestID,ID;
'@|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Native Skyhold objectives read failed'}}finally{$env:MYSQL_PWD=$previous}
[IO.File]::WriteAllLines("$out/objectives.tsv",[string[]]$rows)
$expected=@("279784`t39191`t0`t0`t96508`t1`t0","279785`t39191`t0`t1`t94851`t1`t2","282129`t39191`t0`t2`t103739`t1`t2","280193`t40043`t0`t0`t98032`t1`t0","280194`t40043`t0`t1`t98033`t1`t2","282124`t40043`t0`t2`t103739`t1`t2","281862`t41105`t0`t0`t97377`t1`t4","281863`t41105`t0`t1`t103114`t1`t2","282354`t41105`t0`t2`t103739`t1`t2","286343`t41105`t0`t3`t102394`t1`t4","286344`t41105`t0`t4`t113105`t1`t0")
if(Compare-Object $expected $rows){throw 'Native three-quest objective identities/slots/flags changed'}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$baselineFlag=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $baselineFlag /I`"$out`" `"$PSScriptRoot/SkyholdArtifactReturnTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/objectives.tsv`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Skyhold return progression failed'}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_skyhold_artifact_return_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Skyhold quest guard missing'}}finally{$env:PYTHONPATH=$previous}
}
