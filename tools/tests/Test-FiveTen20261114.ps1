param([switch]$Baseline,[switch]$FloatBaseline)
if($Baseline -and $FloatBaseline){throw 'Choose one counterfactual'}
$ErrorActionPreference='Stop';$fiveTenRepo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$fiveTenOut="$fiveTenRepo/build-extractors/runtime-test/five-ten-20261114/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$fiveTenOut/evidence" -RuntimeDirectories @($fiveTenOut) -Test {
 $fiveTenArgs=@("$fiveTenRepo/tools/tests/prepare_five_ten_20261114.py",$fiveTenOut)
 if($Baseline){$fiveTenArgs+='baseline'}
 if($FloatBaseline){$fiveTenArgs+='float-baseline'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @fiveTenArgs
 if($LASTEXITCODE){throw 'Source extraction failed'}
 $fiveTenVs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $fiveTenDefines=Get-Content "$fiveTenOut/defines.txt" -Raw
 [IO.File]::WriteAllText("$fiveTenOut/run.cmd","@echo off`ncall `"$fiveTenVs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $fiveTenDefines /I`"$fiveTenOut`" `"$fiveTenRepo/tools/tests/FiveTen20261114Test.cpp`" /Fe`"$fiveTenOut/test.exe`" /Fo`"$fiveTenOut/test.obj`" /Fd`"$fiveTenOut/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$fiveTenOut/test.exe`"`nexit /b %errorlevel%`n")
 & "$fiveTenOut/run.cmd" *> "$fiveTenOut/compiler-run.log"
 if($LASTEXITCODE){Get-Content "$fiveTenOut/compiler-run.log" -Tail 16;throw 'Selected repair regression failed'}
 Get-Content "$fiveTenOut/compiler-run.log"
}
