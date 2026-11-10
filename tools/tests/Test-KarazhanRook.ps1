param([switch]$Baseline)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/karazhan-rook/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";[void][IO.Directory]::CreateDirectory($out)
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 $mode=if($Baseline){'baseline'}else{'fixed'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe "$PSScriptRoot/prepare_karazhan_rook.py" $out $mode
 if($LASTEXITCODE){throw 'Chess extraction failed'}
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $flags=if($Baseline){'/DCOUNTERFACTUAL'}else{''}
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /MD /fsanitize=address /Zi $flags /I`"$out`" `"$PSScriptRoot/KarazhanRookTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/compiler.pdb`"`nif %errorlevel% neq 0 exit /b %errorlevel%`n`"$out/test.exe`"`nexit /b %errorlevel%`n")
 & cmd.exe /d /c "`"$out/run.cmd`" > `"$out/result.log`" 2>&1"
 if($LASTEXITCODE){Get-Content "$out/result.log" -Tail 24;throw 'Chess ASAN regression failed'}
 Get-Content "$out/result.log" -Tail 1
}
$global:LASTEXITCODE=0
