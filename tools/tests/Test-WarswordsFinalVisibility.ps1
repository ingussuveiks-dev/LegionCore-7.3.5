param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-visibility"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function Source([string]$path){if($Baseline){$source=(& git -C $repo show "e50ea50d:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned visibility baseline unavailable'};return $source};Get-Content "$repo/$path" -Raw}
$source=Source 'src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
foreach($entry in @(@('EncounterReady','bool IsEncounterReady('),@('CanAIAttack','bool CanAIAttack('),@('AttackStart','void AttackStart('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
[IO.File]::WriteAllText("$out/Visibility.inc",$(if($Baseline){''}else{Block $source 'bool CanBeVisibleFor('}))
$source=Source 'src/server/scripts/Scenario/Artifacts/Warrior/Warswords/instance_warswords.cpp'
foreach($entry in @(@('Create','void OnCreatureCreate('),@('Step','void onScenarionNextStep('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
[IO.File]::WriteAllText("$out/Remove.inc",$(if($Baseline){''}else{Block $source 'void OnCreatureRemove('}))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
foreach($entry in @(@('NativeVisibility','bool Creature::IsNeverVisible('),@('CreatureAttack','bool Creature::canCreatureAttack('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",(Block $source $entry[1]))}
$source=Get-Content "$repo/src/server/game/Entities/Object/Object.h" -Raw
[IO.File]::WriteAllText("$out/BaseVisibility.inc",(Block $source 'virtual bool IsNeverVisible('))
$source=Get-Content "$repo/src/server/game/AI/CreatureAI.h" -Raw
[IO.File]::WriteAllText("$out/BaseAIVisibility.inc",(Block $source 'virtual bool CanBeVisibleFor('))
$source=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
[IO.File]::WriteAllText("$out/VisibilityRefresh.inc",(Block $source 'void Unit::UpdateObjectVisibility('))
$source=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.cpp" -Raw
foreach($name in @('AttackStart','AttackStartNoMove')){[IO.File]::WriteAllText("$out/Native$name.inc",(Block $source "void ScriptedAI::$name("))}
$source=Get-Content "$PSScriptRoot/WarswordsFinalAdmissionTest.cpp" -Raw
[IO.File]::WriteAllText("$out/CombatHarness.inc",$source.Substring(0,$source.IndexOf('int main()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsFinalVisibilityTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native final visibility regression failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -VerifyReleaseBinary
 if($LASTEXITCODE){throw 'Existing native combat/presentation regression failed'}
 $native=Get-Content "$repo/build-extractors/runtime-test/warswords-final-thresholds/native.tsv"
 if(@($native | Where-Object {$_ -like "STEP`t*"}).Count -ne 7 -or $native -notcontains "CRITERION`t6`t29089`t92`t47017`t0"){throw 'Native seven-step944 terminal progression changed'}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'instance',map,script FROM instance_template WHERE map=1511;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=98602;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final visibility binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 foreach($expected in @("instance`t1511`tinstance_warswords","binding`t98602`tnpc_vigfus_bladewind_last","spawn`t361283`t98602`t1511`t7003")){if($rows -notcontains $expected){throw "Final visibility binding changed: $expected"}}
 if($rows.Count -ne 3){throw 'Additional final visibility bindings need review'}
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_visibility_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical final visibility missing'}}finally{$env:PYTHONPATH=$previous}
}
