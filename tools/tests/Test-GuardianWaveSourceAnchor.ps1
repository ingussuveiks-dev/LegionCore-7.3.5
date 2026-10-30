param([string]$DataDirectory="$PSScriptRoot/../../build-extractors/runtime-test/guardian-wave-source-anchor-data")
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/guardian-wave-source-anchor"
$native="$repo/build-extractors/runtime-test/guardian-wave-death"
$original="$repo/build-extractors/runtime-test/guardian-wave-death-data"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -RuntimeDirectories @($native,$repo) -Test {
 $typeMasks=Get-Content "$repo/src/server/game/Entities/Object/ObjectGuid.h" -Raw
 if($typeMasks -notmatch 'TYPEMASK_UNIT\s*=\s*0x0008' -or $typeMasks -notmatch 'TYPEMASK_PLAYER\s*=\s*0x0010'){throw 'Native Unit/Player mask24 differs'}
 & "$PSScriptRoot/Test-GuardianWaveDeathData.ps1"
 if(!$?){throw 'Original wave integrity data failed'}
 & "$PSScriptRoot/Test-GuardianWaveDeath.ps1" -VerifyNativeCatalog
 if(!$?){throw 'Original native wave integrity failed'}
 $data=[IO.Path]::GetFullPath($DataDirectory)
 foreach($path in @("$data/original-conditions.tsv","$data/conditions.tsv")){if(!(Test-Path -LiteralPath $path)){throw "Missing source-anchor SQL evidence $path"}}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$native`" `"$PSScriptRoot/GuardianWaveSourceAnchorTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$original/original.tsv`" `"$original/repaired.tsv`" `"$original/original-conditions.tsv`" `"$original/conditions.tsv`" `"$data/original-conditions.tsv`" `"$data/conditions.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Native Guardian source-anchor regression failed'}
}
