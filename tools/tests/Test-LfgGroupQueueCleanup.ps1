$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/lfg-group-queue-cleanup"
New-Item -ItemType Directory -Force $out | Out-Null
$source=Get-Content "$repo/src/server/game/DungeonFinding/LFGMgr.cpp" -Raw
$a=$source.IndexOf('void LFGMgr::RemoveFromGroupQueue(');$b=$source.IndexOf('bool LFGMgr::HasQueue(',$a)
if($a -lt 0 -or $b -le $a){throw 'Missing native queue cleanup'}
[IO.File]::WriteAllText("$out/Cleanup.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /W4 /I`"$out`" `"$PSScriptRoot/LfgGroupQueueCleanupTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'LFG group-queue regression failed'}
