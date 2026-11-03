param([string]$OutputDirectory='')
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
if(!$OutputDirectory){$OutputDirectory="$repo/build-extractors/runtime-test/ragnvald-second-battle/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"};$out=[IO.Path]::GetFullPath($OutputDirectory)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/research-evidence" -RuntimeDirectories @($repo,$out) -Test {
 # This exporter first certifies all current Warrior successors, then emits
 # full CURRENT Ragnvald/Bezzeredes and explicitly pinned legacy Danica.
 & "$PSScriptRoot/Test-WarriorIntroHandoffs.ps1" -OutputDirectory $out
 if(!$?){throw 'Retained current Ragnvald/native handoff export failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run-research.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/RagnvaldSecondBattleResearch.cpp`" /Fe`"$out/research.exe`" /Fo`"$out/research.obj`" /Fd`"$out/research.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`ncd /d `"$out`"`n`"$out/research.exe`"`nexit /b %errorlevel%`n")
 & "$out/run-research.cmd";if($LASTEXITCODE){throw 'Ragnvald current-code counterfactual host failed'}
 'OPEN research only: two current caller contexts/control versus refused1412, no production repair or repaired regression claimed'
}
