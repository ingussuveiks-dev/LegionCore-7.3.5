param([switch]$Baseline)
$ErrorActionPreference='Stop';$goTravelRepo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$goTravelOut="$goTravelRepo/build-extractors/runtime-test/go-travel-20261114/$(Get-Date -Format yyyyMMdd-HHmmss-fff)"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$goTravelOut/evidence" -RuntimeDirectories @($goTravelOut) -Test {
 $goTravelArgs=@("$goTravelRepo/tools/tests/prepare_go_travel_20261114.py",$goTravelOut);if($Baseline){$goTravelArgs+='baseline'}
 & C:/Users/Admin/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe @goTravelArgs
 if($LASTEXITCODE){throw 'Travel source export failed'}
 $goTravelVs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 $goTravelDefines=Get-Content "$goTravelOut/defines.txt" -Raw
 [IO.File]::WriteAllText("$goTravelOut/run.cmd","@echo off`ncall `"$goTravelVs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi $goTravelDefines /I`"$goTravelOut`" `"$goTravelRepo/tools/tests/GoTravel20261114Test.cpp`" /Fe`"$goTravelOut/test.exe`" /Fo`"$goTravelOut/test.obj`" /Fd`"$goTravelOut/test.pdb`"`nif errorlevel 1 exit /b 1`n`"$goTravelOut/test.exe`"`nexit /b %errorlevel%`n")
 & "$goTravelOut/run.cmd" *> "$goTravelOut/compiler-run.log"
 if($LASTEXITCODE){Get-Content "$goTravelOut/compiler-run.log" -Tail 18;throw 'Actual travel-handler regression failed'}
 Get-Content "$goTravelOut/compiler-run.log"
}
