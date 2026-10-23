param([switch]$Baseline,[switch]$VerifyReleaseBinary,[string]$ReleaseDirectory="$PSScriptRoot/../../build-extractors/bin/Release")
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/legendary-travel-forms"
New-Item -ItemType Directory -Force $out | Out-Null
function Source([string]$file) {
 if($Baseline){$s=(& git -C $repo show "e6cc8408:$file") -join "`n";if($LASTEXITCODE){throw "Cannot read pinned baseline $file"};return $s}
 [IO.File]::ReadAllText("$repo/$file")
}
function Section([string]$s,[string]$start,[string]$end) {
 $a=$s.IndexOf($start);if($a -lt 0){throw "Missing $start"};$b=$s.IndexOf($end,$a);if($b -le $a){throw "Missing $end"};$s.Substring($a,$b-$a)
}
$s=Source 'src/server/scripts/Spells/spell_item.cpp'
$start=if($Baseline){'// 242105 Vigilance Perch'}else{'static void UpdateLegendaryTravelForm('}
[IO.File]::WriteAllText("$out/Controllers.inc",(Section $s $start '// Orb of Destruction - 229700'))
$s=Source 'src/server/game/Spells/Auras/SpellAuraEffects.cpp'
[IO.File]::WriteAllText("$out/Shapeshift.inc",(Section $s 'void AuraEffect::HandleAuraModShapeshift(' 'void AuraEffect::HandleAuraTransform('))
$s=Source 'src/server/game/Entities/Unit/Unit.h'
[IO.File]::WriteAllText("$out/Forms.inc",(Section $s 'enum ShapeshiftForm' '#define MAX_SPELL_CHARM'))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$arg=if($Baseline){'baseline'}else{''}
[IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /I`"$out`" `"$PSScriptRoot/LegendaryTravelFormsTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" $arg`nexit /b %errorlevel%`n")
& "$out/run.cmd"
if($LASTEXITCODE){throw 'Legendary travel forms production regression failed'}
if($VerifyReleaseBinary){
 $python='C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
 & $python "$PSScriptRoot/verify_legendary_travel_forms_release.py" $ReleaseDirectory
 if($LASTEXITCODE){throw 'Linked legendary travel form verification failed'}
}
