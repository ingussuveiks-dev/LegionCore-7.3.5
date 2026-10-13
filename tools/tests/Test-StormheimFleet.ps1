$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/stormheim-fleet"
New-Item -ItemType Directory -Force -Path $output | Out-Null
& "$PSScriptRoot/Export-QuestDelayedEvent.ps1" -OutputDirectory $output
$source=Get-Content "$repo/src/server/scripts/Legion/stormheim_fleet.cpp" -Raw
$source=[regex]::Replace($source,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$output/Fleet.inc",$source.Substring(0,$source.IndexOf('void AddSC_stormheim_fleet()')))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$repo/src/server/scripts/Legion`" /I`"$output`" `"$PSScriptRoot/StormheimFleetTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/test.exe`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`ncl /nologo /EHsc /std:c++17 /I`"$repo/src/server/scripts/Legion`" `"$PSScriptRoot/StormheimFleetStateTest.cpp`" /Fe`"$output/state.exe`" /Fo`"$output/state.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/state.exe`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`ncl /nologo /EHsc /std:c++17 `"$PSScriptRoot/CreatureScalingTest.cpp`" /Fe`"$output/scaling.exe`" /Fo`"$output/scaling.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$output/scaling.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Stormheim fleet production regression failed'}
