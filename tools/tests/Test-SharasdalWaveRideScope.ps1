$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/sharasdal-wave-ride-scope/$(Get-Date -Format yyyyMMdd-HHmmss-fff)";New-Item -ItemType Directory -Force $out|Out-Null
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory "$out/evidence" -RuntimeDirectories @($out) -Test {
 foreach($name in @('AreaTrigger.h','Unit.h','Scenario.h','ScenarioMgr.h')){[IO.File]::WriteAllText("$out/$name",'// Native service boundary for isolated scope admission only.')}
 Copy-Item "$repo/src/server/game/Entities/AreaTrigger/SharasdalWaveRide.h" "$out/SharasdalWaveRide.h"
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /Fd`"$out/compiler.pdb`" /I`"$out`" `"$PSScriptRoot/SharasdalWaveRideScopeTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`nexit /b %errorlevel%`r`n")
 & "$out/run.cmd" *> "$out/source.log";if($LASTEXITCODE){Get-Content "$out/source.log" -Tail 10;throw 'Wave ride scope ASAN failed'};Get-Content "$out/source.log" -Tail 2
}
