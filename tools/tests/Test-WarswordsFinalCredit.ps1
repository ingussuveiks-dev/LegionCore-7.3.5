param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot certify fixed Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-credit"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "f5ad6a2d:$path") -join "`n";if($LASTEXITCODE){throw 'Cannot read pinned baseline'}}
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
[IO.File]::WriteAllText("$out/JustDied.inc",(Block $source 'void JustDied(Unit* who)'))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/OwnerPlayer.inc",(Block $source 'Player* Unit::GetCharmerOrOwnerPlayerOrPlayerItself() const'))
$source=Get-Content "$repo/src/server/game/DataStores/DBCEnums.h" -Raw
[IO.File]::WriteAllText("$out/CriteriaTypes.inc",(Block $source 'enum CriteriaTypes')+';')
$source=Get-Content "$repo/src/server/game/Achievements/AchievementMgr.cpp" -Raw
[IO.File]::WriteAllText("$out/GroupTypes.inc",(Block $source 'bool AchievementGlobalMgr::IsGroupCriteriaType('))
$source=Get-Content "$repo/src/server/game/Entities/Player/Player.cpp" -Raw
$first=$source.IndexOf('    if (!ignoreGroup && sAchievementMgr->IsGroupCriteriaType(type))',$source.IndexOf('void Player::UpdateAchievementCriteria('))
$last=$source.IndexOf('    // Update only individual achievement criteria here, otherwise we may get multiple updates',$first+1)
if($first -lt 0 -or $last -le $first){throw 'Missing native Player-to-scenario bridge'}
[IO.File]::WriteAllText("$out/ScenarioBridge.inc",$source.Substring($first,$last-$first))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$argument=if($Baseline){' baseline'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/WarswordsFinalCreditTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"$argument`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native Warswords final credit failed'}
$previous=$env:LEGION_DIALOGUE_CONFIG
try{$env:LEGION_DIALOGUE_CONFIG="$repo/build-extractors/bin/Release/worldserver.conf";& "$repo/tools/research/Read-LegionLegendaryCatalog.ps1" -SourceFile WarswordsCreditCatalog.cpp -ProbeName WarswordsCreditCatalog -OutputFile "$out/native.tsv" *> "$out/catalog.log";if(!$? -or $LASTEXITCODE){throw 'Native Warswords catalog failed'}}finally{$env:LEGION_DIALOGUE_CONFIG=$previous}
$native=Get-Content "$out/native.tsv"
if($native -notcontains "CRITERION`t5`t29088`t92`t46987`t0"){throw 'Expected native stage5/criterion29088/event46987/root amount0'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{
  $env:MYSQL_PWD=$connection[3]
  $rows=@("SELECT 'binding',entry,ScriptName FROM creature_template WHERE entry IN(98007,98602);SELECT 'spawn',id,map FROM creature WHERE id=98602;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names)
  if($LASTEXITCODE){throw 'Warswords live binding read failed'}
 }finally{$env:MYSQL_PWD=$previous}
 [IO.File]::WriteAllLines("$out/live.tsv",[string[]]$rows)
 if($rows -notcontains "binding`t98007`tnpc_vigfus_bladewind" -or $rows -notcontains "binding`t98602`tnpc_vigfus_bladewind_last" -or @($rows|Where-Object {$_ -like "spawn`t*"}).Count -ne 1 -or $rows -notcontains "spawn`t98602`t1511"){throw 'Unexpected shared boss binding/spawn context'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_credit_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical Warswords final credit failed'}}finally{$env:PYTHONPATH=$previous}
}
'PASS: Warswords native callback/owner resolution and matching26972 criterion checked. Not actual combat, full chain, group propagation, conversation rendering, real loot/equip or client traversal.'
