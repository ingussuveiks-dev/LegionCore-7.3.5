$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath("$PSScriptRoot/../..")
& "$PSScriptRoot/Test-StormheimHuntingData.ps1"
& "$PSScriptRoot/Test-StormheimBlightcaller.ps1" *> "$repo/.codex/hunting-kill-upstream.log"
$out="$repo/build-extractors/runtime-test/stormheim-hunting"
Get-ChildItem "$repo/build-extractors/runtime-test/stormheim-blightcaller/*.inc" | Copy-Item -Destination $out
$f=Get-Content "$out/BlightFixture.inc" -Raw
$f=$f.Replace('GetEntry()==91158&&q==38361','GetEntry()==107675&&(q==39789||q==39793)')
[IO.File]::WriteAllText("$out/HuntingKillFixture.inc",$f)
$vs=& "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmd="@echo off`r`ncall `"$vs/VC/Auxiliary/Build/vcvars64.bat`" >nul`r`ncl /nologo /EHsc /std:c++20 /I`"$out`" `"$PSScriptRoot/StormheimHuntingKillTest.cpp`" /Fe`"$out/HuntingKill.exe`" /Fo`"$out/HuntingKill.obj`"`r`nif not %errorlevel%==0 exit /b %errorlevel%`r`n`"$out/HuntingKill.exe`" `"$out/world.tsv`"`r`n"
[IO.File]::WriteAllText("$out/run-kill.cmd",$cmd);& "$out/run-kill.cmd"
if($LASTEXITCODE){throw 'Hunting native kill regression failed'}
