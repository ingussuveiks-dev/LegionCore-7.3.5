param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-intro"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "9dd043ad:$path") -join "`n";if($LASTEXITCODE){throw 'Missing pinned final intro baseline'}}
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
$first=$source.IndexOf('        EventMap events;');$last=$source.IndexOf('        void Reset()',$first)
if($first -lt 0 -or $last -le $first){throw 'Final intro member fields missing'}
[IO.File]::WriteAllText("$out/Fields.inc",$source.Substring($first,$last-$first))
foreach($name in @('Reset','MoveInLineOfSight')){[IO.File]::WriteAllText("$out/$name.inc",(Block $source "void $name("))}
$helper=if($Baseline){''}else{Block $source 'bool IsIntroTarget('}
[IO.File]::WriteAllText("$out/IsIntroTarget.inc",$helper)
$retry=if($Baseline){''}else{Block $source 'void UpdateIntro('}
[IO.File]::WriteAllText("$out/UpdateIntro.inc",$retry)
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.cpp" -Raw
[IO.File]::WriteAllText("$out/GetPlayer.inc",(Block $source 'Player* ObjectAccessor::GetPlayer('))
$source=Get-Content "$repo/src/server/game/Globals/ObjectAccessor.h" -Raw
[IO.File]::WriteAllText("$out/ObjectInMap.inc",(Block $source 'template<class T> static T* GetObjectInMap('))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsFinalIntroTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Final intro native regression failed'}
if($VerifyReleaseBinary){
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=98602;SELECT 'helya',guid,id,map,PhaseId FROM creature WHERE id=99043;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final intro binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("binding`t98602`tnpc_vigfus_bladewind_last","spawn`t361283`t98602`t1511`t7003","helya`t361286`t99043`t1511`t7003")){if($rows -notcontains $expected){throw "Final intro binding differs: $expected"}}
 if($rows.Count -ne 3){throw 'Additional shared final boss or Helya spawns require review'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_intro_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical final intro missing'}}finally{$env:PYTHONPATH=$previous}
}
