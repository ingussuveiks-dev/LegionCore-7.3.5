$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
$out="$repo/build-extractors/runtime-test/stormheim-batrider"
New-Item -ItemType Directory -Force -Path $out | Out-Null
$code=Get-Content "$repo/src/server/scripts/Legion/stormheim_batrider.cpp" -Raw
$code=[regex]::Replace($code,'(?m)^#include.*\r?\n','')
[IO.File]::WriteAllText("$out/BatRider.inc",$code.Substring(0,$code.IndexOf('void AddSC_stormheim_batrider()')))
$path=Get-Content "$repo/src/server/scripts/Legion/stormheim_batrider_path.h" -Raw
$path=[regex]::Replace($path,'(?m)^#(?:include|pragma).*\r?\n','')
[IO.File]::WriteAllText("$out/BatPath.inc",$path)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimBatRiderTest.cpp`" /Fe`"$out/test.exe`" /Fo`"$out/test.obj`"`r`nif errorlevel 1 exit /b 1`r`n`"$out/test.exe`"`r`n"
[IO.File]::WriteAllText("$out/run.cmd",$cmd);& "$out/run.cmd"
if($LASTEXITCODE){throw 'Bat-Rider production regression failed'}
