param([switch]$Baseline,[switch]$VerifyReleaseBinary,[switch]$FirstBoss)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-visibility"
if($FirstBoss){$out+='-first'}
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
function Source([string]$path){if($Baseline){$revision=if($FirstBoss){'baca33c7'}else{'e50ea50d'};$source=(& git -C $repo show "${revision}:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned visibility baseline unavailable'};return $source};Get-Content "$repo/$path" -Raw}
$source=Source 'src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
if($FirstBoss){
 $first=$source.Substring($source.IndexOf('class npc_vigfus_bladewind :'),$source.IndexOf('class npc_vigfus_bladewind_last :')-$source.IndexOf('class npc_vigfus_bladewind :'))
 foreach($entry in @(@('FirstActive','bool IsEncounterActive('),@('FirstReady','bool IsEncounterReady('),@('FirstVisibility','bool CanBeVisibleFor('),@('FirstCanAttack','bool CanAIAttack('),@('FirstAttack','void AttackStart('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",$(if($Baseline){''}else{Block $first $entry[1]}))}
 $phase=($first -split "`n"|Where-Object {$_ -match '^\s*bool phase\b'}) -join "`n"
 if(!$phase){throw 'First retreat state declaration missing'}
 [IO.File]::WriteAllText("$out/FirstPhase.inc",$phase)
}
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
if($FirstBoss){$definition+=' /DFIRST_REENTRY'}
$test=if($FirstBoss){'WarswordsFirstReentryTest.cpp'}else{'WarswordsFinalVisibilityTest.cpp'}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/$test`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Native final visibility regression failed'}
if($VerifyReleaseBinary){
 & "$PSScriptRoot/Test-WarswordsFinalAdmission.ps1" -VerifyReleaseBinary
 if($LASTEXITCODE){throw 'Existing native combat/presentation regression failed'}
 $native=Get-Content "$repo/build-extractors/runtime-test/warswords-final-thresholds/native.tsv"
 if(@($native | Where-Object {$_ -like "STEP`t*"}).Count -ne 7 -or $native -notcontains "CRITERION`t6`t29089`t92`t47017`t0"){throw 'Native seven-step944 terminal progression changed'}
 $connection=(Get-Content "$repo/build-extractors/bin/Release/worldserver.conf"|Where-Object {$_ -match '^WorldDatabaseInfo\s*='}).Split('"')[1].Split(';')
 $previous=$env:MYSQL_PWD
 try{$env:MYSQL_PWD=$connection[3];$rows=@("SELECT 'instance',map,script FROM instance_template WHERE map=1511;SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind_last';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=98602;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'Final visibility binding read failed'}}finally{$env:MYSQL_PWD=$previous}
 $bindingEvidence="$out/live-bindings-$(Get-Date -Format yyyyMMdd-HHmmss-fff).tsv"
 [IO.File]::WriteAllLines($bindingEvidence,[string[]]$rows)
 "Final visibility binding evidence: $bindingEvidence"
 foreach($expected in @("instance`t1511`tinstance_warswords","binding`t98602`tnpc_vigfus_bladewind_last","spawn`t361283`t98602`t1511`t7003")){if($rows -notcontains $expected){throw "Final visibility binding changed: $expected; returned $($rows.Count) rows, evidence $bindingEvidence"}}
 if($rows.Count -ne 3){throw 'Additional final visibility bindings need review'}
 if($FirstBoss){
  try{$env:MYSQL_PWD=$connection[3];$firstRows=@("SELECT 'binding',entry,ScriptName FROM creature_template WHERE ScriptName='npc_vigfus_bladewind';SELECT 'spawn',guid,id,map,PhaseId FROM creature WHERE id=98007;"|& C:/wamp64/bin/mariadb/mariadb11.4.9/bin/mysql.exe --host=$($connection[0]) --port=$($connection[1]) --user=$($connection[2]) --database=$($connection[4]) --batch --raw --skip-column-names);if($LASTEXITCODE){throw 'First reentry binding read failed'}}finally{$env:MYSQL_PWD=$previous}
  $firstEvidence="$out/first-bindings-$(Get-Date -Format yyyyMMdd-HHmmss-fff).tsv"
  [IO.File]::WriteAllLines($firstEvidence,[string[]]$firstRows)
  "First reentry binding evidence: $firstEvidence"
  foreach($expected in @("binding`t98007`tnpc_vigfus_bladewind","spawn`t361266`t98007`t1511`t7000")){if($firstRows -notcontains $expected){throw "First reentry binding changed: $expected; returned $($firstRows.Count) rows, evidence $firstEvidence"}}
  if($firstRows.Count -ne 2){throw 'Additional first reentry bindings need review'}
 }
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";$verifier=if($FirstBoss){'verify_warswords_first_reentry_release.py'}else{'verify_warswords_final_visibility_release.py'};& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/$verifier" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical visibility missing'}}finally{$env:PYTHONPATH=$previous}
}
