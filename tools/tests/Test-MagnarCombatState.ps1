param([switch]$AddressSanitizer)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/magnar-combat-state/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
function Block([string]$s,[string]$name){$a=$s.IndexOf($name);if($a -lt 0){throw "Missing $name"};$b=$s.IndexOf('{',$a)+1;$depth=1;while($depth){$depth+=([int]($s[$b] -eq '{'))-([int]($s[$b] -eq '}'));++$b};$s.Substring($a,$b-$a)}
$header=(Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Warrior/EarthWarder/magnar_combat.h" -Raw).Replace('#include "ObjectGuid.h"','')
[IO.File]::WriteAllText("$out/CombatHeader.inc",$header)
$source=Get-Content "$repo/src/server/scripts/Scenario/Artifacts/Paladin/ShieldsRest/instance_shields_rest.cpp" -Raw;$bridge=''
foreach($name in @('void MagnarCombat::Reset(','void MagnarCombat::Bind(','void MagnarCombat::Remove(','MagnarCombat::State* MagnarCombat::Find(','void MagnarCombat::Remember(','bool MagnarCombat::Restore(')){$bridge+=Block $source $name}
[IO.File]::WriteAllText("$out/CombatBridge.inc",$bridge)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$instrument=if($AddressSanitizer){'/fsanitize=address /Zi'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 $instrument /I`"$out`" `"$PSScriptRoot/MagnarCombatStateTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {& "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 12;throw 'Magnar combat state source regression failed'};Get-Content "$out/source.log" -Tail 1}
