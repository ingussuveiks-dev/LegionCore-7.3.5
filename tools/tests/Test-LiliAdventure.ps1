param()
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/lili-adventure"
New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_lili_adventure.py" $out
 if($LASTEXITCODE){throw 'Li Li production extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 foreach($source in @('LiliAdventureTest','PersonalCreatureTextTest')){
  [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out`" `"$PSScriptRoot/$source.cpp`" /Fe`"$out/$source.exe`" /Fo`"$out/$source.obj`" /Fd`"$out/$source.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/$source.exe`"`nexit /b %errorlevel%`n")
  & "$out/run.cmd";if($LASTEXITCODE){throw "Li Li regression failed: $source"}
 }
}
