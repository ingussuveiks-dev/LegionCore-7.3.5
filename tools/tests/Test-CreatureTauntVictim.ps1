param([string]$BaselineRevision,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/creature-taunt-victim"
New-Item -ItemType Directory -Force $out | Out-Null
$relative='src/server/game/Entities/Unit/Unit.cpp'
if($BaselineRevision){$source=(& git -C $repo show "${BaselineRevision}:$relative") -join "`n";if($LASTEXITCODE){throw 'Cannot read baseline'}}else{$source=Get-Content "$repo/$relative" -Raw}
$a=$source.IndexOf('Unit* Creature::SelectVictim()');$b=$source.IndexOf('bool Unit::GetThreatTarget(', $a)
if($a -lt 0 -or $b -le $a){throw 'Production victim selector not found'}
[IO.File]::WriteAllText("$out/SelectVictim.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$mode=if($BaselineRevision){'baseline'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/CreatureTauntVictimTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $mode`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Native taunt victim regression failed'}
if($VerifyReleaseBinary){
 if($BaselineRevision){throw 'Baseline reproduction is not a Release verification'}
 & 'C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' "$PSScriptRoot/verify_creature_taunt_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked taunt victim implementation check failed'}
}
