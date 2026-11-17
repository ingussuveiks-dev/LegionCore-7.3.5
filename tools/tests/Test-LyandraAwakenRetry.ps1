param([Parameter(Mandatory)][string]$DataDirectory)
$ErrorActionPreference='Stop';$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..");$out="$repo/build-extractors/runtime-test/lyandra-awaken-retry"
& "$PSScriptRoot/Invoke-WithRuntimeDiagnostics.ps1" -EvidenceDirectory $out -Test {
 & "$PSScriptRoot/Test-SmartLinkedCastRetry.ps1" -OutputDirectory "$out/native"
 if(!$?){throw 'Retained linked-cast native regression failed'}
 $events=@(Get-Content "$DataDirectory/terminal-smart.tsv"|Where-Object {$f=$_.Split("`t");$f[0] -eq '99615' -and $f[1] -eq '0' -and [int]$f[2] -in 19,20,21}|ForEach-Object {$f=$_.Split("`t");(@(0,1,2,3,5,6,7,8,9,10,11,12,14,15,16,21,22,23)|ForEach-Object {$f[$_]}) -join ' '})
 if($events.Count -ne 3){throw 'Actual Lyandra cast/tail rows differ'};[IO.File]::WriteAllLines("$out/lyandra-events.tsv",$events)
 $vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
 [IO.File]::WriteAllText("$out/run.cmd","@echo off`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`ncl /nologo /EHsc /std:c++17 /W4 /fsanitize=address /Zi /I`"$out/native`" `"$PSScriptRoot/LyandraAwakenRetryTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`" /Fd`"$out/test.pdb`" /link /INCREMENTAL:NO`nif errorlevel 1 exit /b 1`n`"$out/test.exe`" `"$out/lyandra-events.tsv`"`nexit /b %errorlevel%`n")
 & "$out/run.cmd";if($LASTEXITCODE){throw 'Actual native Lyandra Awaken retry regression failed'}
}
