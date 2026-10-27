param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/boost-departure-flight"
function Block([string]$s,[string]$marker){$a=$s.IndexOf($marker);if($a -lt 0){throw "Missing $marker"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){if($s[$b] -eq '{'){++$depth};if($s[$b] -eq '}'){--$depth};++$b};$s.Substring($a,$b-$a)}
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories $out -Test {
 $boost=Get-Content "$repo/src/server/scripts/Scenario/BoostExperience/boost_experience.cpp" -Raw
 [IO.File]::WriteAllText("$out/Departure.inc",(Block $boost 'struct DepartureAI :')+';')
 $creature=Get-Content "$repo/src/server/game/Entities/Creature/Creature.cpp" -Raw
 [IO.File]::WriteAllText("$out/MovementDefaults.inc",(Block $creature 'CreatureMovementData::CreatureMovementData()'))
 [IO.File]::WriteAllText("$out/UpdateMovementFlags.inc",(Block $creature 'void Creature::UpdateMovementFlags()'))
 $unit=Get-Content "$repo/src/server/game/Entities/Unit/Unit.cpp" -Raw
 $gravity=Block $unit 'bool Unit::SetDisableGravity('
 [IO.File]::WriteAllText("$out/AnimationTier.inc",(Block $gravity 'if (IsCreature() && updateAnimationTier'))
 $spline=Get-Content "$repo/src/server/game/Movement/Spline/MoveSplineInit.cpp" -Raw
 [IO.File]::WriteAllText("$out/SplineConstructor.inc",(Block $spline 'MoveSplineInit::MoveSplineInit(Unit&'))
 [IO.File]::WriteAllText("$out/Define.h",'// Native CreatureData enum/header with controlled test definitions.')
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncd /d `"$out`"`ncl /nologo /EHsc /std:c++17 /W4 $flags /I`"$out`" /I`"$repo/src/server/game/Entities/Creature`" `"$PSScriptRoot/BoostDepartureFlightTest.cpp`" /Fe:test.exe /Fo:test.obj /Fd:test.pdb`nif errorlevel 1 exit /b 1`ntest.exe`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Boost departure flight regression failed'}
}
