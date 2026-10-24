param([switch]$Baseline,[switch]$VerifyReleaseBinary)
$ErrorActionPreference='Stop'
if($Baseline -and $VerifyReleaseBinary){throw 'Baseline cannot verify repaired Release'}
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/warswords-final-admission"
New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$source,[string]$start){$first=$source.IndexOf($start);if($first -lt 0){throw "Missing $start"};$last=$source.IndexOf('{',$first)+1;$depth=1;while($depth){if($source[$last] -eq '{'){++$depth};if($source[$last] -eq '}'){--$depth};++$last};$source.Substring($first,$last-$first)}
$path='src/server/scripts/Scenario/Artifacts/Warrior/Warswords/Warswords.cpp'
$source=Get-Content "$repo/$path" -Raw
if($Baseline){$source=(& git -C $repo show "14774c59:$path") -join "`n";if($LASTEXITCODE){throw 'Pinned admission baseline unavailable'}}
$source=$source.Substring($source.IndexOf('class npc_vigfus_bladewind_last :'))
foreach($entry in @(@('EncounterReady','bool IsEncounterReady('),@('CanAIAttack','bool CanAIAttack('),@('AttackStart','void AttackStart('))){[IO.File]::WriteAllText("$out/$($entry[0]).inc",$(if($Baseline){''}else{Block $source $entry[1]}))}
[IO.File]::WriteAllText("$out/Visibility.inc",$(if($Baseline){''}else{Block $source 'bool CanBeVisibleFor('}))
$source=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
[IO.File]::WriteAllText("$out/CreatureAttack.inc",(Block $source 'bool Creature::canCreatureAttack('))
[IO.File]::WriteAllText("$out/NativeVisibility.inc",(Block $source 'bool Creature::IsNeverVisible('))
$source=Get-Content "$repo/src/server/game/Entities/Object/Object.h" -Raw
[IO.File]::WriteAllText("$out/BaseVisibility.inc",(Block $source 'virtual bool IsNeverVisible('))
$source=Get-Content "$repo/src/server/game/AI/CreatureAI.h" -Raw
[IO.File]::WriteAllText("$out/BaseAIVisibility.inc",(Block $source 'virtual bool CanBeVisibleFor('))
$source=Get-Content "$repo/src/server/game/AI/ScriptedAI/ScriptedCreature.cpp" -Raw
foreach($name in @('AttackStart','AttackStartNoMove')){[IO.File]::WriteAllText("$out/Native$name.inc",(Block $source "void ScriptedAI::$name("))}
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$definition=if($Baseline){'/DBASELINE'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $definition /I`"$out`" `"$PSScriptRoot/WarswordsFinalAdmissionTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`n")
& "$out/run.cmd";if($LASTEXITCODE){throw 'Final native combat admission failed'}
if($Baseline){& "$PSScriptRoot/Test-WarswordsFinalThresholds.ps1" -AdmissionBaseline;if($LASTEXITCODE){throw 'Final scaled damage baseline failed'}}
if(!$Baseline){& "$PSScriptRoot/Test-WarswordsFinalThresholds.ps1" -VerifyReleaseBinary:$VerifyReleaseBinary;if($LASTEXITCODE){throw 'Final scaled damage admission failed'}}
if($VerifyReleaseBinary){
 $previous=$env:PYTHONPATH
 try{$env:PYTHONPATH="$repo/.codex/pe-review-deps;$previous";& C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/verify_warswords_final_admission_release.py" "$repo/build-extractors/bin/Release";if($LASTEXITCODE){throw 'Canonical final admission missing'}}finally{$env:PYTHONPATH=$previous}
}
