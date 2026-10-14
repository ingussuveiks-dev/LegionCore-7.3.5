$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$output="$repo/build-extractors/runtime-test/new-blood-archive"
New-Item -ItemType Directory -Force -Path $output | Out-Null
$source=Get-Content "$repo/src/server/scripts/Legion/new_blood.cpp" -Raw
$a=$source.IndexOf('struct npc_new_blood_jagganoth');$b=$source.IndexOf('// Keep the native missile/impact', $a)
if($a -lt 0 -or $b -lt 0){throw 'Archive production AI not found'}
[IO.File]::WriteAllText("$output/NewBloodArchive.inc",$source.Substring($a,$b-$a))
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++17 /W4 /I`"$output`" `"$PSScriptRoot/NewBloodArchiveTest.cpp`" /Fe`"$output/test.exe`" /Fo`"$output/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$output/test.exe`"`r`n"
[IO.File]::WriteAllText("$output/run.cmd",$cmd)
& "$output/run.cmd"
if($LASTEXITCODE){throw 'Archive lifecycle regression failed'}
